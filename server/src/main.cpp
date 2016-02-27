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


// Hipe.
// A display layer/window system in pure webkit.
// It allows applications to produce web-like interfaces and manipulate them directly without the overhead of standard web protocols.
// It supplies a socket which applications connect to.
// Applications request a container via a key.
// They then do stuff by manipulating html code elements and executing javascript.


#include <QApplication>
#include <string>
#include <sstream> //for std::stringstream
#include <unistd.h> //for getuid()
#include <sys/types.h> //for getuid()
#include <QWebSettings>
#include "connectionmanager.h"
#include "containermanager.h"
#include "keylist.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setQuitOnLastWindowClosed(false);
    desktop = a.desktop();

    std::stringstream userid; userid << getuid();

    ContainerManager containerManager(std::string("/tmp/hipe-uid")+userid.str()+".hostkey");
    globalContainerManager = &containerManager; //make the instance globally available so containers can register themselves.
    ConnectionManager connectionManager(&containerManager);

    connectionManager.listen(QString("hipe-uid")+userid.str().c_str()+".socket"); //this maps the socket file and begins listening
    //the socket is created as /tmp/hipe-uid1000.socket, where 1000 is the user's UID.

    return a.exec(); //start Qt's event loop, listening for connections, responding to events, etc.
}
