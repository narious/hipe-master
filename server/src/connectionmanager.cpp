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

#include "connectionmanager.h"
#include "connection.h"

//The global server object accepts and manages connections.
//ONE connection propagates ONE client connection thread,
//which enables ONE OR MORE clients

//The socket server that listens for and accepts new connections

ConnectionManager::ConnectionManager(ContainerManager* theContainerManager, QObject *parent) :
    QLocalServer(parent)
{
    containerManager = theContainerManager;
//    setSocketOptions(QLocalServer::UserAccessOption); //setting access flags causes Qt to mangle socket path names
    connect(this,SIGNAL(newConnection()),this,SLOT(acceptConnection()));
}

void ConnectionManager::acceptConnection() {
    QLocalSocket* con = nextPendingConnection();
    if(!con) return; //no pending connections; false alarm.
    new Connection(con, containerManager);
}
