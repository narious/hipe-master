/*  Copyright (c) 2015 Daniel Kos, General Development Systems

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

#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/un.h> /*for struct sockaddr_un*/

#define READ_BUFFER_SIZE 128 /*this choice is arbitrary.*/
#define MAX_READS 50 /*the maximum number of consecutive read operations that can be completed without a return.*/

struct _hipe_session { /*all session-specific state variables go here!*/
    int connection_fd; /*File descriptor for the connection, or -1 when disconnected.*/

    char readBuffer[READ_BUFFER_SIZE];
    instruction_encoder outgoingInstruction;
    instruction_decoder incomingInstruction;

    /*linked list queue of incoming instructions:*/
    hipe_instruction* oldestInstruction;
    hipe_instruction* newestInstruction;
};

int readFromServer(hipe_session session, int blocking);
/*non-blocking read from server. Receives the number of characters available in the connection's
  input buffer, and begins assembling them into an instruction.
*/

void hipe_session_init(struct _hipe_session* obj) { /*contructor for a _hype_session struct instance.*/
    instruction_encoder_init(&obj->outgoingInstruction);
    instruction_decoder_init(&obj->incomingInstruction);
    obj->oldestInstruction = 0;
    obj->newestInstruction = 0;
}

void hipe_session_clear(struct _hipe_session* obj) { /*destructor for a _hype_session struct instance.*/
    instruction_encoder_clear(&obj->outgoingInstruction);
    instruction_decoder_clear(&obj->incomingInstruction);
}



void hipe_disconnect(hipe_session session) {
/* Private function to close a session's server connection without freeing the server struct.
 * The user doesn't need to call this, it is called automatically by other functions when the client
 * receives a critical error, the connection is broken, or the user calls hipe_close_session().
 * Postcondition: the session's file descriptor is set to -1. Other functions should check for this
 * before attempting to transmit or receive data. */

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
    } else if(getenv("HIPE_HOSTKEY")) { /*key specified by environment [note: each key can only be used once.]*/
        strncpy(key, getenv("HIPE_HOSTKEY"), 200);
    } else { /*need to load a key from the given key_path.*/
        FILE* keyfile;
        keyfile = fopen(keyPath, "r");
        if(!keyfile) { /*could not open keyfile*/
            perror("Hipe: Could not open keyfile");
            perror(keyPath);
            return 0;
        }
        fgets(key, 200, keyfile);
    }

    /*Allocate a socket endpoint file descriptor.*/
    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        return 0; /*null pointer*/
    }

    printf("Trying to connect...\n");
    struct sockaddr_un remote;
    remote.sun_family = AF_UNIX;
    strcpy(remote.sun_path, sockPath);
    int len = strlen(remote.sun_path) + sizeof(remote.sun_family);
    if (connect(fd, (struct sockaddr *)&remote, len) == -1) {
        close(fd);
        perror("Hipe: Could not connect to socket");
        perror(sockPath);
        return 0; /*null pointer*/
    }

    /*Now request a container using a given host key:*/
    hipe_session session = (hipe_session) malloc(sizeof(struct _hipe_session));
    hipe_session_init(session);
    session->connection_fd = fd;
    hipe_instruction rq;
    rq.opcode = HIPE_OPCODE_REQUEST_CONTAINER;
    rq.location = rq.requestor = 0;
    rq.arg1 = key;
    rq.arg1Length = strlen(key);
    rq.arg2 = (char*) clientName;
    rq.arg2Length = strlen(clientName);
    hipe_send_instruction(session, rq);

    /*Await response from server. If the container request is rejected then close the
    session immediately and don't return it.*/
    hipe_instruction incoming;
    hipe_instruction_init(&incoming);
    int result;
    result = hipe_await_instruction(session, &incoming, HIPE_OPCODE_CONTAINER_GRANT);
    if(result<0) {
        printf("Something wrong; bad connection.\n");
        hipe_close_session(session);
        exit(0);
    }
    if(incoming.arg1[0] != '1') {
        printf("Container request denied.\n");
        hipe_close_session(session);
        return 0; /*null pointer*/
    } else {
        printf("Container request granted!\n");
    }
    hipe_instruction_clear(&incoming);

    return session; /*success*/
}


int hipe_send_instruction(hipe_session session, hipe_instruction instruction)
/*encode and transmit an instruction.*/
{
    ssize_t err;

    if(session->connection_fd == -1) return -1; //not connected.

    instruction_encoder_encodeinstruction(&session->outgoingInstruction, instruction);
    /*send the instruction over the connection.*/
    err = send(session->connection_fd, session->outgoingInstruction.encoded_output, 
               session->outgoingInstruction.encoded_length, MSG_NOSIGNAL);

    if(err == -1) hipe_disconnect(session);

    return 0; /*success*/
}


short hipe_next_instruction(hipe_session session, hipe_instruction* instruction_ret, short blocking)
{
    short result;

    hipe_instruction_clear(instruction_ret); /*clear any previous instruction so that the user doesn't have to.*/
   
    while(!session->newestInstruction) {
    /*Only read something new from server if the queue is empty.*/
        result = readFromServer(session, blocking);
        if(!blocking && result == 0) return 0; 
        else if(result == -1) { /* Not connected to server */
            instruction_ret->opcode = HIPE_OPCODE_SERVER_DENIED;
            return -1;
        }
    }

    /*pull next instruction from queue and update queue.*/
    *instruction_ret = *(session->newestInstruction); /*shallow copy*/
    free(session->newestInstruction);                 /*shallow clear. Any args now exist in instruction_ret only.*/
    session->newestInstruction = instruction_ret->next;
    instruction_ret->next = 0;

    return 1; /*success*/
}


int readFromServer(hipe_session session, int blocking)
/*If blocking is set, the function will not return until at least a partial instruction has been read.
 *This function processes zero or more complete instructions before it returns. It calls the publish
 *function to process each instruction after it has read it.
 *
 * Returns the number of completed instructions read into the session queue (if any), or -1 on error.
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
            hipe_close_session(session);
            return -1;
        } else for(p=0; p<bufferedChars; p++) { /*let's process our input! (Iterate for each character we've read in)*/
            char c = session->readBuffer[p];

            instruction_decoder_feed(&session->incomingInstruction, c);
            if(instruction_decoder_iscomplete(&session->incomingInstruction)) {

                if(session->incomingInstruction.output.opcode == HIPE_OPCODE_SERVER_DENIED) {
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
    hipe_instruction* pre_newest; //what's the newest instruction before we start getting new instructions?
    hipe_instruction* found_in_queue; //the address of the desired instruction, once we find it.
    int fetched_instructions;

    pre_newest = session->newestInstruction; //this will be null if we begin with an empty queue.

    hipe_instruction_clear(instruction_ret); /*clear any previous instruction so that the user doesn't have to.*/

    while(1) { /*while we haven't received our desired instruction yet...*/
        fetched_instructions = 0;
        while(fetched_instructions == 0) {
            fetched_instructions = readFromServer(session, 1);
            if(fetched_instructions < 0) /*bad. handle error.*/
                return -1;
        }

        /* Handle special case: If we began with an empty queue so there was no 'pre_newest' yet.
         * We can't just assign pre_newest to the new oldest queue element: that might actually
         * be our awaited instruction rather than an instruction preceding it.
         */
        if(!pre_newest) {
            if(session->oldestInstruction->opcode == opcode) { /*found it!*/
                found_in_queue = session->oldestInstruction;
                session->oldestInstruction = found_in_queue->next; //remove it from the queue.
                if((session->newestInstruction = found_in_queue)) session->newestInstruction = 0;

                *instruction_ret = *found_in_queue; /*shallow copy*/
                instruction_ret->next = 0;
                free(found_in_queue);               /*shallow clear. Any args now exist in instruction_ret only.*/
                return 1;
            } else
                pre_newest = session->oldestInstruction;
        }


        int i;
        for(i=0; i<fetched_instructions; i++) { /*check all the newly-fetched instructions for required opcode.*/
            if(pre_newest->next->opcode == opcode) { /*found it!*/
                found_in_queue = pre_newest->next;
                pre_newest->next = pre_newest->next->next; /*remove it from the queue.*/
                if(session->newestInstruction == found_in_queue) session->newestInstruction = pre_newest;

                *instruction_ret = *found_in_queue; /*shallow copy*/
                instruction_ret->next = 0;
                free(found_in_queue);               /*shallow clear. Any args now exist in instruction_ret only.*/
                return 1;
            } else {
                pre_newest = pre_newest->next;
            }
        }
    }
}


int hipe_send(hipe_session session, char opcode, uint64_t requestor, hipe_loc location, const char* arg1, const char* arg2) {
    int result;
    hipe_instruction instruction;
    hipe_instruction_init(&instruction);
    instruction.opcode = opcode;
    instruction.requestor = requestor;
    instruction.location = location;
    if(arg1) {
        instruction.arg1 = malloc(strlen(arg1)+1);
        strcpy(instruction.arg1, arg1);
        instruction.arg1Length = strlen(arg1);
    }
    if(arg2) {
        instruction.arg2 = malloc(strlen(arg2)+1);
        strcpy(instruction.arg2, arg2);
        instruction.arg2Length = strlen(arg2);
    }
    result = hipe_send_instruction(session, instruction);
    hipe_instruction_clear(&instruction);
    return result;
}


