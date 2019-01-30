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
#include <sys/socket.h>
#include <sys/un.h>
#include <QWebSettings>
#include <QPixmap>
#include "connectionmanager.h"
#include "containertoplevel.h"
#include "keylist.h"
#include "sanitation.h"
#include "container.h"
#include "connection.h"
#include <map>
#include <thread>
#include <mutex>
#include <signal.h>
#include "brokenimg.h"

std::string uid;
bool verbose;
bool fillscreen;
int serverFD;
KeyList* topLevelKeyList;
std::string keyFilePath; //path and filename to store next available top-level key in.
std::string randomDevice;

std::map<int, Connection*> activeConnections; //maps each socket descriptor to its client connection object.
std::recursive_mutex mActiveConnections; //mutex to lock the activeConnections list during accesses.

void registerConnection(Connection* c, int fd)
{
    std::lock_guard<std::recursive_mutex> guard(mActiveConnections);
    activeConnections[fd] = c;
    //activeConnections.push_back(c);
}

void deregisterConnection(Connection* c)
{
    std::lock_guard<std::recursive_mutex> guard(mActiveConnections);
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
//CONCURRENCY: this function is only called from within Connection::runInstruction which only happens in the
//main thread with the activeConnections mutex held. Therefore there's no need to guard mActiveConnections -
//the lock is already held by this thread.
{
    if(topLevelKeyList->claimKey(key)) {
        makeNewTopLevelKeyFile();
        Container* container = new ContainerTopLevel(c, clientName);
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
    std::lock_guard<std::recursive_mutex> guard(mActiveConnections);
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
    std::lock_guard<std::recursive_mutex> guard(mActiveConnections); //lock activeConnections list for access.
    for(auto it=activeConnections.begin(); it!=activeConnections.end(); it++) {
        Connection* c = it->second;
        if(!c->isConnected()) {
            activeConnections.erase(it); //clean up defunct connection.
            delete c; //clean up connection object by running its destructor.
            return true; //can't continue iterating now since we've changed the list.
        }
        if(c->service()) was_productive = true;
    }
    return was_productive;
}


void incomingSocketThread() {
//responds to incoming socket events - new connection request, incoming instruction, etc.

    fd_set fds_to_watch;
    int max_fd;
    std::unique_lock<std::recursive_mutex> guard(mActiveConnections, std::defer_lock);
    //will lock this later to ensure mutual exclusion when accessing activeConnection map.

    while(true) {
        //fill the fd_set with all the socket descriptors we want to look for data on...
        FD_ZERO(&fds_to_watch);
        FD_SET(serverFD, &fds_to_watch);
        max_fd = serverFD;
        guard.lock();
        for(auto& elmnt : activeConnections) {
        //elmnt is a pair; first element is the socket descriptor.
            FD_SET(elmnt.first, &fds_to_watch);
            if(elmnt.first > max_fd) max_fd = elmnt.first;
        }
        guard.unlock(); //done accessing activeConnections for now.

        int numReady = select(++max_fd, &fds_to_watch, NULL, NULL, NULL); //block until there's something of interest.

        //just got out of select() call. Let's find out what's new...
        if(numReady == 0) continue; //nothing ready (timeout if applicable)
        if(numReady < 0) {
            if(errno == EBADF) {
            //A client may have disconnected but not yet been cleaned up by the service function.
                usleep(20000); //we don't know how long until the next service cycle, but there's no point
                //burning CPU cycles to spin this error repeatedly until then. Wait a short time; order of ~10ms.
                continue;
            }
            return; //error
        }

        if(FD_ISSET(serverFD, &fds_to_watch)) {
        //new connection pending...
            FD_CLR(serverFD, &fds_to_watch);
            numReady--;
            int clientFD = accept(serverFD, NULL, NULL);
            new Connection(clientFD);
        }

        guard.lock();  //check for file descriptors of active connections...
        for(auto& elmnt : activeConnections) {
            if(FD_ISSET(elmnt.first, &fds_to_watch)) {
                FD_CLR(elmnt.first, &fds_to_watch);

                //read from this file descriptor into the connection's instruction queue...
                elmnt.second->_readyRead();

                numReady--;
                if(numReady < 1) break;

            }
        }
        guard.unlock();
    }
}


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName("hiped");
    a.setApplicationVersion("v0 beta. Check README.md for more specific info.");

    std::stringstream userid; userid << getuid();
    uid = userid.str();
    randomDevice = ""; //unless overridden by --random argument, default behaviour is used.
    verbose = true;
    fillscreen = false;

    Sanitation::init();

    //set some global policies
    QWebSettings::globalSettings()->setAttribute(QWebSettings::LocalContentCanAccessFileUrls, false);
    QWebSettings::globalSettings()->setAttribute(QWebSettings::JavascriptEnabled, true);
    QWebSettings::globalSettings()->setAttribute(QWebSettings::PrivateBrowsingEnabled, true);

    //set default icon for corrupt/broken image tags.
    QPixmap brokenImgIcon;
    brokenImgIcon.loadFromData((const uchar*) brokenImgData, (uint) brokenImgDataLen);
    QWebSettings::globalSettings()->setWebGraphic(QWebSettings::MissingImageGraphic, brokenImgIcon);
    QWebSettings::globalSettings()->setWebGraphic(QWebSettings::MissingPluginGraphic, brokenImgIcon);


    //Parse command line options ad-hoc to avoid platform dependencies.
    //(QCommandLineParser isn't backwards compatible with Qt4.)
    std::string socketFileArg = ""; //store parameters in these vars if specified by user.
    std::string keyFileArg = "";
    std::string stylesheetArg = ""; //filename of a CSS file.
    for(int i=1; i<argc; i++) {
        std::string thisArg = argv[i];
        if(thisArg.compare("--help")==0 || thisArg.compare("-h")==0) { //print help text then exit normally.
            std::cout << "Usage: " << argv[0] << " [options]\nOptions:\nHipe display server\n\n";
            std::cout << "--css (file path)\n\tLoad a CSS file to provide default global styling to all clients.\n";
            std::cout << "--fill\n\tTop level frames fill the entire screen.\n";
            std::cout << "--keyfile (file path)\n\tCreate top-level host key file with a custom path and filename.\n";
            std::cout << "--random (file path)\n\tUse a hardware random device (e.g. /dev/random), for key generation.\n";
            std::cout << "--silent\n\tOnly critical error messages will be printed to the error console.\n";
            std::cout << "--socket (file path)\n\tCreate server socket file with a custom path and filename.\n";
            std::cout << "-h, --help\n\tOutput this help information.\n";
            return 0;
        } else if(thisArg.compare("--socket")==0) {
            //next arg must be present.
            if(++i == argc) {
                std::cerr << "Argument not optional. Try '" << argv[0] << " --help' for usage information\n";
                return 1;
            }
            socketFileArg = argv[i];
        } else if(thisArg.compare("--random")==0) {
            //next arg must be present.
            if(++i == argc) {
                std::cerr << "Argument not optional. Try '" << argv[0] << " --help' for usage information\n";
                return 1;
            }
            randomDevice = argv[i];
        } else if(thisArg.compare("--keyfile")==0) {
            //next arg must be present.
            if(++i == argc) {
                std::cerr << "Argument not optional. Try '" << argv[0] << " --help' for usage information\n";
                return 1;
            }
            keyFileArg = argv[i];
        } else if(thisArg.compare("--css")==0) {
            //next arg must be present.
            if(++i == argc) {
                std::cerr << "Argument not optional. Try '" << argv[0] << " --help' for usage information. [--stylesheet ...]\n";
                return 1;
            }
            stylesheetArg = argv[i];
        } else if(thisArg.compare("--silent")==0) {
            verbose = false;
        } else if(thisArg.compare("--fill")==0) {
            fillscreen = true;
        } else if(thisArg[0] == '-') {
            std::cerr << "Unrecognised option: " << thisArg << "\nTry '" << argv[0] << " --help' for usage information\n";
            return 1;
        }
    }

    signal(SIGPIPE, SIG_IGN); //broken client pipes must not crash the display server.

    a.setQuitOnLastWindowClosed(false);

    if(stylesheetArg.size()) { //load the stylesheet into a string
        std::ifstream cssFile(stylesheetArg);
        if(!cssFile.is_open()) {
            std::cerr << "hiped: Could not open stylesheet file.\n";
            return 1;
        } else if(verbose) {
            std::cerr << "hiped: Using CSS stylesheet at " << stylesheetArg << "\n";
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

    KeyList::initClass(randomDevice);
    topLevelKeyList = new KeyList("H");
    keyFilePath = keyFile;
    makeNewTopLevelKeyFile();

    ConnectionManager connectionManager;

    std::string socketFile;
    if(socketFileArg.size())
        socketFile = socketFileArg;
    else
        socketFile = std::string(default_path) + "hipe.socket";


    //create a new socket...
    serverFD = socket(AF_UNIX, SOCK_STREAM, 0);
    if(serverFD < 0) {
    std::cerr << "hiped: could not create server socket.\n";
        return 1;
    }

    //map the socket to the location of socketFile (socket file is now created)...
    struct sockaddr_un sockServAddr;
    sockServAddr.sun_family = AF_UNIX;
    strcpy(sockServAddr.sun_path, socketFile.c_str());
    unlink(socketFile.c_str()); //remove any abandoned socket file of the same name.
    if(bind(serverFD, (struct sockaddr*)&sockServAddr, sizeof(struct sockaddr_un)) != 0) {
        std::cerr << "hiped: could not create socket file.\n";
        return 1;
    }

    //make serverFD a 'listening' socket for new connections.
    if(listen(serverFD, 128) != 0) {
        std::cerr << "hiped: Socket error. Please try again.\n";
        return 1;
    }

    if(verbose)
        std::cerr << "hiped: Listening on " << socketFile << "\n";

    std::thread sockThread(incomingSocketThread);
    a.exec(); //start Qt's event loop, listening for connections, responding to events, etc.

    //clean up...
    sockThread.join();
    unlink(socketFile.c_str());
    delete topLevelKeyList;
    return 0;
}
