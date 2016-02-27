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

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Header for functions common to both the Hipe server and API code.
 * The same version of this file must be used for both processes.
 * This means functions used for encoding and decoding instructions intended for transmission across the GUI bridge, for example.
 *
 * BEWARE: the same file is symlinked to the libhipe source directory. Do not delete the original!
 **/

#ifndef COMMON_H
#define COMMON_H

#define PREAMBLE_LENGTH 25 /*number of bytes you need to read before you can determine the total instruction length.*/

#include <stdint.h>
#include <sys/types.h>

#include "hipe_instruction.h"

/**
 *  instruction_decoder struct-based class.
 *  to instantiate: allocate an instruction_decoder instance and call instruction_decoder_init() on it.
 *  to re-use: call instruction_decoder_clear on the instance to reset it to a clean state.
 *  to destroy: call instruction_decoder_clear then free the struct instance from memory.
 *  to use: call instruction_decoder_feed() to feed characters into the decoder until
 *  instruction_decoder_iscomplete() returns true. Then read the decoded instruction in the output variable.
 **/

struct _instruction_decoder {
    char preamble[PREAMBLE_LENGTH];

    unsigned int instruction_chars_read;
    /* number of characters of the instruction that have been read in so far.
     * Once this reaches the predefined preamble length, we have collected enough information to
     * determine the total instruction length and allocate a full-sized array accordingly.*/

    hipe_instruction output;
};
typedef struct _instruction_decoder instruction_decoder;

void instruction_decoder_init(instruction_decoder*);
/*analogous to an object constructor. Must be called on any new instruction_decoder instance before first use.*/

void instruction_decoder_clear(instruction_decoder*);
/*must be called before freeing memory associated with an instruction_decoder instance, or to reset an
instance to a clean state for reuse.*/

void instruction_decoder_feed(instruction_decoder*, char);
/*feed the next character into the decoder.*/

short instruction_decoder_iscomplete(instruction_decoder*);
/*use this to check if the complete instruction has been read and decoded. If it returns true,
the completed instruction may be read from the output member variable.*/



void decodeInstructionPreamble(const char* preamble, char* opcode, uint64_t* requestor, uint64_t* location, uint32_t* arg1len, uint32_t* arg2len);




/**
 * instruction_encoder struct-based class.
 * To instantiate: allocate an instruction_encoder instance and call instruction_encoder_init()
 * To destroy: call instruction_encoder_clear() and then de-allocate the instruction_encoder struct.
 * To use: call instruction_encoder_encodeinstruction() on the struct instance. The struct is populated
 * with encoded output for transmission over a socket or network connection.
 * To re-use: call instruction_encoder_clear() on the struct instance.
 **/

struct _instruction_encoder {
    unsigned char* encoded_output;
    size_t encoded_length;
};
typedef struct _instruction_encoder instruction_encoder;

void instruction_encoder_init(instruction_encoder*);
/*equivalent to a constructor for an instruction_encoder object. Must be called in initialise the struct.*/

void instruction_encoder_clear(instruction_encoder*);
/*clears the contents of an instruction_encoder struct for re-use, or to free memory prior to destruction.*/

void instruction_encoder_encodeinstruction(instruction_encoder*, hipe_instruction);


#endif
#ifdef __cplusplus
}
#endif
