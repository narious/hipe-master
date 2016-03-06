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
#include <iostream>
#include <sstream> //for std::stringstream
#include <unistd.h> //for getuid()
#include <sys/types.h> //for getuid()
#include <QWebSettings>
#include <QCommandLineParser>
#include "connectionmanager.h"
#include "containermanager.h"
#include "keylist.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName("hiped");
    a.setApplicationVersion("v0 alpha. Check README.md for more specific info.");
    QCommandLineParser clp;
    clp.setApplicationDescription("Hipe display server.");
    QCommandLineOption socketFileArg("socket", "Create server socket file with a custom name. The socket location is /tmp/ and cannot be changed.", "filename", "");
    clp.addOption(socketFileArg);
    QCommandLineOption keyFileArg("keyfile", "Create top-level host key file with a custom path and filename.", "file path", "");
    clp.addOption(keyFileArg);
    clp.addHelpOption();
    clp.addVersionOption();
    clp.process(a);

    a.setQuitOnLastWindowClosed(false);
    desktop = a.desktop();

    std::stringstream userid; userid << getuid();
    uid = userid.str(); //uid defined in containermanager.h as a convenient global variable.

    std::string keyFile;
    if(clp.isSet(keyFileArg))
        keyFile = clp.value(keyFileArg).toStdString();
    else
        keyFile = std::string("/tmp/hipe-uid")+uid+".hostkey";

    ContainerManager containerManager(keyFile);
    globalContainerManager = &containerManager; //make the instance globally available so containers can register themselves.
    ConnectionManager connectionManager(&containerManager);

    QString socketFile;
    if(clp.isSet(socketFileArg))
        socketFile = clp.value(socketFileArg);
    else
        socketFile = QString("hipe-uid")+uid.c_str()+".socket";

    connectionManager.removeServer(socketFile); //remove any abandoned socket file of the same name.
    if(connectionManager.listen(socketFile)) { //this maps the socket file and begins listening
    //by default, the socket is created as /tmp/hipe-uid1000.socket, where 1000 is the user's UID.
        std::cout << "Listening on " << connectionManager.fullServerName().toStdString() << "\n";
        return a.exec(); //start Qt's event loop, listening for connections, responding to events, etc.
    } else {
        std::cerr << "Couldn't open socket.\n";
        return 1;
    }
}
