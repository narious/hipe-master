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

#ifndef CLIENTCONTAINER_H
#define CLIENTCONTAINER_H

#include <QWebElement>
#include <QWebFrame>
#include <QObject>
#include <stack>
#include <list>
#include <map>
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

    bool operator==(const FrameData& other);
};


class Container : public QObject
{
    Q_OBJECT
public:
    friend class ContainerManager;

    Container(Connection* client, QString clientName);
    ~Container();

    void receiveInstruction(hipe_instruction instruction);

    virtual void setTitle(QString newTitle)=0;
    virtual void setIcon(const char* imgData, size_t length)=0;
    virtual void setBody(QString newBodyHtml, bool overwrite=true)=0;

    void containerClosed();

    Container* requestNew(std::string key, std::string clientName, Connection* c);
    //request a new sub-frame that is managed by this container. Returns nullptr if the
    //key is not held by this container, else creates a new ContainerFrame object and
    //returns its pointer.

    void receiveSubFrameEvent(short evtType, QWebFrame* sender, std::string detail);
    //called by a sub-frame (ContainerFrame object) of this container to indicate that
    //the frame has been modified in a way that should be reported to the framing client.

    void receiveMessage(int64_t requestor, std::string arg1, std::string arg2, QWebFrame* sender);
    //called by another container object to transmit a HIPE_OPCODE_MESSAGE instruction
    //from a direct parent/child frame's client to this container's client.
    //sender should be used only if sender is a child of the recipient. If it's the parent, this should be indicated
    //by passing a nullptr.

    virtual Container* getParent()=0; //returns the parent container, or nullptr if it's a top level container.

protected:
    Connection* client;
    QWebElement webElement;
    QWebFrame* frame;
    QString stylesheet; //build up the stylesheet before we apply it.
signals:
    void receiveGuiEvent(quint64 location, quint64 requestor, QString event, QString detail);
    //signal called from within the QWebView object (via Javascript), each time a user interaction takes place.
protected slots:
    void _receiveGuiEvent(quint64 location, quint64 requestor, QString event, QString detail);
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



    KeyList* keyList;
    std::list<FrameData> subFrames; //table of subframes, mapping web element of an iframe to its corresponding child frame object and host-key (if assigned).


};

#endif // CLIENTCONTAINER_H
