/*  Copyright (c) 2016-2019 Daniel Kos, General Development Systems

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

ContainerFrame::ContainerFrame(Connection* bridge, std::string clientName, QWebFrame* frame, Container* parent) : Container(bridge, clientName) {
    initYet = false;

    //Disable network access and link navigation:
    frame->page()->networkAccessManager()->setNetworkAccessible(QNetworkAccessManager::NotAccessible);
    frame->page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);

    this->frame = frame;
    this->parent = parent;
    connect(frame, SIGNAL(javaScriptWindowObjectCleared()), this, SLOT(frameCleared()));
    connect(frame, SIGNAL(destroyed()), this, SLOT(frameDestroyed()));
}

ContainerFrame::~ContainerFrame()
{
    if(frame) frame->setHtml(""); //clear the frame's contents if it still exists.
    if(parent)
        parent->receiveSubFrameEvent(HIPE_FRAME_EVENT_CLIENT_DISCONNECTED, frame, "");
}

Container* ContainerFrame::getParent()
{
    return parent;
}

void ContainerFrame::setBody(std::string newBodyHtml, bool overwrite)
{
    if(!parent) return;
    if(!initYet) {
        frame->setHtml(QString("<html><head><style>") + stylesheet.c_str() + "</style><script>var canvascontext;</script></head><body onkeydown=\"c.receiveKeyEventOnBody(false, event.which);\" onkeyup=\"c.receiveKeyEventOnBody(true, event.which);\"></body></html>");
        stylesheet = ""; //clear already-applied stylesheet data.
        webElement = frame->documentElement().lastChild();
        initYet = true;

        //alert parent of fg/bg colour scheme
        std::string fg, bg;
        while(!fg.size())  //the frame might not be rendered straight away; during this time these will return blank strings.
            fg = webElement.styleProperty("color", QWebElement::ComputedStyle).toStdString();
        while(!bg.size())
            bg = webElement.styleProperty("background-color", QWebElement::ComputedStyle).toStdString();

        getParent()->receiveSubFrameEvent(HIPE_FRAME_EVENT_BACKGROUND_CHANGED, frame, bg);
        getParent()->receiveSubFrameEvent(HIPE_FRAME_EVENT_COLOR_CHANGED, frame, fg);
    }
    if(overwrite) webElement.setInnerXml(newBodyHtml.c_str());
    else webElement.appendInside(newBodyHtml.c_str()); //c_str() conversion is adequate since any binary data will be in safe base64 encoding.
}

void ContainerFrame::setTitle(std::string newTitle)
{
    if(!parent) return;
    parent->receiveSubFrameEvent(HIPE_FRAME_EVENT_TITLE_CHANGED, frame, newTitle);
}

void ContainerFrame::setIcon(const char* imgData, size_t length)
{
    if(!parent) return;
    parent->receiveSubFrameEvent(HIPE_FRAME_EVENT_ICON_CHANGED, frame, std::string(imgData, length));
}
