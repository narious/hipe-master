#include <QTimer>

#include "connection.h"
#include "connectionmanager.h"
#include "main.hpp"

// The global server object accepts and manages connections.
// ONE connection propagates ONE client connection thread,
// which enables ONE client

// The socket server that listens for and accepts new connections
ConnectionManager::ConnectionManager(QObject *parent) : QObject(parent) {
  timer = new QTimer();
  timer->connect(timer, SIGNAL(timeout()), this, SLOT(timerEvent()));
  timer->start(40); // ms interval.
}

void ConnectionManager::timerEvent() {
  // when a timer event occurs in the main hiped event loop,
  // go thru the connection list and service all events.
  timer->stop();
  if (serviceConnections()) { // returns true if the call was productive
    serviceConnections();
    serviceConnections();
    serviceConnections(); // add more calls -- performance experiment.
    serviceConnections();
    serviceConnections();
    serviceConnections();
    serviceConnections();
    timer->start(1); // next event should therefore be scheduled sooner.
  } else {
    timer->start(80); // nothing much going on right now.
  }
}
