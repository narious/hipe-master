

#include "connectionmanager.h"
#include "connection.h"

//The global server object accepts and manages connections.
//ONE connection propagates ONE client connection thread,
//which enables ONE client

//The socket server that listens for and accepts new connections


ConnectionManager::ConnectionManager(QObject *parent) :
    QLocalServer(parent)
{
    connect(this,SIGNAL(newConnection()),this,SLOT(acceptConnection()));
}

void ConnectionManager::acceptConnection() {
    QLocalSocket* con = nextPendingConnection();
    if(!con) return; //no pending connections; false alarm.
    new Connection(con);
}
