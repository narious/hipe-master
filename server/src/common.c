/*  Copyright (c) 2015-2018 Daniel Kos, General Development Systems

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


size_t instruction_decoder_feed(instruction_decoder* obj, char* buffer, size_t bufferLen)
{
    size_t charsNeeded; //number of chars needed to fill an element of the instruction
    size_t charsConsumed=1; //number of chars consumed from the buffer.

    int i;
    size_t cumulativeLength = PREAMBLE_LENGTH;

    if(obj->instruction_chars_read < PREAMBLE_LENGTH) {
        charsNeeded = PREAMBLE_LENGTH - obj->instruction_chars_read;
        if(charsNeeded > bufferLen) 
            charsConsumed = bufferLen;
        else 
            charsConsumed = charsNeeded;

        //copy as many characters as are available/will fill the preamble out of the buffer.
        memcpy(obj->preamble + obj->instruction_chars_read, buffer, charsConsumed);
        obj->instruction_chars_read += charsConsumed;

        //obj->preamble[obj->instruction_chars_read++] = c; //OLD!!

        if(obj->instruction_chars_read == PREAMBLE_LENGTH) { /*Preamble complete.*/
            decodeInstructionPreamble(obj->preamble, &obj->output.opcode, &obj->output.requestor,
                                      &obj->output.location, obj->output.arg_length);
            hipe_instruction_alloc(&(obj->output));
            /* allocate memory in the instruction based on lengths read. */

            /*now ready to read args.*/
        }
    } else {
        for(i=0; i<HIPE_NARGS; i++) {
            if(obj->instruction_chars_read < cumulativeLength + obj->output.arg_length[i]) {
            //currently filling this arg in the resultant instruction...
                charsNeeded = (cumulativeLength + obj->output.arg_length[i])
                                        - obj->instruction_chars_read;

                if(charsNeeded > bufferLen)
                    charsConsumed = bufferLen; //eat them all
                else
                    charsConsumed = charsNeeded; //eat until arg is filled.

                //copy as many chars as we want to eat from the buffer to the arg.
                memcpy(obj->output.arg[i] + obj->instruction_chars_read - cumulativeLength, 
                        buffer, charsConsumed);
                obj->instruction_chars_read += charsConsumed;

                //obj->output.arg[i][obj->instruction_chars_read++ - cumulativeLength] = c;
                break;
            } else {
            //this arg has already been filled; account for its length
                cumulativeLength += obj->output.arg_length[i];
            }
        }
    }
    return charsConsumed;
}


short instruction_decoder_iscomplete(instruction_decoder* obj)
{
    size_t totalLength = PREAMBLE_LENGTH;
    int i=0;
    while(i<HIPE_NARGS) totalLength += obj->output.arg_length[i++];
    return (obj->instruction_chars_read >= totalLength);
}

void decodeInstructionPreamble(const char* preamble, char* opcode, uint64_t* requestor, hipe_loc* location, uint64_t arglen[])
/*preamble is an array of characters of length PREAMBLE_LENGTH that have been
 *read into the array.
 *They are the initial fixed-length preamble of the instruction.
 *Returns everything except the two variable-length arguments, but returns
 *their lengths so they can be read in separately.
 *PRECONDITION: preamble is an array of 17 characters, forming the start of an instruction, that have been read over a connection.
 *POSTCONDITION: reference arguments opcode, location, arg1len and arg2len will be populated
 *with values extracted from preamble.
 *AFTER RETURN: please read the two arguments separately using the returned
 *lengths arg1len and arg2len. The instruction is terminated
 *by reading the full lengths of the arguments.*/
{
    *requestor=0; *location=0;
    int i=0;
    while(i<HIPE_NARGS) arglen[i++]=0; //initialise argument lengths to zero

    *opcode = (unsigned char) preamble[0];

    unsigned int pos=1;
    unsigned short p;
    for(p=0; p<8; p++) {
        *requestor |= ((uint64_t)(unsigned char)preamble[pos++]) << (8*p);
    }
    for(p=0; p<8; p++) {
        *location |= ((uint64_t)(unsigned char)preamble[pos++]) << (8*p);
    }

    for(i=0; i<HIPE_NARGS; i++)
        for(p=0; p<_HIPE_ARG_WIDTH; p++) {
            arglen[i] |= ((uint64_t)(unsigned char)preamble[pos++]) << (8*p);
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
     * 8 bytes arg[0] length (number of bytes in string)
     * 8 bytes arg[1] length (number of bytes in string)
     * 8 bytes arg[...] (up to HIPE_NARGS - 1)
     * Variable bytes arg[0] (utf8 string or arbitrary non-null-terminated character array)
     * Variable bytes arg[1] (utf8 string or arbitrary non-null-terminated character array)
     * ... up to arg[HIPE_NARGS-1]
     **/

    unsigned short argWidth=8;
    uint64_t arglen[HIPE_NARGS];

    int i=0;
    for(; i<HIPE_NARGS; i++) //make temporary copies of argument lengths to work with.
        arglen[i] = instruction.arg_length[i];

    instruction_encoder_clear(obj);

    size_t encoding_length = PREAMBLE_LENGTH;
    for(i=0; i<HIPE_NARGS; i++) encoding_length += arglen[i];
    obj->encoded_output = (unsigned char*) malloc(encoding_length);
    obj->encoded_output[0] = instruction.opcode;
    size_t pos=1;
    size_t p;
    for(p=0; p<8; p++) {
        obj->encoded_output[pos++] = instruction.requestor & 255;
        instruction.requestor >>= 8;
    }
    for(p=0; p<8; p++) {
        obj->encoded_output[pos++] = instruction.location & 255;
        instruction.location >>= 8;
    }

    for(i=0; i<HIPE_NARGS; i++) {
        for(p=0; p<argWidth; p++) {
            //encode the least significant 8 bits of the length.
            obj->encoded_output[pos++] = arglen[i] & 255;
            //bit-shift along to the next least-significant byte of the length integer.
            arglen[i] >>= 8;
        }
    }

    for(i=0; i<HIPE_NARGS; i++) {
        for(p=0; p<instruction.arg_length[i]; p++) {
            obj->encoded_output[pos++] = instruction.arg[i][p];
        }
    }
    obj->encoded_length = pos;
}
