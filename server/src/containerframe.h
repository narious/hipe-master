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

#ifndef CLIENTFRAME_H
#define CLIENTFRAME_H

#include "container.h"

class ContainerFrame : public Container {

//Lifetime:
//Created when a client is granted a connection to an existing iframe with a valid hostkey as determined
//by the parent container.

public:
    ContainerFrame(Connection* client, QString clientName, QWebFrame* frame, Container* parent); //object is not created until an iframe is actually assigned to a client.
    ~ContainerFrame();

    Container* getParent();

    void setBody(QString newBodyHtml, bool overwrite=true);
protected:
    void setTitle(QString newTitle);
    void setIcon(const char* imgData, size_t length);
private:
    bool initYet;
    Container* parent;
};

#endif // CLIENTFRAME_H
