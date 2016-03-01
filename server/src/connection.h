/*  Copyright (c) 2015 Daniel Kos, General Development Systems

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

///This is the new L1io.
/// Rather than populating a buffer with information, this class will
/// read input instructions one at a time, assemble a preamble first then
/// a complete instruction, then pass the instruction to a Client object
/// to deal with.
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
#include "common.h"
#include "containermanager.h"

class Connection : public QObject
{
    Q_OBJECT
public:
    explicit Connection(QLocalSocket* con, ContainerManager* containerManager, QObject *parent = 0);
    ~Connection();

    void sendInstruction(char opcode, int64_t requestor, int64_t location, std::string arg1, std::string arg2);
    void sendInstruction(hipe_instruction& instruction);

private:
    QLocalSocket* con;
    ContainerManager* containerManager;
    Container* container;

    char readBuffer[READ_BUFFER_SIZE]; //where we put characters that have been read in over the connection.

    instruction_decoder currentInstruction;

    void publishInstruction();
    //deals with a completed instruction -- sends it to wherever it's needed.

signals:

public slots:
    void _readyRead();
    void _disconnected();
};

#endif // CONNECTION_H
