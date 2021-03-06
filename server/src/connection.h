/*  Copyright (c) 2016-2021 Daniel Kos, General Development Systems

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

/// Connection class. Objects of this class read input instructions from the
/// connected client application one at a time, assemble a preamble first then
/// a complete instruction, then pass the instruction to either the
/// containerManager or the client's container object to deal with.
///
/// Lifetime:
/// - Created by connectionManager, then left to be autonomous.
/// - Discards itself when connection is closed.
/// - Makes requests to ContainerManager, and exchanges instructions with a
/// container object provided by same.

// OWNERSHIP:
// The connection owns the on-screen container object. It is responsible for
// deleting the container object when destroyed.

#ifndef CONNECTION_H
#define CONNECTION_H

// WARN(Di): api/hipe.c also defined a READ_BUFFER_SIZE 128, are they related?
#define READ_BUFFER_SIZE 256 // 128 //this choice is arbitrary.

#include <mutex>
#include <queue>

#include "common.h"
#include "container.h"

class Connection {
private:
  // UNCLEAR: why predefined separately.
  bool connected = true;

public:
  Connection(int clientFD);
  ~Connection();

  // send one encoded insturction to dedicated client according to their fd.
  void sendInstruction(char opcode, uint64_t requestor, uint64_t location,
                       const std::vector<std::string> &args = {});
  void sendInstruction(hipe_instruction &instruction);
  Container *container;

  // The hiped event loop iterates over each activeConnection and calls the
  // service() function in each, returning to an idle state if all connections
  // return false (unproductive call). The purpose of service() is to check if
  // an incoming instruction has been queued by the socket thread and service
  // it in the primary/GUI thread; by modifying the GUI appropriately.
  bool service();

  inline bool isConnected() { return connected; }
  void disconnect();

  // TODO: bad name, rename to read
  // read instruction from `readBuffer`, then decode, then move buffer content
  // to `incomingInstructions`
  void _readyRead();

private:
  int clientFD;    // socket descriptor of the connection.
  pid_t clientPID; // pid_t == int

  // where we put characters that have been read in over the connection.
  char readBuffer[READ_BUFFER_SIZE];

  // UNCLEAR : reusable?
  instruction_decoder currentInstruction;

  // deals with a completed instruction -- sends it to wherever it's needed.
  void runInstruction(hipe_instruction *instruction);

  std::queue<hipe_instruction *> incomingInstructions;

  // instruction queue is filled by the incomingInstruction thread, and emptied
  // in the main thread. Mutual exclusion must be enforced to ensure no two
  // threads access the queue simultaneously.
  std::mutex mIncomingInstructions;

  // mutex to enforce atomicity of write calls when sending messages back
  // to client.
  std::mutex mWriteProtect;
};

#endif // CONNECTION_H
