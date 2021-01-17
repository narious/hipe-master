/*  Copyright (c) 2016-2021 Daniel Kos, General Development Systems

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

#include "container.h"
#include "connection.h"
#include "containerframe.h"
#include "containertoplevel.h"
#include "sanitation.h"
#include "main.hpp"
#include "instructionhandler.h"

#include <QWebFrame>
#include <QWebPage>
#include <stdio.h>

#include <iostream>

std::string Container::globalStyleRules="";

Container::Container(Connection* bridge, std::string clientName) : QObject()
{
    this->client = bridge;

    keyList = new KeyList(clientName);

    connect(this, SIGNAL(receiveGuiEvent(QString,QString,QString,QString)),
            this, SLOT(_receiveGuiEvent(QString,QString,QString,QString)));

    connect(this, SIGNAL(receiveKeyEventOnBody(bool,QString)),
            this, SLOT(_receiveKeyEventOnBody(bool,QString)));
    //keyup/keydown events on the body element are treated as a special case,
    //since they might need to be propagated to a parent tag.

    stylesheet = globalStyleRules.c_str();
    //initialise our stylesheet rules to any global rules that have been loaded
    //in from a CSS file.

    stylesheet += " ";
}

Container::~Container()
{
    delete keyList;
}

void Container::applyStylesheet() {

    if(!initYet) return;
    //no-op. Styles will be applied in the <head> when setBody is called.

    //appending new style rules after </head> is not supposed to be valid, but we might get away with it.
    webElement.appendInside(QString("<style>") + stylesheet.c_str() + "</style>");
    stylesheet = ""; //clear after application.
}


void Container::receiveInstruction(hipe_instruction instruction)
//POLICY NOTES: Qt's webkit DOM functions require QStrings extensively. However
//we prefer to avoid coupling our application too closely to Qt due to the
//Qt Company's neglect of webkit bindings.
//THEREFORE, use C++11 standard types where possible/efficient to do so,
//and only convert to QString type where it is necessary to do so.
{

    //uint64_t requestor = instruction.requestor;
    bool locationSpecified = (bool) instruction.location;
    //if location not specified, set it to the body element of the container.
    QWebElement location = locationSpecified ? getReferenceableElement(instruction.location)
                                             : webElement; //may need to update this after calling setBody!!

    invoke_handler(this, &instruction, locationSpecified, location);
    return;
}

void Container::containerClosed()
//Called when the container is requested to be closed by the user.
//(We need to disconnect the connection to the client and free all the
//associated resources of this instance.)
//This function sends a message to the client to request disconnection at the
//client's end. The client needs to check for this message and deal with it.
{
    //client->deleteLater();
    client->sendInstruction(HIPE_OP_FRAME_CLOSE, 0, 0);
}

Container* Container::requestNew(std::string key, std::string clientName, uint64_t pid, Connection* c) {
    if(keyList->claimKey(key)) {
        //find the relevant frame in the subFrames list.
        for(FrameData& fd : subFrames) {
            if(fd.hostkey.toStdString() == key) { //found it.
                fd.hostkey = ""; //now claimed, not reusable.
                fd.clientName = clientName;
                fd.title = clientName;
                fd.pid = pid;
                receiveSubFrameEvent(HIPE_FRAME_EVENT_CLIENT_CONNECTED, fd.wf, clientName);
                return (Container*) new ContainerFrame(c, clientName, fd.wf, this);
            }
        }
    }
    return nullptr;
}

void Container::receiveSubFrameEvent(short evtType, QWebFrame* sender, std::string detail)
//called from a sub-frame when an event affecting that subframe takes place.
{
    std::string evtTypeString = " "; evtTypeString[0] = (char) evtType;

    //find the sender in subFrames list...
    for(FrameData& sf : subFrames) {
        if(sf.wf == sender) { //found it.
            if(evtType == HIPE_FRAME_EVENT_TITLE_CHANGED)
                sf.title = detail;
            else if(evtType == HIPE_FRAME_EVENT_BACKGROUND_CHANGED) {
                if(detail.compare(sf.bg) != 0) { 
                //check frame metadata in sf, if background has not changed, return without sending event.
                    sf.bg = detail;
                } else
                    return; //no change to background colour.
            } else if(evtType == HIPE_FRAME_EVENT_COLOR_CHANGED) { //foreground colour
                if(detail.compare(sf.fg) != 0) { //similarly as for background colour
                    sf.fg = detail;
                } else
                    return; //no change to foreground colour.
            }

            if(evtType == HIPE_FRAME_EVENT_CLIENT_CONNECTED) //this event has an extra detail arg: the process ID.
                client->sendInstruction(HIPE_OP_FRAME_EVENT, sf.requestor, findReferenceableElement(sf.we),
                                    {evtTypeString, detail, std::to_string(sf.pid)});
            else
                client->sendInstruction(HIPE_OP_FRAME_EVENT, sf.requestor, findReferenceableElement(sf.we),
                                    {evtTypeString, detail});

            if(evtType == HIPE_FRAME_EVENT_CLIENT_DISCONNECTED)
                subFrames.remove(sf);

            break;
        }
    }
}

void Container::receiveMessage(char opcode, int64_t requestor, const std::vector<std::string>& args, QWebFrame* sender, bool propagateToParent) {
//If the sender is the parent of this frame, a nullptr should be passed as sender.
//If the sender is a child frame, we'll resolve the child frame's location from the perspective of this frame.
//If propagateToParent is set, all parents of this container will see this message as originating from their relevant child frame.

    size_t location = 0;

    //if it came from the parent we send a 0 for location. Otherwise we need to identify the child frame it came from.
    if(sender) //need to resolve location of child frame that sent this.
        for(FrameData& sf : subFrames) {
            if(sf.wf == sender) { //found it.
                location = getIndexOfElement(sf.we);
                break;
            }
        }

    client->sendInstruction(opcode, requestor, location, args);

    //propagate to parent (and grandparent, etc.) if flag specified.
    if(propagateToParent && getParent())
        getParent()->receiveMessage(opcode, requestor, args, webElement.webFrame(), true);
}

void Container::keyEventOnChildFrame(QWebFrame* origin, bool keyUp, QString keycode) {
//if keyup is false, it was a keydown event.
//This function is called from a child container instructing this container that a keyup/keydown event has
//occurred on the body element of this frame (or has propagated from a child frame of *that* frame).
//The event should be propagated up to the top level so the framing manager can intercept global keyboard shortcuts.
//It should also trigger a simulated event on the frame to this client, if this client has bound onkeydown/onkeyup
//attributes to this frame.

    size_t location=0;
    QWebElement childFrame;

    for(FrameData& sf : subFrames) {
        if(sf.wf == origin) { //found it.
            location = getIndexOfElement(sf.we);
            childFrame = sf.we;
            break;
        }
    }

    //Determine if an onkeydown/onkeyup attribute is attached to this element.
    //Fire off an event if so.
    if(keyUp && childFrame.hasAttribute("onkeyup"))
        client->sendInstruction(HIPE_OP_EVENT, 0 /*fixme: how can we find out the requestor?*/, location, {"keyup", keycode.toStdString()});
    else if(!keyUp && childFrame.hasAttribute("onkeydown"))
        client->sendInstruction(HIPE_OP_EVENT, 0 /*fixme: how can we find out the requestor?*/, location, {"keydown", keycode.toStdString()});

    if(getParent()) { //propagate this up to *our* parent and so on, in case they need this keyboard event.
        getParent()->keyEventOnChildFrame(webElement.webFrame(), keyUp, keycode);
    }

}


void Container::_receiveGuiEvent(QString location, QString requestor, QString event, QString detail)
//location and requestor are hexadecimal string representations of uint64_t values.
{
    uint64_t loc, rq;
    bool ok;
    loc = location.toULongLong(&ok, 16); //LongLong conversion seems necessary on some distros to prevent truncation.
    rq = requestor.toULongLong(&ok, 16);
    client->sendInstruction(HIPE_OP_EVENT, rq, loc, {event.toStdString(), detail.toStdString()});
}

void Container::_receiveKeyEventOnBody(bool keyUp, QString keycode)
//keyup and keydown events are treated as a special case when they happen on the body element.
//receiveGuiEvent is not called directly, instead this slot is ALWAYS called, since we want to receive
//the event and propagate it up the client tree regardless of whether the user has asked to be notified of it.
{
    if(keyUp && reportKeyupOnBody)
        _receiveGuiEvent("0", QString::number(keyUpOnBodyRequestor,16), "keyup", keycode);
    else if(!keyUp && reportKeydownOnBody)
        _receiveGuiEvent("0", QString::number(keyDownOnBodyRequestor,16), "keydown", keycode);

    // the whole point of this function is that we'll now notify the parent of the event.
    // if this frame has a onkeydown or onkeyup attribute specified in the parent, we'll fire off an event on that iframe.
    // Regardless, we then propagate to *that* element's parent as well.
    if(getParent()) { //propagate this up to *our* parent and so on, in case they need this keyboard event.
        getParent()->keyEventOnChildFrame(webElement.webFrame(), keyUp, keycode);
    }

}

void Container::frameCleared() {
    frame->addToJavaScriptWindowObject("c", this);
    //From Qt website: "If you want to ensure that your QObjects remain
    //accessible after loading a new URL, you should add them in a slot
    //connected to the javaScriptWindowObjectCleared() signal."
}

void Container::frameDestroyed()
{
    frame = nullptr;
    //delete client; //not here -causes 'circular' cleanup.
    client->disconnect();  //the connection will now be cleaned up in next cycle.
    //in turn, the defunct connection will destroy this container object.
}

size_t Container::addReferenceableElement(const QWebElement& w)
{
    for(size_t i=firstFreeElementAfter; i<=referenceableElement.size(); i++) {
        if(!referenceableElement[i]) {
            firstFreeElementAfter = i;
            referenceableElement[i] = new QWebElement(w);
            return i;
        }
    }
    //no space available. Grow the array by 1 and initialise the new element.
    size_t i = referenceableElement.size()+1;
    referenceableElement.setSize(i);
    firstFreeElementAfter = i;
    referenceableElement[i] = new QWebElement(w);
    return i;
}

void Container::removeReferenceableElement(size_t i)
{
    if(i>referenceableElement.size() || i<1) return;
    delete referenceableElement[i];
    referenceableElement[i] = nullptr;
    if(i < firstFreeElementAfter) firstFreeElementAfter = i;

    if(i==referenceableElement.size()) {
    //might be an opportunity to truncate the array if one or more elements
    //at the end are now vacant.
        while(!referenceableElement[i])
            i--;
        referenceableElement.setSize(i);
        //shrink the array if the last element was removed. (The underlying
        //class then optimises this to avoid too much actual resizing.)
    }

}

QWebElement Container::getReferenceableElement(size_t i)
{
    if(i>referenceableElement.size() || !referenceableElement[i]) {
        return QWebElement();
        //TODO:terminate the client instead! Like with a general protection fault.
    } else if(i == 0) {
        return QWebElement();
        //there is no zeroth element. Reserve it to mean 'not applicable'.
    } else {
        return *referenceableElement[i];
    }
}

size_t Container::findReferenceableElement(QWebElement we) {
    if(we.isNull()) return 0;
    for(size_t i=referenceableElement.size(); i>=1; i--) {
    //count down so that the last-added reference is more likely to be checked first.
        if(referenceableElement[i] && *(referenceableElement[i])==we)
            return i;
    }
    return 0;
}

size_t Container::getIndexOfElement(QWebElement location)
{
    size_t locElement = 0;
    if(!location.isNull()) {
        locElement = findReferenceableElement(location);
        //check if the element is already on the list

        if(!locElement) locElement = addReferenceableElement(location);
        //if not, add it.
    }
    return locElement;
}


bool FrameData::operator==(const FrameData& other) {
    return (this->we == other.we);
}
