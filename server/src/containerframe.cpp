/*  Copyright (c) 2016 Daniel Kos, General Development Systems

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

//ContainerFrame is an alternative version of ContainerTopLevel, that exists within an iframe
//of another.

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

Container* ContainerFrame::getParent()
{
    return parent;
}

void ContainerFrame::setBody(QString newBodyHtml, bool overwrite)
{
    if(!initYet) {
        frame->setHtml(QString("<html><head><style>") + stylesheet + "</style><script>var canvascontext;</script></head><body onkeydown=\"c.receiveKeyEventOnBody(false, event.which);\" onkeyup=\"c.receiveKeyEventOnBody(true, event.which);\"></body></html>");
        webElement = frame->documentElement().lastChild();
        initYet = true;

        //alert parent of fg/bg colour scheme
        QString fg, bg;
        while(!fg.size())  //the frame might not be rendered straight away; during this time these will return blank strings.
            fg = webElement.styleProperty("color", QWebElement::ComputedStyle);
        while(!bg.size())
            bg = webElement.styleProperty("background-color", QWebElement::ComputedStyle);

        getParent()->receiveSubFrameEvent(HIPE_FRAME_EVENT_BACKGROUND_CHANGED, frame, bg.toStdString());
        getParent()->receiveSubFrameEvent(HIPE_FRAME_EVENT_COLOR_CHANGED, frame, fg.toStdString());
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
