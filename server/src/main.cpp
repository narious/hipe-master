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
#include <fstream>
#include <sstream> //for std::stringstream
#include <unistd.h> //for getuid()
#include <sys/types.h> //for getuid()
#include <QWebSettings>
#include "connectionmanager.h"
#include "containermanager.h"
#include "keylist.h"
#include "sanitation.h"
#include "container.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName("hiped");
    a.setApplicationVersion("v0 beta. Check README.md for more specific info.");

    Sanitation::init();

    //Parse command line options ad-hoc to avoid platform dependencies.
    //(QCommandLineParser isn't backwards compatible with Qt4.)
    std::string socketFileArg = ""; //store parameters in these vars if specified by user.
    std::string keyFileArg = "";
    std::string stylesheetArg = ""; //filename of a CSS file.
    for(int i=1; i<argc; i++) {
        std::string thisArg = argv[i];
        if(thisArg.compare("--help")==0 || thisArg.compare("-h")==0) { //print help text then exit normally.
            std::cout << "Usage: " << argv[0] << " [options]\nOptions:\nHipe display server\n\n";
            std::cout << "--socket (file path)\n\tCreate server socket file with a custom path and filename.\n";
            std::cout << "--keyfile (file path)\n\tCreate top-level host key file with a custom path and filename.\n";
            std::cout << "--stylesheet (file path)\n\tLoad a CSS file to provide default global styling to all clients.\n";
            std::cout << "-h, --help\n\tDisplay this help information.\n";
            return 0;
        } else if(thisArg.compare("--socket")==0) {
            //next arg must be present.
            if(++i == argc) {
                std::cerr << "Argument not optional. Try '" << argv[0] << " --help' for usage information\n";
                return 1;
            }
            socketFileArg = argv[i];
        } else if(thisArg.compare("--keyfile")==0) {
            //next arg must be present.
            if(++i == argc) {
                std::cerr << "Argument not optional. Try '" << argv[0] << " --help' for usage information\n";
                return 1;
            }
            keyFileArg = argv[i];
        } else if(thisArg.compare("--stylesheet")==0) {
            //next arg must be present.
            if(++i == argc) {
                std::cerr << "Argument not optional. Try '" << argv[0] << " --help' for usage information\n";
                return 1;
            }
            stylesheetArg = argv[i];
        } else if(thisArg[0] == '-') {
            std::cerr << "Unrecognised option: " << thisArg << "\nTry '" << argv[0] << " --help' for usage information\n";
            return 1;
        }
    }

    a.setQuitOnLastWindowClosed(false);
    desktop = a.desktop();

    if(stylesheetArg.size()) { //load the stylesheet into a string
        std::ifstream cssFile(stylesheetArg);
        if(!cssFile.is_open()) {
            std::cout << "hiped: Could not open stylesheet file.\n";
            return 1;
        }
        std::stringstream buffer;
        buffer << cssFile.rdbuf();
        cssFile.close();
        Container::globalStyleRules = buffer.str();
    }

    std::stringstream userid; userid << getuid();
    uid = userid.str(); //uid defined in containermanager.h as a convenient global variable.

    char default_path[200]; //determine default runtime path for the current user.
    default_runtime_dir(default_path, 200);

    std::string keyFile;
    if(keyFileArg.size())
        keyFile = keyFileArg;
    else
        keyFile = std::string(default_path) + "hipe.hostkey";

    ContainerManager containerManager(keyFile);
    globalContainerManager = &containerManager; //make the instance globally available so containers can register themselves.
    ConnectionManager connectionManager(&containerManager);

    QString socketFile;
    if(socketFileArg.size())
        socketFile = socketFileArg.c_str();
    else
        socketFile = QString(default_path) + "hipe.socket";

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
