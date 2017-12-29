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

/*BEWARE: the same file is symlinked to the HIPElib source directory. Do not delete the original!
 */

#include "common.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

int default_runtime_dir(char path_ret[], size_t buffer_size)
{
    char* xdg_path;
    size_t length;
    if(!getuid()) { //if root.
        strncpy(path_ret, "/run/", buffer_size);
    } else if((xdg_path = getenv("XDG_RUNTIME_DIR"))) { //this will be a path like "/run/user/1000", if set.
        if(strlen(xdg_path) >= buffer_size-1) return -1;
        strncpy(path_ret, xdg_path, buffer_size);
    } else { //fallback if XDG_RUNTIME_DIR isn't set. /tmp/ is writable by everyone.
        strncpy(path_ret, "/tmp/", buffer_size);
    }
    length = strlen(path_ret);
    if(length < buffer_size-1) {
        if(path_ret[length-1] != '/')
            path_ret[length++] = '/'; //ensure trailing slash in path name.
        path_ret[length++] = '\0';
        return length;
    } else
        return -1;
}

void instruction_decoder_init(instruction_decoder* obj)
{
    hipe_instruction_init(&obj->output);
    obj->instruction_chars_read = 0;
}


void instruction_decoder_clear(instruction_decoder* obj)
{
    obj->instruction_chars_read = 0;
    hipe_instruction_clear(&obj->output);
}


void instruction_decoder_feed(instruction_decoder* obj, char c)
{
    if(obj->instruction_chars_read < PREAMBLE_LENGTH) {
        obj->preamble[obj->instruction_chars_read++] = c;

        if(obj->instruction_chars_read == PREAMBLE_LENGTH) { /*Preamble complete.*/
            decodeInstructionPreamble(obj->preamble, &obj->output.opcode, &obj->output.requestor,
                                      &obj->output.location, &(obj->output.arg_length[0]), &(obj->output.arg_length[1]));
            obj->output.arg[0] = (char*) malloc(obj->output.arg_length[0] /*+ 1*/); /* **DONT** allocate extra element for null termination.*/
            obj->output.arg[1] = (char*) malloc(obj->output.arg_length[1] /*+ 1*/);
            //obj->output.arg[0][obj->output.arg_length[0]] = '\0';
            //obj->output.arg[1][obj->output.arg_length[1]] = '\0';
            /*now ready to read args.*/
        }
    } else if(obj->instruction_chars_read < (PREAMBLE_LENGTH + obj->output.arg_length[0])) {
        obj->output.arg[0][obj->instruction_chars_read++ - PREAMBLE_LENGTH] = c;
    } else if(obj->instruction_chars_read < (PREAMBLE_LENGTH + obj->output.arg_length[0] + obj->output.arg_length[1])) {
        obj->output.arg[1][obj->instruction_chars_read++ - obj->output.arg_length[0] - PREAMBLE_LENGTH] = c;
    }
}


short instruction_decoder_iscomplete(instruction_decoder* obj)
{
    return (obj->instruction_chars_read >= PREAMBLE_LENGTH + obj->output.arg_length[0] + obj->output.arg_length[1]);
}




void decodeInstructionPreamble(const char* preamble, char* opcode, uint64_t* requestor, hipe_loc* location, uint64_t* arg1len, uint64_t* arg2len)
/*preamble is an array of characters of length PREAMBLE_LENGTH that have been read into the array.
 *They are the initial fixed-length preamble of the instruction.
 *Returns everything except the two variable-length arguments, but returns their lengths so they
 *can be read in separately.
 *PRECONDITION: preamble is an array of 17 characters, forming the start of an instruction, that have been read over a connection.
 *POSTCONDITION: reference arguments opcode, location, arg1len and arg2len will be populated
 *with values extracted from preamble.
 *AFTER RETURN: please read the two arguments separately using the returned lengths arg1len and arg2len. The instruction is terminated
 *by reading the full lengths of the arguments.*/
{
    *requestor=0; *location=0; *arg1len=0; *arg2len=0;

    *opcode = (unsigned char) preamble[0];

    unsigned int pos=1;
    unsigned short i;
    for(i=0; i<8; i++) {
        *requestor |= ((unsigned char)preamble[pos++]) << (8*i);
    }
    for(i=0; i<8; i++) {
        *location |= ((unsigned char)preamble[pos++]) << (8*i);
    }
    for(i=0; i<4; i++) {
        *arg1len |= ((unsigned char)preamble[pos++]) << (8*i);
    }
    for(i=0; i<4; i++) {
        *arg2len |= ((unsigned char)preamble[pos++]) << (8*i);
    }
}



void instruction_encoder_init(instruction_encoder* obj)
{
    obj->encoded_output = 0;
    obj->encoded_length = 0;
}


void instruction_encoder_clear(instruction_encoder* obj)
{
    if(obj->encoded_output) free(obj->encoded_output);
    obj->encoded_output = 0;
    obj->encoded_length = 0;
}


void instruction_encoder_encodeinstruction(instruction_encoder* obj, hipe_instruction instruction)
{
    /**
     * ENCODING SCHEME: (all integers are encoded little-endian)
     * 1 byte opcode
     * 8 bytes requestor -- client's reference (an unsigned integer that may correspond to a pointer or other reference value relevant to the client)
     * 8 bytes location -- server's reference (this is an unsigned integer representing an array position and 0 means not-applicable).
     * 4 bytes arg1 length (number of bytes in string)
     * 4 bytes arg2 length (number of bytes in string)
     * Variable bytes arg1 (utf8 string or arbitrary non-null-terminated character array)
     * Variable bytes arg2 (utf8 string or arbitrary non-null-terminated character array)
     **/

    unsigned short argWidth=4;
    size_t arg1Size = instruction.arg_length[0];
    size_t arg2Size = instruction.arg_length[1];

    instruction_encoder_clear(obj);

    obj->encoded_output = (unsigned char*) malloc(1+8+8+argWidth+argWidth + arg1Size + arg2Size);
    obj->encoded_output[0] = instruction.opcode;
    size_t pos=1;
    size_t i;
    for(i=0; i<8; i++) {
        obj->encoded_output[pos++] = instruction.requestor & 255;
        instruction.requestor >>= 8;
    }
    for(i=0; i<8; i++) {
        obj->encoded_output[pos++] = instruction.location & 255;
        instruction.location >>= 8;
    }
    for(i=0; i<argWidth; i++) {
        obj->encoded_output[pos++] = instruction.arg_length[0] & 255;
        instruction.arg_length[0] >>= 8;
    }
    for(i=0; i<argWidth; i++) {
        obj->encoded_output[pos++] = instruction.arg_length[1] & 255;
        instruction.arg_length[1] >>= 8;
    }
    for(i=0; i<arg1Size; i++) {
        obj->encoded_output[pos++] = instruction.arg[0][i];
    }
    for(i=0; i<arg2Size; i++) {
        obj->encoded_output[pos++] = instruction.arg[1][i];
    }
    obj->encoded_length = pos;
}


