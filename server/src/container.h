/*  Copyright (c) 2016-2018 Daniel Kos, General Development Systems

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

#ifndef CLIENTCONTAINER_H
#define CLIENTCONTAINER_H

#include <QWebElement>
#include <QWebFrame>
#include <QObject>
#include <stack>
#include <list>
#include <map>
#include <string>
#include "ExpArray.hh"
#include "common.h"
#include "keylist.h"
class Client;
class Connection;
class ContainerManager;


class FrameData {
//stores metadata and references relevant to a particular child container.
public:
    QWebElement we; //reference to the <iframe> tag
    QWebFrame* wf; //reference to the frame object which matches the <iframe> tag.
    QString hostkey; //once claimed, this becomes an empty string.

    uint64_t requestor; //to be attached when sending back child frame events.
    std::string clientName;
    std::string title;
    pid_t pid; //client process ID. pid_t defined in <types.h>

    std::string fg, bg; //foreground and background colours as CSS values.

    bool operator==(const FrameData& other);
};


class Container : public QObject
{
    Q_OBJECT
public:
    friend class ContainerManager;
    static std::string globalStyleRules;

    Container(Connection* client, QString clientName);
    ~Container();

    void receiveInstruction(hipe_instruction instruction);

    virtual void setTitle(std::string newTitle)=0;
    virtual void setIcon(const char* imgData, size_t length)=0;
    virtual void setBody(std::string newBodyHtml, bool overwrite=true)=0;
    virtual void applyStylesheet()=0; //apply stylesheet after changes. If <body> was not opened yet (!initYet)
    //then this is a no-op as styling gets applied when setBody is called. Otherwise, this call causes a <style>
    //tag to be appended inside the <body> tag; which is not strictly valid practice but should do the trick.

    void containerClosed();

    Container* requestNew(std::string key, std::string clientName, uint64_t pid, Connection* c);
    //request a new sub-frame that is managed by this container. Returns nullptr if the
    //key is not held by this container, else creates a new ContainerFrame object and
    //returns its pointer.

    void receiveSubFrameEvent(short evtType, QWebFrame* sender, std::string detail);
    //called by a sub-frame (ContainerFrame object) of this container to indicate that
    //the frame has been modified in a way that should be reported to the framing client.

    void receiveMessage(char opcode, int64_t requestor, std::string arg1, std::string arg2, QWebFrame* sender, bool propagateToParent=false);
    //called by another container object to transmit an instruction (e.g. HIPE_OP_MESSAGE)
    //from a direct parent/child frame's client to this container's client.
    //sender should be used only if sender is a child of the recipient. If it's the parent, this should be indicated
    //by passing a nullptr.
    //This can also be used to transmit events across frame boundaries (the parent will see the event as originating
    //from the client frame element.

    void keyEventOnChildFrame(QWebFrame* origin, bool keyUp, QString keycode);
    //if keyup is false, it was a keydown event.
    //This function is called from a child container instructing this container that a keyup/keydown event has
    //occurred on the body element of this frame (or has propagated from a child frame of *that* frame).
    //The event should be propagated up to the top level so the framing manager can intercept global keyboard shortcuts.
    //It should also trigger a simulated event on the frame to this client, if this client has bound onkeydown/onkeyup
    //attributes to this frame.

    virtual Container* getParent()=0; //returns the parent container, or nullptr if it's a top level container.

protected:
    Connection* client;
    QWebElement webElement;
    QWebFrame* frame;
    std::string stylesheet; //build up the stylesheet before we apply it.

signals:
    void receiveGuiEvent(quint64 location, quint64 requestor, QString event, QString detail);
    //signal called from within the QWebView object (via Javascript), each time a user interaction takes place.

    void receiveKeyEventOnBody(bool keyUp, QString keycode);
    //signal called when a keyup (or else keydown) event happens on the body element.
protected slots:
    void _receiveGuiEvent(quint64 location, quint64 requestor, QString event, QString detail);
    void _receiveKeyEventOnBody(bool keyUp, QString keycode);
    void frameCleared();
    void frameDestroyed(); //conneected to the QWebFrame's destroyed() signal.
public slots:
private:
    ///This block of variables/functions provides pointer protection of web element references so a handle
    ///(array element number) can safely be given to external processes, and invalid references can be detected.
    AutoExpArray<QWebElement*> referenceableElement;
    //when an element is created, we store its location as an element number here, then share the element number
    //(not a QWebElement--dangerous!) with the client.
    size_t addReferenceableElement(const QWebElement&);
    size_t firstFreeElementAfter=1; //store the location of the first free element, or a smaller element number, to speed insertions.
    void removeReferenceableElement(size_t);
    QWebElement getReferenceableElement(size_t); //resolve a reference integer.
    size_t findReferenceableElement(QWebElement);
    size_t getIndexOfElement(QWebElement); //finds corresponding index, or adds it if it has not been allocated an index yet.

    //flags to handle keyup/down events on body as a special case (since this event needs to propagate to the framing manager for special window manipulation keys)
    bool reportKeydownOnBody=false; //has the client requested keydown events on the body element?
    bool reportKeyupOnBody=false; //has the client requested keyup events on the body element?
    uint64_t keyDownOnBodyRequestor=0; //corresponding requestors if the client requests these key events.
    uint64_t keyUpOnBodyRequestor=0;

    KeyList* keyList;
    std::list<FrameData> subFrames; //table of subframes, mapping web element of an iframe to its corresponding child frame object and host-key (if assigned).


};

#endif // CLIENTCONTAINER_H
