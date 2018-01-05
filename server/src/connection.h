/*  Copyright (c) 2016 Daniel Kos, General Development Systems

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

/// Connection class. Objects of this class read input instructions from the connected
/// client application one at a time, assemble a preamble first then
/// a complete instruction, then pass the instruction to either the containerManager
/// or the client's container object to deal with.
///
///Lifetime:
/// - Created by connectionManager, then left to be autonomous.
/// - Discards itself when connection is closed.
/// - Makes requests to ContainerManager, and exchanges instructions with a container object provided by same.

#ifndef CONNECTION_H
#define CONNECTION_H

#define READ_BUFFER_SIZE 256 //128 //this choice is arbitrary.

#include <QObject>
#include <QLocalSocket>
#include <queue>
#include "common.h"
#include "container.h"

#include <iostream> //for debugging only

class Connection : public QObject
{
    Q_OBJECT
public:
    explicit Connection(QLocalSocket* con, QObject *parent = 0);
    ~Connection();

    void sendInstruction(char opcode, int64_t requestor, int64_t location, std::string arg1, std::string arg2);
    void sendInstruction(hipe_instruction& instruction);
    Container* container;

    bool service();
    //for future implementation: The hiped event loop iterates over each activeConnection and calls the
    //service() function in each, returning to an idle state if all connections return false (unproductive call).
    //The purpose of service() is to check if an incoming instruction has been queued by the socket thread
    //and service it in the primary/GUI thread; by modifying the GUI appropriately.

private:
    QLocalSocket* con;

    char readBuffer[READ_BUFFER_SIZE]; //where we put characters that have been read in over the connection.

    instruction_decoder currentInstruction;

    void runInstruction(hipe_instruction* instruction);
    //deals with a completed instruction -- sends it to wherever it's needed.

    std::queue<hipe_instruction*> incomingInstructions;

signals:

public slots:
    void _readyRead();
    void _disconnected();
};

#endif // CONNECTION_H
