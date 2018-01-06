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
#include <QTimer>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream> //for std::stringstream
#include <unistd.h> //for getuid()
#include <sys/types.h> //for getuid()
#include <QWebSettings>
#include <QLocalServer>
#include "connectionmanager.h"
#include "containertoplevel.h"
#include "keylist.h"
#include "sanitation.h"
#include "container.h"
#include "connection.h"
#include <map>


std::string uid;
std::map<int, Connection*> activeConnections; //maps each socket descriptor to its client connection object.
KeyList* topLevelKeyList;
std::string keyFilePath; //path and filename to store next available top-level key in.


void registerConnection(Connection* c, int fd)
{
    activeConnections[fd] = c;
    //activeConnections.push_back(c);
}

void deregisterConnection(Connection* c)
{
    for(auto it=activeConnections.begin(); it!=activeConnections.end(); it++)
        if((it->second) == c) {
            activeConnections.erase(it);
            return;
        }
}

void makeNewTopLevelKeyFile()
//create file. Store random key in it.
{
    std::ofstream keyfile(keyFilePath);
    keyfile << topLevelKeyList->generateContainerKey();
    keyfile.close();
}

Container* requestContainerFromKey(std::string key, std::string clientName, uint64_t pid, Connection* c)
//returns nullptr if request denied.
{
    if(topLevelKeyList->claimKey(key)) {
        makeNewTopLevelKeyFile();
        Container* container = new ContainerTopLevel(c, QString::fromUtf8(clientName.c_str()));
        container->setTitle(clientName);
        return container;
    } else { //not top-level. Traverse each container in case the key refers to a sub-frame.
        for(auto& elmnt : activeConnections) {
        //elmnt is a pair. Second element points to Connection object.
            Connection* connection = elmnt.second;
            Container* container = connection->container;
            if(!container) continue;
            Container* newContainer = container->requestNew(key, clientName, pid, c);
            if(newContainer)
                return newContainer;
        }
    }
    return nullptr;
}

Connection* identifyFromFrame(QWebFrame* frame)
{
    for(auto& elmnt : activeConnections) { 
    //each element is a pair of <int descriptor, Connection*>
        if(!elmnt.second->container) continue;
        if(elmnt.second->container->frame == frame)
            return elmnt.second;
    }
    return nullptr;
}

bool serviceConnections() {
//calls the service() function on each active connection.
//This allows each connection to check if it has queued instructions waiting to be executed,
//and execute them upon the GUI as part of the main thread's event loop.
//RETURNS true if there have been productive service calls made - this can be used as a hint
//to repeat calling this function sooner as this is a period of activity.
    bool was_productive = false;
    for(auto& elmnt : activeConnections) {
    //elmnt is a pair. Second element points to Connection object.
        if(elmnt.second->service()) was_productive = true;
    }
    return was_productive;
}


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName("hiped");
    a.setApplicationVersion("v0 beta. Check README.md for more specific info.");

    std::stringstream userid; userid << getuid();
    uid = userid.str();

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

    char default_path[200]; //determine default runtime path for the current user.
    default_runtime_dir(default_path, 200);

    std::string keyFile;
    if(keyFileArg.size())
        keyFile = keyFileArg;
    else
        keyFile = std::string(default_path) + "hipe.hostkey";

    topLevelKeyList = new KeyList("H");
    keyFilePath = keyFile;
    makeNewTopLevelKeyFile();

    ConnectionManager connectionManager;

    QString socketFile;
    if(socketFileArg.size())
        socketFile = socketFileArg.c_str();
    else
        socketFile = QString(default_path) + "hipe.socket";

    connectionManager.removeServer(socketFile); //remove any abandoned socket file of the same name.
    if(connectionManager.listen(socketFile)) { //this maps the socket file and begins listening
        std::cout << "Listening on " << connectionManager.fullServerName().toStdString() << "\n";
        return a.exec(); //start Qt's event loop, listening for connections, responding to events, etc.
    } else {
        std::cerr << "Couldn't open socket.\n";
        return 1;
    }
}
