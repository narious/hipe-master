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

#include "container.h"
#include "connection.h"
#include "containerframe.h"
#include "sanitation.h"

#include <QWebFrame>
#include <QWebPage>
#include <QPrinter>
#include <stdio.h>

//INCOMING REQUESTS FROM CLIENTTHREAD (must come from there)
// - requestor: arbitrary pointer to the entity on the client end that requires the action.
// - target: a pointer to the GUI markup element we wish to manipulate.
// - action: enumerated opcode. e.g. ACT_MAKEWIDGET.
// - arg1, arg2: optional string arguments


Container::Container(Connection* bridge, QString clientName) : QObject()
{
    this->client = bridge;

    keyList = new KeyList(clientName.toStdString());

    connect(this, SIGNAL(receiveGuiEvent(quint64,quint64,QString,QString)),
            this, SLOT(_receiveGuiEvent(quint64,quint64,QString,QString)));

    globalContainerManager->registerContainer(this);
}

Container::~Container()
{
    globalContainerManager->deregisterContainer(this);
}

void Container::receiveInstruction(hipe_instruction instruction)
{
    QString arg1 = QString::fromUtf8(instruction.arg1, instruction.arg1Length);
    QString arg2 = QString::fromUtf8(instruction.arg2, instruction.arg2Length);
    uint64_t requestor = instruction.requestor;
    bool locationSpecified = (bool) instruction.location;
    QWebElement location = locationSpecified ? getReferenceableElement(instruction.location)
                                             : webElement;


    if(instruction.opcode == HIPE_OPCODE_CLEAR) {
        if(!locationSpecified) setBody("",true);
        else location.setInnerXml("");
    } else if(instruction.opcode == HIPE_OPCODE_APPEND_TAG) {
        arg1 = Sanitation::sanitisePlainText(arg1);
        arg2 = Sanitation::sanitisePlainText(arg2);
        if(Sanitation::isAllowedTag(arg1)) { //eliminate forbidden tags.
            QString newTagString = "<";
            newTagString += arg1;
            if(arg2.size()) {
                newTagString += " id=\"" + arg2 + "\"";
            } else if(arg1 == "iframe" || arg1 == "canvas") { //these tags don't function properly without an ID. Make a random one.
                std::string randomID = keyList->generateContainerKey();
                keyList->claimKey(randomID); //burn through a contaner key in order to get a random string out of it.
                newTagString += " id=\"";
                newTagString += randomID.c_str();
                newTagString += "\"";
            }
            newTagString += "></" + arg1 + ">";
            if(!locationSpecified) setBody(newTagString, false);
            else location.appendInside(newTagString);
        }
    } else if(instruction.opcode == HIPE_OPCODE_SET_TEXT) {
        arg1 = Sanitation::sanitisePlainText(arg1);
        if(!locationSpecified) setBody(arg1);
        else location.setInnerXml(arg1);
    } else if(instruction.opcode == HIPE_OPCODE_APPEND_TEXT) {
        arg1 = Sanitation::sanitisePlainText(arg1);
        if(!locationSpecified) setBody(arg1, false);
        else location.appendInside(arg1);
    } else if(instruction.opcode == HIPE_OPCODE_ADD_STYLE_RULE) {
        if(Sanitation::isAllowedCSS(arg1) && Sanitation::isAllowedCSS(arg2))
            stylesheet += arg1 + "{" + arg2 + "}\n";
    } else if(instruction.opcode == HIPE_OPCODE_SET_TITLE) {
        setTitle(arg1);
    } else if(instruction.opcode == HIPE_OPCODE_GET_FIRST_CHILD) {
        //answer the location request.
        client->sendInstruction(HIPE_OPCODE_LOCATION_RETURN, instruction.requestor,
                                getIndexOfElement(location.firstChild()), "", "");
    } else if(instruction.opcode == HIPE_OPCODE_GET_LAST_CHILD) {
        //answer the location request.
        client->sendInstruction(HIPE_OPCODE_LOCATION_RETURN, instruction.requestor,
                                getIndexOfElement(location.lastChild()), "", "");
    } else if(instruction.opcode == HIPE_OPCODE_GET_NEXT_SIBLING) {
        //answer the location request.
        client->sendInstruction(HIPE_OPCODE_LOCATION_RETURN, instruction.requestor,
                                getIndexOfElement(location.nextSibling()), "", "");
    } else if(instruction.opcode == HIPE_OPCODE_GET_PREV_SIBLING) {
        //answer the location request.
        client->sendInstruction(HIPE_OPCODE_LOCATION_RETURN, instruction.requestor,
                                getIndexOfElement(location.previousSibling()), "", "");
    } else if(instruction.opcode == HIPE_OPCODE_GET_BY_ID) {
        client->sendInstruction(HIPE_OPCODE_LOCATION_RETURN, instruction.requestor,
                                getIndexOfElement(webElement.findFirst(QString("#") + arg1)), "", "");
    } else if(instruction.opcode == HIPE_OPCODE_SET_ATTRIBUTE) {
        if(Sanitation::isAllowedAttribute(arg1))
            location.setAttribute(arg1, Sanitation::sanitisePlainText(arg2));
    } else if(instruction.opcode == HIPE_OPCODE_SET_STYLE) {
        if(Sanitation::isAllowedCSS(arg1) && Sanitation::isAllowedCSS(arg2)) {
            if(!locationSpecified) { //we need to be sure the body has been initialised first.
                if(webElement.isNull())
                    setBody("");
                webElement.setStyleProperty(arg1, arg2);
            } else {
                location.setStyleProperty(arg1, arg2);
            }
        }
    } else if(instruction.opcode == HIPE_OPCODE_FREE_LOCATION) {
        removeReferenceableElement(instruction.location);
    } else if(instruction.opcode == HIPE_OPCODE_EVENT_REQUEST) {
        QString locStr = QString::number(instruction.location);
        QString reqStr = QString::number(requestor);
        QString evtDetailArgs;
        if(arg1 == "mousemove" || arg1 == "mousedown" || arg1 == "mouseup")
            evtDetailArgs = "'' + event.which + ',' + event.pageX + ',' + event.pageY";
        else
            evtDetailArgs = "event.which";
        location.setAttribute(QString("on") + arg1, QString("c.receiveGuiEvent(") + locStr + "," + reqStr + ",'" + arg1 + "'," + evtDetailArgs + ")");
    } else if(instruction.opcode == HIPE_OPCODE_EVENT_CANCEL) {
        location.removeAttribute(QString("on") + arg1);
    } else if(instruction.opcode == HIPE_OPCODE_GET_GEOMETRY) {
        QString left = location.evaluateJavaScript("this.offsetLeft;").toString();
        QString top = location.evaluateJavaScript("this.offsetTop;").toString();
        if(arg1.at(0) != '1') { //get position
            client->sendInstruction(HIPE_OPCODE_POSITION_RETURN, instruction.requestor, instruction.location,
                                    left.toStdString(), top.toStdString());
        } else { //get size
            QString width = location.evaluateJavaScript("this.offsetWidth;").toString();
            QString height = location.evaluateJavaScript("this.offsetHeight;").toString();
            client->sendInstruction(HIPE_OPCODE_SIZE_RETURN, instruction.requestor, instruction.location,
                                    width.toStdString(), height.toStdString());
        }
    } else if(instruction.opcode == HIPE_OPCODE_GET_ATTRIBUTE) {
        client->sendInstruction(HIPE_OPCODE_ATTRIBUTE_RETURN, instruction.requestor, instruction.location,
                                arg1.toStdString(), location.attribute(arg1).toStdString());
    } else if(instruction.opcode == HIPE_OPCODE_SET_SRC) {
        QString dataURI = QString("data:") + arg1 + ";base64,";

        QByteArray b64Data = QByteArray(instruction.arg2, instruction.arg2Length).toBase64();

        dataURI += QString::fromLocal8Bit(b64Data);
        location.setAttribute("src", dataURI);
    } else if(instruction.opcode == HIPE_OPCODE_SET_BACKGROUND_SRC) {
        QString dataURI = QString("data:") + arg1 + ";base64,";
        QByteArray b64Data = QByteArray(instruction.arg2, instruction.arg2Length).toBase64();
        dataURI += QString::fromLocal8Bit(b64Data);
        location.setStyleProperty("background-image", QString("url(\"") + dataURI + "\")");
    } else if(instruction.opcode == HIPE_OPCODE_ADD_STYLE_RULE_SRC) {
        QString dataURI = QString("data:image/png;base64,");
        QByteArray b64Data = QByteArray(instruction.arg2, instruction.arg2Length).toBase64();
        dataURI += QString::fromLocal8Bit(b64Data);
        stylesheet += arg1 + "{background-image:url(\"" + dataURI + "\");}\n";
    } else if(instruction.opcode == HIPE_OPCODE_GET_FRAME_KEY) {
        //Check if the location is already represented in the frame table.
        QString frameID = location.attribute("id"); //Need this for matching the frame.
        bool found = false;
        QString hostKey = "";
        for(FrameData& fd : subFrames) {
            if(fd.we == location) { //found
                found = true;
                hostKey = fd.hostkey;
                break;
            }
        }

        //If not, we need to match the unique ID of the iframe DOM element to its corresponding QWebFrame.
        if(!found) {
            hostKey = keyList->generateContainerKey().c_str();

            //now traverse child frames to find the one with the same ID.
            auto frames = webElement.webFrame()->childFrames();
            for(QWebFrame* frame : frames) {
                if(frame->frameName() == frameID) {
                    found = true; //match found.
                    subFrames.push_back({location, frame, hostKey, requestor, "", ""}); //add new entry to the table.
                    break;
                }
            }
        }
        if(frameID.size()) location.setAttribute("id", frameID); //restore any previous tag id.
        else location.removeAttribute("id");

        //return the host key to the client if element was found, else return blank string.
        client->sendInstruction(HIPE_OPCODE_KEY_RETURN, instruction.requestor, instruction.location, found ? hostKey.toStdString() : "", "");
    } else if(instruction.opcode == HIPE_OPCODE_FRAME_CLOSE) {
        //find the relevant client
        for(FrameData& fd : subFrames) {
            if(fd.we == location) { //found
                Container* target = globalContainerManager->findContainer(fd.wf); //find the corresponding container.
                if(target) {
                    if(!arg1.size() || arg1[0] == '\0')
                        target->containerClosed(); //soft close request.
                    else
                        delete target->client; //hard disconnection
                }
                break;
            }
        }
    } else if(instruction.opcode == HIPE_OPCODE_TOGGLE_CLASS) {
        location.toggleClass(arg1);
    } else if(instruction.opcode == HIPE_OPCODE_SET_FOCUS) {
        location.setFocus();
    } else if(instruction.opcode == HIPE_OPCODE_TAKE_SNAPSHOT) {
        if(arg1.toLower() == "pdf") { //vector screenshot.
            QPrinter pdfGen(QPrinter::ScreenResolution);
            pdfGen.setOutputFormat(QPrinter::PdfFormat);
            pdfGen.setFontEmbeddingEnabled(true);
            pdfGen.setFullPage(true);
            //scale paper size to whichever resolution the QPrinter object is using:
            pdfGen.setPaperSize(QSizeF(webElement.webFrame()->contentsSize().width()/pdfGen.resolution(), webElement.webFrame()->contentsSize().height()/pdfGen.resolution()), QPrinter::Inch);
            QString snapshotFile = QString("/tmp/hipe-uid") + uid.c_str() + "_snapshot.pdf";
            pdfGen.setOutputFileName(snapshotFile);
            webElement.webFrame()->print(&pdfGen);

            //We now have the screenshot in a temporary file. We need to get that file's contents and send
            //it back to the client.
            FILE* ssFile = fopen(snapshotFile.toStdString().c_str(), "r");
            bool success = true;
            char* fData = 0;
            size_t size;
            if(ssFile) { //successfully opened
                fseek(ssFile, 0, SEEK_END); //determine file size
                size = ftell(ssFile);
                rewind(ssFile);
                fData = (char*) malloc(size);
                size_t result = fread(fData, 1, size, ssFile);
                if(result != size) success = false;
                fclose(ssFile);
            } else {
                success = false;
            }

            //send the file and/or error state to the client.
            hipe_instruction payload;
            hipe_instruction_init(&payload);
            payload.opcode = HIPE_OPCODE_FILE_RETURN;
            payload.requestor = instruction.requestor;
            payload.location = instruction.location;
            if(success) {
                payload.arg1 = fData;
                payload.arg1Length = size;
                payload.arg2 = 0; payload.arg2Length = 0;
            } else {
                payload.arg1 = 0; payload.arg1Length = 0;
                payload.arg2 = (char*) "File error.";
                payload.arg2Length = 11;
            }

            client->sendInstruction(payload);
            free(fData);
            remove(snapshotFile.toStdString().c_str()); //delete the temporary file.
        }
    } else if(instruction.opcode == HIPE_OPCODE_USE_CANVAS) {
        webElement.evaluateJavaScript(QString("canvascontext=document.getElementById(\"")
                                      + location.attribute("id") + "\").getContext(\"" + arg1 + "\");");
        //TODO: sanitise arg1 against e.g. quotation marks.
        //Don't allow parentheses or semicolons.
    } else if(instruction.opcode == HIPE_OPCODE_CANVAS_ACTION) {
        webElement.evaluateJavaScript(QString("canvascontext.") + arg1 + "(" + arg2 + ");");
        //TODO sanitise arg1 and arg2 against javascript injections.
        //Don't allow parentheses or semicolons.
    } else if(instruction.opcode == HIPE_OPCODE_CANVAS_SET_PROPERTY) {
        webElement.evaluateJavaScript(QString("canvascontext.") + arg1 + "=" + arg2 + ";");
        //TODO sanitise arg1 and arg2 against javascript injections.
        //Don't allow parentheses, semicolons, etc.
    } else if(instruction.opcode == HIPE_OPCODE_SET_ICON) {

        //TODO: implement this instruction.
    } else if(instruction.opcode == HIPE_OPCODE_REMOVE_ATTRIBUTE) {
        if(Sanitation::isAllowedAttribute(arg1))
            location.removeAttribute(arg1);
    } else if(instruction.opcode == HIPE_OPCODE_MESSAGE) {
        //Determine whether we need to send the message to the parent frame or a child frame.
        Container* target = nullptr;
        QWebFrame* sourceframe = nullptr;
        if(locationSpecified) {
            //find the relevant child frame client
            for(FrameData& fd : subFrames) {
                if(fd.we == location) { //found
                    target = globalContainerManager->findContainer(fd.wf); //find the corresponding container.
                    break;
                }
            }
        } else { //send to parent element
            target = getParent();
            sourceframe = webElement.webFrame();
        }
        if(target) { //send the instruction to the destination.
            target->receiveMessage(requestor, std::string(instruction.arg1,instruction.arg1Length), std::string(instruction.arg2, instruction.arg2Length), sourceframe);
        }
    }
}

void Container::containerClosed()
//Called when the container is requested to be closed by the user.
//We need to disconnect the connection to the client and free all the associated resources of this instance.
//TODO: we need to provide an alternative mechanism of sending an event to the client so the client
//can handle the close event instead (e.g. by asking for confirmation).
{
    client->deleteLater();
}

Container* Container::requestNew(std::string key, std::string clientName, Connection* c) {
    if(keyList->claimKey(key)) {
        //find the relevant frame in the subFrames list.
        for(FrameData& fd : subFrames) {
            if(fd.hostkey.toStdString() == key) { //found it.
                fd.hostkey = ""; //now claimed, not reusable.
                fd.clientName = clientName;
                fd.title = clientName;
                receiveSubFrameEvent(HIPE_FRAME_EVENT_CLIENT_CONNECTED, fd.wf, clientName);
                return (Container*) new ContainerFrame(c, QString(clientName.c_str()), fd.wf, this);
            }
        }
    }
    return nullptr;
}

void Container::receiveSubFrameEvent(short evtType, QWebFrame* sender, std::string detail)
{
    std::string evtTypeString = " "; evtTypeString[0] = (char) evtType;

    //find the sender in subFrames list...
    for(FrameData& sf : subFrames) {
        if(sf.wf == sender) { //found it.
            if(evtType == HIPE_FRAME_EVENT_TITLE_CHANGED)
                sf.title = detail;

            client->sendInstruction(HIPE_OPCODE_FRAME_EVENT, sf.requestor, findReferenceableElement(sf.we),
                                    evtTypeString, detail);

            if(evtType == HIPE_FRAME_EVENT_CLIENT_DISCONNECTED)
                subFrames.remove(sf);

            break;
        }
    }
}

void Container::receiveMessage(int64_t requestor, std::string arg1, std::string arg2, QWebFrame* sender) {
//If the sender is the parent of this frame, a nullptr should be passed as sender.
//If the sender is a child frame, we'll resolve the child frame's location from the perspective of this frame.

    size_t location = 0;

    //if it came from the parent we send a 0 for location. Otherwise we need to identify the child frame it came from.
    if(sender) //need to resolve location of child frame that sent this.
        for(FrameData& sf : subFrames) {
            if(sf.wf == sender) { //found it.
                location = getIndexOfElement(sf.we);
            }
        }

    client->sendInstruction(HIPE_OPCODE_MESSAGE, requestor, location, arg1, arg2);
}


void Container::_receiveGuiEvent(quint64 location, quint64 requestor, QString event, QString detail)
{
    client->sendInstruction(HIPE_OPCODE_EVENT, requestor, location, event.toStdString(), detail.toStdString());
}

void Container::frameCleared() {
    frame->addToJavaScriptWindowObject("c", this);
    //From Qt website: "If you want to ensure that your QObjects remain accessible after loading a new URL, you should add them in a slot connected to the javaScriptWindowObjectCleared() signal."
}

void Container::frameDestroyed()
{
    frame = nullptr;
    delete client;
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
    //no space available. Grow the array.
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

    //TODO: shrink the array if the last element was removed.
}

QWebElement Container::getReferenceableElement(size_t i)
{
    if(i>referenceableElement.size() || !referenceableElement[i]) return QWebElement(); //todo:terminate the client instead! Like with a general protection fault.
    else if(i == 0) return QWebElement(); //there is no zeroth element. Reserve it to mean 'not applicable'.
    else return *referenceableElement[i];
}

size_t Container::findReferenceableElement(QWebElement we) {
    if(we.isNull()) return 0;
    for(size_t i=referenceableElement.size(); i>=1; i--) { //count down so that the last-added reference is more likely to be checked first.
        if(referenceableElement[i] && *(referenceableElement[i])==we)
            return i;
    }
    return 0;
}

size_t Container::getIndexOfElement(QWebElement location)
{
    size_t locElement = 0;
    if(!location.isNull()) {
        locElement = findReferenceableElement(location); //check if the element is already on the list
        if(!locElement) locElement = addReferenceableElement(location); //if not, add it.
    }
    return locElement;
}


bool FrameData::operator==(const FrameData& other) {
    return (this->we == other.we);
}
