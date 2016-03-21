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

#include "containerframe.h"
#include <QWebPage>

//clientframe is an alternative version of ClientWindow, that exists within an iframe
//of another

ContainerFrame::ContainerFrame(Connection* bridge, QString clientName, QWebFrame* frame, Container* parent) : Container(bridge, clientName) {
    initYet = false;
    this->frame = frame;
    this->parent = parent;
    connect(frame, SIGNAL(javaScriptWindowObjectCleared()), this, SLOT(frameCleared()));
    connect(frame, SIGNAL(destroyed()), this, SLOT(frameDestroyed()));
}

ContainerFrame::~ContainerFrame()
{
    if(frame) frame->setHtml(""); //clear the frame's contents if it still exists.
    parent->receiveSubFrameEvent(HIPE_FRAME_EVENT_CLIENT_DISCONNECTED, frame, "");
}

void ContainerFrame::setBody(QString newBodyHtml, bool overwrite)
{
    if(!initYet) {
        frame->setHtml(QString("<html><head><style>") + stylesheet + "</style><script>var canvascontext;</script></head><body></body></html>");
        webElement = frame->documentElement().lastChild();
        initYet = true;
    }
    if(overwrite) webElement.setInnerXml(newBodyHtml);
    else webElement.appendInside(newBodyHtml);
}

void ContainerFrame::setTitle(QString newTitle)
{
    parent->receiveSubFrameEvent(HIPE_FRAME_EVENT_TITLE_CHANGED, frame, newTitle.toStdString());
}

void ContainerFrame::setIcon(const char* imgData, size_t length)
{
    parent->receiveSubFrameEvent(HIPE_FRAME_EVENT_ICON_CHANGED, frame, std::string(imgData, length));
}
