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

#ifndef _HIPE_H
#define _HIPE_H

#include <sys/types.h>
#include "hipe_instruction.h"

struct _hipe_session;
typedef struct _hipe_session* hipe_session;


hipe_session hipe_open_session(const char* host_key, const char* socket_path, const char* key_path, const char* client_name);

short hipe_close_session(hipe_session);

int hipe_send_instruction(hipe_session session, hipe_instruction instruction);
/*encodes and transmits an instruction.*/

short hipe_next_instruction(hipe_session session, hipe_instruction* instruction_ret, short blocking);
/* optional blocking-wait for an instruction to be received from the server.
 * Returns 1 if a new instruction has been returned in instruction_ret, or 0 if nothing is available
 * and !blocking.
 */

short hipe_await_instruction(hipe_session session, hipe_instruction* instruction_ret, short opcode);
/* Awaits an instruction with a particular opcode, then returns that instruction without adding it
 * to the session queue.
 * Any other instructions that arrive in the meantime are placed in the session queue so they
 * can be retrieved as normal by hipe_next_instruction when checking for events.
 * The awaited instruction must not already be in the session queue when this function is called.
 * This means you should never call hipe_next_instruction in between making a request and retrieving
 * an acknowledgement from the server.
 */

int hipe_send(hipe_session session, char opcode, uint64_t requestor, hipe_loc location, int n_args, ...);
/* Convenience function to send instructions when the arguments (0 or more) are null-terminated strings expressed
 * as char* or const char*
 */
 

#endif

#ifdef __cplusplus
}
#endif
