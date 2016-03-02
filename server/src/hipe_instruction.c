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

#include "hipe_instruction.h"

#include <stdlib.h>
#include <string.h>

void hipe_instruction_init(hipe_instruction* obj)
{
    obj->opcode = 0;
    obj->location = 0;
    obj->requestor = 0;
    obj->arg1 = 0;
    obj->arg2 = 0;
    obj->arg1Length = 0;
    obj->arg2Length = 0;
    obj->next = 0;
}


void hipe_instruction_clear(hipe_instruction* obj)
{
    if(obj->arg1) free(obj->arg1);
    if(obj->arg2) free(obj->arg2);
    hipe_instruction_init(obj);
}


void hipe_instruction_copy(hipe_instruction* dest, hipe_instruction* src)
{
    *dest = *src;
    dest->arg1 = malloc(dest->arg1Length);
    dest->arg2 = malloc(dest->arg2Length);
    uint32_t i;
    for(i=0; i<dest->arg1Length; i++)
        dest->arg1[i] = src->arg1[i];
    for(i=0; i<dest->arg2Length; i++)
        dest->arg2[i] = src->arg2[i];
}
