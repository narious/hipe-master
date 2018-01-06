/*  Copyright (c) 2018 Daniel Kos, General Development Systems

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

#pragma once

//DECLARATIONS FOR GLOBAL RESOURCES

class Connection;

extern std::string uid; //string-format of UID number of the Unix user that owns this hiped instance.

extern bool verbose; //verbose mode (unless overridden by --silent arg) means success messages are printed
//to stderr.

extern int serverFD; //the socket descriptor in which we listen for new client connection requests.

//self-registration functions for connections to call when they are created/destructed.
void registerConnection(Connection*, int fd);
void deregisterConnection(Connection*);
bool serviceConnections();

extern KeyList* topLevelKeyList;
extern std::string keyFilePath; //path and filename to store next available top-level key in.

void makeNewTopLevelKeyFile();
Connection* identifyFromFrame(QWebFrame* frame); //identify a connection from its DOM frame element.
Container* requestContainerFromKey(std::string key, std::string clientName, uint64_t pid, Connection* c);
