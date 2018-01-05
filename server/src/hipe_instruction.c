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

#include "hipe_instruction.h"

#include <stdlib.h>
#include <string.h>

void hipe_instruction_init(hipe_instruction* obj)
{
    obj->opcode = 0;
    obj->location = 0;
    obj->requestor = 0;
    int i;
    for(i=0; i<HIPE_NARGS; i++) {
        obj->arg[i] = NULL;
        obj->arg_length[i] = 0; //zero length means the argument is unspecified.
    }
    obj->next = 0;
}


void hipe_instruction_alloc(hipe_instruction* obj)
{
    int i;
    for(i=0; i<HIPE_NARGS; i++) {
        if(obj->arg_length[i]) {
            obj->arg[i] = malloc(obj->arg_length[i]+1);
            obj->arg[i][obj->arg_length[i]] = '\0';
            //Note: this function places a null-pointer after the end of each
            //argument (not counted in the length of the argument) for convenience
            //of accessing string arguments. Instructions received will have been
            //encoded similarly.  However, Hipe does not require instructions
            //to be encoded this way for transmission.
        } else
            obj->arg[i] = NULL;
    }
}


void hipe_instruction_clear(hipe_instruction* obj)
{
    int i;
    for(i=0; i<HIPE_NARGS; i++)
        if(obj->arg_length[i])
            free(obj->arg[i]); //this is a no-op for null pointers.
    hipe_instruction_init(obj);
}


void hipe_instruction_copy(hipe_instruction* dest, hipe_instruction* src)
{
    *dest = *src;
    hipe_instruction_alloc(dest);
    int i;
    for(i=0; i<HIPE_NARGS; i++) 
        if(dest->arg_length[i]) 
            memcpy(dest->arg[i], src->arg[i], dest->arg_length[i]);
}

void hipe_instruction_move(hipe_instruction* dest, hipe_instruction* src) {
    *dest = *src;
    hipe_instruction_init(src);
}
