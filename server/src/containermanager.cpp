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

#include "containermanager.h"
#include "containertoplevel.h"
#include <sstream> //for std::stringstream
#include <fstream> //for std::ofstream

ContainerManager* globalContainerManager; //pointer to the one and only ContainerManager instance created in main().
QDesktopWidget* desktop;

ContainerManager::ContainerManager(std::string keyFilePath)
{
    topLevelKeyList = new KeyList("H");
    this->keyFilePath = keyFilePath;
    makeNewTopLevelKeyFile();
}

ContainerManager::~ContainerManager()
{
    delete topLevelKeyList;
}

Container* ContainerManager::requestNew(std::string key, std::string clientName, Connection* c)
//returns nullptr if request denied.
{
    if(topLevelKeyList->claimKey(key)) {
        makeNewTopLevelKeyFile();
        Container* container = new ContainerTopLevel(c, QString::fromUtf8(clientName.c_str()));
        container->setTitle(QString::fromUtf8(clientName.c_str()));
        return container;
    } else { //not top-level. Traverse each container in case the key refers to a sub-frame.
        for(Container* container : extantContainers) {
            Container* newContainer = container->requestNew(key, clientName, c);
            if(newContainer)
                return newContainer;
        }
    }
    return nullptr;
}

void ContainerManager::registerContainer(Container* c)
{
    extantContainers.push_back(c);
}

void ContainerManager::deregisterContainer(Container* c)
{
    extantContainers.remove(c);
}

Container* ContainerManager::findContainer(QWebFrame* frame)
{
    for(Container* c : extantContainers) {
        if(c->frame == frame)
            return c;
    }
    return nullptr;
}

void ContainerManager::makeNewTopLevelKeyFile()
{
    //create file. Store random key in it.
    std::ofstream keyfile(keyFilePath);
    keyfile << topLevelKeyList->generateContainerKey();
    keyfile.close();
}
