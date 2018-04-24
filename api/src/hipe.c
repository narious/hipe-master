/*  Copyright (c) 2016-2018 Daniel Kos, General Development Systems

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of this Software library.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
    THE SOFTWARE.
*/

#include "hipe.h"
#include "common.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/un.h> /*for struct sockaddr_un*/
#include <pthread.h>

#define READ_BUFFER_SIZE 128 /*this choice is arbitrary.*/
/* Defines the size (in bytes) of the read buffer into which instruction data is
 * read in from the display server. Data read in a single read operation may
 * correspond to one or more instructions, or even a fragment of a single
 * large instruction. A larger value of READ_BUFFER_SIZE will allow more data to
 * be read in a single read operation, but there is no guarantee that the
 * hiped display server process will often send large chunks of data at once.
 * A smaller value of READ_BUFFER_SIZE will require more consecutive read
 * operations to read in the same data, but will process incoming data with
 * greater regularity when there is a large stream of data incoming. */

#define MAX_READS 50
/*the maximum number of consecutive read operations that can be completed
 *without a return.*/

struct _hipe_session { /*all session-specific state variables go here!*/
    int connection_fd; /*File descriptor for the connection, or -1 when disconnected.*/

    pthread_mutex_t send_lock; //this mutex is used to make sending outgoing
    //instructions threadsafe.

    char readBuffer[READ_BUFFER_SIZE];
    instruction_encoder outgoingInstruction;
    instruction_decoder incomingInstruction;

    /*linked list queue of incoming instructions:*/
    hipe_instruction* oldestInstruction;
    hipe_instruction* newestInstruction;
};

int read_to_queue(hipe_session session, int blocking);
/*blocking or nonblocking read from server. Receives the number of characters
 *available in the connection's input buffer, and begins assembling them into an
 *instruction. */

void hipe_session_init(struct _hipe_session* obj) {
/*contructor for a _hype_session struct instance.*/
    instruction_encoder_init(&obj->outgoingInstruction);
    instruction_decoder_init(&obj->incomingInstruction);
    pthread_mutex_init(&obj->send_lock, NULL);
    obj->oldestInstruction = 0;
    obj->newestInstruction = 0;
}

void hipe_session_clear(struct _hipe_session* obj) {
/*destructor for a _hype_session struct instance.*/
    instruction_encoder_clear(&obj->outgoingInstruction);
    instruction_decoder_clear(&obj->incomingInstruction);
    pthread_mutex_destroy(&obj->send_lock);
}

void hipe_disconnect(hipe_session session) {
/* Private function to close a session's server connection without freeing the server struct.
 * The user doesn't need to call this, it is called automatically by other functions when the client
 * receives a critical error, the connection is broken, or the user calls hipe_close_session().
 * Postcondition: the session's file descriptor is set to -1. Other functions should check for this
 * before attempting to transmit or receive data. */

    if(session->connection_fd == -1) return; //already disconnected.
    shutdown(session->connection_fd, SHUT_RDWR);
    close(session->connection_fd);
    session->connection_fd = -1;
}

hipe_session hipe_open_session(const char* host_key, const char* socket_path, const char* key_path, const char* clientName) {
/*connect to the host socket. A custom socket file path may be specified.
  The parameters may be null pointers in which case default values are used instead.
*/

    int fd; /*file descriptor*/

    char sockPath[200];
    char keyPath[200];
    char key[200];

    if(socket_path) { /*socket path specified by the caller*/
        strncpy(sockPath, socket_path, 200);
    } else if(getenv("HIPE_SOCKET")) { /*socket path specified by environment variable HIPE_SOCKET*/
        strncpy(sockPath, getenv("HIPE_SOCKET"), 200);
    } else { /*Infer default socket path*/
        default_runtime_dir(sockPath, 200);
        strncat(sockPath, "hipe.socket", 200-strlen(sockPath));
    }

    if(key_path) { /*keyfile path specified by caller*/
        strncpy(keyPath, key_path, 200);
    } else if(getenv("HIPE_KEYFILE")) { //keyfile path specified by environment variable HIPE_KEYFILE
        strncpy(keyPath, getenv("HIPE_KEYFILE"), 200);
    } else { /*Infer default keyfile path*/
        default_runtime_dir(keyPath, 200);
        strncat(keyPath, "hipe.hostkey", 200-strlen(keyPath));
    }

    if(host_key) { /*host key specified by user*/
        strncpy(key, host_key, 200);
    } else if(getenv("HIPE_HOSTKEY")) { /*key specified by environment [NB: each key can only be used once.]*/
        strncpy(key, getenv("HIPE_HOSTKEY"), 200);
    } else { /*need to load a key from the given key_path.*/
        FILE* keyfile;
        keyfile = fopen(keyPath, "r");
        if(!keyfile) { /*could not open keyfile*/
            fprintf(stderr, "Hipe: Could not open keyfile: %s\n", keyPath);
            perror("Hipe");
            return 0;
        }
        if(!fgets(key, 200, keyfile)) {
            fprintf(stderr, "Hipe: Could not read key from keyfile: %s\n", keyPath);
            perror("Hipe");
            return 0;
        }
    }

    /*Allocate a socket endpoint file descriptor.*/
    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("Hipe: socket");
        return 0; /*null pointer*/
    }

    struct sockaddr_un remote;
    remote.sun_family = AF_UNIX;
    strcpy(remote.sun_path, sockPath);
    int len = strlen(remote.sun_path) + sizeof(remote.sun_family);
    if (connect(fd, (struct sockaddr *)&remote, len) == -1) {
        close(fd);
        fprintf(stderr, "Hipe: Could not connect to socket: %s\n", sockPath);
        perror("Hipe");
        return 0; /*null pointer*/
    }

    /*Now request a container using a given host key:*/
    hipe_session session = (hipe_session) malloc(sizeof(struct _hipe_session));
    hipe_session_init(session);
    session->connection_fd = fd;
    hipe_instruction rq;
    hipe_instruction_init(&rq);
    rq.opcode = HIPE_OP_REQUEST_CONTAINER;
    rq.location = 0;
    rq.requestor = getpid();
    rq.arg[0] = key;
    rq.arg_length[0] = strlen(key);
    rq.arg[1] = (char*) clientName;
    rq.arg_length[1] = strlen(clientName);
    hipe_send_instruction(session, rq);

    /*Await response from server. If the container request is rejected then close the
    session immediately and don't return it.*/
    hipe_instruction incoming;
    hipe_instruction_init(&incoming);
    int result;
    result = hipe_await_instruction(session, &incoming, HIPE_OP_CONTAINER_GRANT);
    if(result<0) {
        fprintf(stderr, "Hipe: Bad connection.\n");
        hipe_close_session(session);
        return 0;
    }
    if(incoming.arg[0][0] != '1') {
        fprintf(stderr, "Hipe: Container request denied.\n");
        hipe_close_session(session);
        return 0; /*null pointer*/
    } else {
        //fprintf(stderr, "Hipe: Container request granted!\n"); /*uncomment for extra verbosity*/
    }
    hipe_instruction_clear(&incoming);

    return session; /*success*/
}


int hipe_send_instruction(hipe_session session, hipe_instruction instruction) {
/*encode and transmit an instruction.*/
    ssize_t err;
    if(session->connection_fd == -1) return -1; //not connected.

    pthread_mutex_lock(&session->send_lock);
    //enforce atomicity so that two threads can send instructions without
    //messing up the encoding. Note that receiving instructions is NOT
    //thread safe, so only one thread should require and be checking for
    //replies.

    instruction_encoder_encodeinstruction(&session->outgoingInstruction, instruction);
    /*send the instruction over the connection.*/
    err = send(session->connection_fd, session->outgoingInstruction.encoded_output,
               session->outgoingInstruction.encoded_length, MSG_NOSIGNAL);

    if(err == -1) hipe_disconnect(session);
    pthread_mutex_unlock(&session->send_lock);

    return 0; /*success*/
}


short hipe_next_instruction(hipe_session session, hipe_instruction* instruction_ret, short blocking)
{
    short result;

    hipe_instruction_clear(instruction_ret);
    /*clear any previous instruction so that the user doesn't have to.*/

    while(!session->oldestInstruction) {
    /*Only read something new from server if the queue is empty.*/
        result = read_to_queue(session, blocking);
        if(!blocking && result == 0) return 0;
        else if(result == -1) { /* Not connected to server */
            instruction_ret->opcode = HIPE_OP_SERVER_DENIED;
            return -1;
        }
    }

    /*pull next instruction from queue and update queue.*/
    *instruction_ret = *(session->oldestInstruction); /*shallow copy*/
    if(session->oldestInstruction == session->newestInstruction)
        session->newestInstruction = 0; /* if this was the only waiting instruction, reflect the now empty state of queue */
    free(session->oldestInstruction);   /*shallow clear. Any args now exist in instruction_ret only.*/
    session->oldestInstruction = instruction_ret->next;
    instruction_ret->next = 0;

    return 1; /*success*/
}


int read_to_queue(hipe_session session, int blocking)
/*If blocking is set, the function will not return until at least a partial
 *instruction has been read. This function processes zero or more complete
 *instructions before it returns. It then adds these to the session's incoming
 *instruction queue.
 *
 *Returns the number of completed instructions read into the session queue (if
 *any), or -1 on error.
 */
{
    if(session->connection_fd == -1) return -1; /*not connected*/

    int completedInstructions;
    completedInstructions = 0;
    short bufferedChars; /*number of characters that have been read into the buffer. Must be <=READ_BUFFER_SIZE */
    short n;
    for(n=0; n<MAX_READS; n++) {
    /*stay in this function for as long as characters are available to be read,
      or until the maximum number of allowed iterations is exhausted.
    */

        if(n>0) blocking=0; /*only enable blocking for the first iteration. Anything that follows is a freebie.*/

        /*attempt to read new characters*/
        bufferedChars = recv(session->connection_fd, session->readBuffer, READ_BUFFER_SIZE, (blocking ? 0 : MSG_DONTWAIT));
        /*can return -1 if connection closed, or 0 when no more ready.*/

        int p;
        if(bufferedChars < 0) { /*connection closed, or error. Or nothing to read right now.*/
            if(errno == EAGAIN) { /*nothing more to read right now*/
                return completedInstructions; /*success*/
            } else { /*disconnected by peer, broken pipe, etc.*/
                hipe_disconnect(session);
                return -1;
            }
        } else if(bufferedChars == 0) { /*connection closed by peer*/
            hipe_disconnect(session);
            return -1;
        } else for(p=0; p<bufferedChars; p++) { /*let's process our input! (Iterate for each character we've read in)*/
            char c = session->readBuffer[p];

            instruction_decoder_feed(&session->incomingInstruction, c);
            if(instruction_decoder_iscomplete(&session->incomingInstruction)) {

                if(session->incomingInstruction.output.opcode == HIPE_OP_SERVER_DENIED) {
                /*Access to the server has been denied. Critical. Disconnect*/
                    hipe_disconnect(session);
                    if(completedInstructions) return completedInstructions;
                    else return -1; /*disconnected*/
                }

                completedInstructions++;

                /*Allocate the new instruction struct and add it to the session's queue of new instructions.*/
                hipe_instruction* newInstruction = (hipe_instruction*) malloc(sizeof(hipe_instruction));
                hipe_instruction_copy(newInstruction, &session->incomingInstruction.output);

                instruction_decoder_clear(&session->incomingInstruction);

                if(session->newestInstruction) session->newestInstruction->next = newInstruction;
                else session->oldestInstruction = newInstruction; /*if the queue is empty, then it's our oldest as well as our newest.*/
                session->newestInstruction = newInstruction;
            }
        }
    }
    return completedInstructions; /*success*/
}


short hipe_close_session(hipe_session session)
{
    hipe_disconnect(session);
    hipe_session_clear(session);
    free(session);
    return 0;
}


short hipe_await_instruction(hipe_session session, hipe_instruction* instruction_ret, short opcode)
{
    hipe_instruction* current=session->oldestInstruction; /* the last instruction we have examined in the queue, or are about to examine. */
    hipe_instruction* previous=0; /* the instruction that points to current. */
    int fetched_instructions=0;

    while(1) { /* we will either return the desired instruction eventually, or return an error condition, such as disconnection. */
        while(current) { /* when we run out of instructions to examine, we'll have to leave this loop to get more */
            /* examine current instruction */
            if(current->opcode == opcode) {
                /* we've found the element we're looking for. Splice it out of the queue and return it. */
                *instruction_ret = *current; /*return a shallow copy to use existing allocations. We'll delete the original */

                if(previous)
                    previous->next = current->next;
                else /* current is at start of queue */
                    session->oldestInstruction = current->next;

                if(current == session->newestInstruction) {
                    session->newestInstruction = previous;
                    if(previous)
                        session->newestInstruction->next = 0;
                }

                free(current);
                instruction_ret->next = 0;

                return 1; /* success */
            }

            /* traverse to next in queue */
            previous = current;
            current = current->next;
        }

        /* need to fetch more instructions */
        do {
            fetched_instructions = read_to_queue(session, 1);
            if(fetched_instructions < 0) /*bad. handle error.*/
                return -1;
        } while(fetched_instructions == 0);

        if(previous)
            current = previous->next;
        else
            current = session->oldestInstruction;
    }
}

int hipe_send(hipe_session session, char opcode, uint64_t requestor, hipe_loc location, int n_args, ...) {
    int result;
    hipe_instruction instruction;
    hipe_instruction_init(&instruction);
    instruction.opcode = opcode;
    instruction.requestor = requestor;
    instruction.location = location;
    int i;
    va_list args;
    const char* this_arg;
    va_start(args, n_args); //process variadic arguments.
    for(i=0; i<n_args && i<HIPE_NARGS; i++) {
        this_arg = va_arg(args, const char*);
        if(this_arg) {
            instruction.arg_length[i] = strlen(this_arg); //adding null terminator to length breaks certain behaviour. Why?
            //note that the encoded arguments in the instruction should *not* be null terminated.
            instruction.arg[i] = (char*) this_arg;
        }
    }
    va_end(args);
    result = hipe_send_instruction(session, instruction);
    //hipe_instruction_clear(&instruction);
    return result;
}
