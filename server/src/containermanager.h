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

#ifndef CONTAINERMANAGER_H
#define CONTAINERMANAGER_H

#include "keylist.h"
#include "container.h"
#include <QDesktopWidget>
#include <QWebFrame>

extern ContainerManager* globalContainerManager; //pointer to the one and only ContainerManager instance created in main().
extern QDesktopWidget* desktop;
extern std::string uid; //UID number of the Unix user that owns this hiped instance.

class ContainerManager
{
public:
    ContainerManager(std::string keyFilePath);
    //keyFilePath is the path/filename of the keyfile to store the key for creating top-level windows in.
    //ContainerManager is in charge of creating and maintaining the value in this file.

    ~ContainerManager();

    Container* requestNew(std::string key, std::string clientName, Connection* c);

    //self-registration functions for containers to call when they are created.
    void registerContainer(Container*);
    void deregisterContainer(Container*);
    Container* findContainer(QWebFrame*); //find a container that exists within a QWebFrame object.
private:
    KeyList* topLevelKeyList;
    std::string keyFilePath;

    std::list<Container*> extantContainers;
    //containers self-register and deregister themselves so this list reflects all containers currently around.

    void makeNewTopLevelKeyFile();
};

#endif // CONTAINERMANAGER_H
