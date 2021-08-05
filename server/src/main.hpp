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

// DECLARATIONS FOR GLOBAL RESOURCES

class Connection;

// string-format of UID number of the Unix user that owns this hiped instance.
extern std::string uid;

// random number device path (e.g. /dev/random). If empty, default behaviour is
// used.
extern std::string randomDevice;

// verbose mode (unless overridden by --silent arg) means success messages are
// printed to stderr.
extern bool verbose;

// as specified by --fill argument. When true, top-level windows are
// automatically resized to fill the entire screen. This is useful when running
// in a bare X session without a window manager.
extern bool fillscreen;

// the socket descriptor in which we listen for new client connection requests.
extern int serverFD;

// self-registration functions for connections to call when they are
// created/destructed.
void registerConnection(Connection *, int fd);
void deregisterConnection(Connection *);
bool serviceConnections();

extern KeyList *topLevelKeyList;
// path and filename to store next available top-level key in.
extern std::string keyFilePath;

// create file. Store random key in it.
void makeNewTopLevelKeyFile();

// Identify a connection from its DOM frame element.
// Return: nullptr if not found.
Connection *identifyFromFrame(QWebFrame *frame);

// Returns nullptr if request denied.
Container *requestContainerFromKey(std::string key, std::string clientName,
                                   uint64_t pid, Connection *c);
