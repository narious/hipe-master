/*  Copyright (c) 2016-2018 Daniel Kos, General Development Systems

    This file is part of Hipe.

    Hipe is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Hipe is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Hipe.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "connection.h"
#include "common.h"
#include "connectionmanager.h"
#include "container.h"
#include "main.hpp"
#include <errno.h>
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>

Connection::Connection(int clientFD) {
  this->clientPID = 0; // to be obtained once data starts flowing.

  this->clientFD = clientFD;
  container = nullptr;

  instruction_decoder_init(&currentInstruction);

  registerConnection(this, clientFD); // con->socketDescriptor());
}

// Deconstructor of `Connection`.
// this happens in the main thread, in the service cycle.
Connection::~Connection() {
  disconnect(); // mark as disconnected if not already done.
  instruction_decoder_clear(&currentInstruction);
  delete container;
}

void Connection::sendInstruction(char opcode, uint64_t requestor,
                                 uint64_t location,
                                 const std::vector<std::string> &args) {
  if (!connected)
    return;
  hipe_instruction instruction;
  hipe_instruction_init(&instruction);
  instruction.opcode = opcode;
  instruction.requestor = requestor;
  instruction.location = location;
  for (size_t i = 0; i < args.size(); i++) {
    if (args[i].size()) {
      instruction.arg[i] = (char *)args[i].data();
      instruction.arg_length[i] = args[i].size();
    }
  }

  sendInstruction(instruction);
}

void Connection::sendInstruction(hipe_instruction &instruction) {
  if (!connected)
    return;
  std::lock_guard<std::mutex> guard(mWriteProtect);

  instruction_encoder outgoingInstruction;
  instruction_encoder_init(&outgoingInstruction);
  instruction_encoder_encodeinstruction(&outgoingInstruction, instruction);

  ssize_t charsWritten;
  const char *bufferToWrite = (const char *)outgoingInstruction.encoded_output;
  size_t bytesRemaining = outgoingInstruction.encoded_length;
  while (bytesRemaining > 0) {
    charsWritten = write(clientFD, bufferToWrite, bytesRemaining);
    if (charsWritten < 1 /* write error */) {
      disconnect();
      break;
    } else {
      bytesRemaining -= charsWritten; // change size
      bufferToWrite += charsWritten;  // move pointer
    }
  }

  instruction_encoder_clear(&outgoingInstruction);
}

void Connection::runInstruction(hipe_instruction *instruction) {
  if (!connected)
    return;
  if (instruction->opcode == HIPE_OP_REQUEST_CONTAINER) {

    // Get credentials for the client process.
    socklen_t credLen = sizeof(struct ucred);
    struct ucred pidCredentials;
    if (0 == getsockopt(clientFD, SOL_SOCKET, SO_PEERCRED, &pidCredentials,
                        &credLen)) {
      // got client creds.
      this->clientPID = pidCredentials.pid;
    }
    if (!clientPID) // fallback to self-reported PID if above approach fails.
      this->clientPID = instruction->requestor;

    container = requestContainerFromKey(
        std::string(instruction->arg[0], instruction->arg_length[0]),
        std::string(instruction->arg[1], instruction->arg_length[1]),
        this->clientPID, this);
    // send the result of the container request (arg1 represents
    // approved/denied)
    sendInstruction(
        HIPE_OP_CONTAINER_GRANT, 0, 0,
        {(container ? "1" : "0"), "0"}); // new client awaits this confirmation
                                         // that its key has been approved.
  } else if (container) { // allow other instructions only if a container
                          // request has already been granted.
    // send the instruction to the container.
    container->receiveInstruction(*instruction);
  } else {
    // error. Access was not granted, so no other instructions are permitted.
    sendInstruction(HIPE_OP_SERVER_DENIED, 0, 0); // access denied.
  }
}

// The hiped event loop iterates over each activeConnection and calls the
// service() function in each, returning to an idle state if all connections
// return false (unproductive call). The purpose of service() is to check if
// an incoming instruction has been queued by the socket thread and service it
// in the primary/GUI thread; by modifying the GUI appropriately.
bool Connection::service() {
  if (!connected)
    return false;
  std::lock_guard<std::mutex> guard(mIncomingInstructions);
  if (incomingInstructions.empty())
    return false; // unproductive call.
  hipe_instruction *hi;
  while (!incomingInstructions.empty()) {
    hi = incomingInstructions.front();
    incomingInstructions.pop();
    runInstruction(hi);
    hipe_instruction_clear(hi);
    delete hi;
    if (!connected)
      return false;
  }
  return true; // this was a productive call.
}

void Connection::disconnect() {
  connected = false;
  shutdown(clientFD, SHUT_RDWR);
  close(clientFD);
}

void Connection::_readyRead() {
  if (!connected)
    return;

  // blocking call! ; // TODO(Di): unclear comment.
  short bufferedChars = read(clientFD, readBuffer, READ_BUFFER_SIZE);

  if (bufferedChars <= 0 /* read error */) {
    disconnect();
    return;
  } else {
    // let's process our input! (Iterate for each character we've read in)
    for (int p = 0; p < bufferedChars;) {
      p += instruction_decoder_feed(&currentInstruction, readBuffer + p,
                                    bufferedChars - p);
      // check if instruction is complete.
      if (instruction_decoder_iscomplete(&currentInstruction)) {
        hipe_instruction *newInstruction = new hipe_instruction;
        hipe_instruction_move(newInstruction, &(currentInstruction.output));
        std::lock_guard<std::mutex> guard(mIncomingInstructions);
        incomingInstructions.push(newInstruction);
        instruction_decoder_clear(&currentInstruction);
      }
    }
  }
}
