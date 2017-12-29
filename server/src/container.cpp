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

#include "container.h"
#include "connection.h"
#include "containerframe.h"
#include "sanitation.h"

#include <QWebFrame>
#include <QWebPage>
#include <QPrinter>
#include <stdio.h>

std::string Container::globalStyleRules="";

Container::Container(Connection* bridge, QString clientName) : QObject()
{
    this->client = bridge;

    keyList = new KeyList(clientName.toStdString());

    connect(this, SIGNAL(receiveGuiEvent(quint64,quint64,QString,QString)),
            this, SLOT(_receiveGuiEvent(quint64,quint64,QString,QString)));

    connect(this, SIGNAL(receiveKeyEventOnBody(bool,QString)),
            this, SLOT(_receiveKeyEventOnBody(bool,QString)));
    //keyup/keydown events on the body element are treated as a special case,
    //since they might need to be propagated to a parent tag.

    globalContainerManager->registerContainer(this);

    stylesheet = globalStyleRules.c_str(); //initialise our stylesheet rules to any global rules that have been loaded in from a CSS file.
    stylesheet += " ";
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


    if(instruction.opcode == HIPE_OP_CLEAR) {
        if(!locationSpecified) setBody("",true);
        else location.setInnerXml("");
    } else if(instruction.opcode == HIPE_OP_APPEND_TAG) {
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
    } else if(instruction.opcode == HIPE_OP_SET_TEXT) {
        arg1 = Sanitation::sanitisePlainText(arg1, (bool)(arg2=="1"));
        if(!locationSpecified) setBody(arg1);
        else location.setInnerXml(arg1);
    } else if(instruction.opcode == HIPE_OP_APPEND_TEXT) {
        arg1 = Sanitation::sanitisePlainText(arg1, (bool)(arg2=="1"));
        if(!locationSpecified) setBody(arg1, false);
        else location.appendInside(arg1);
    } else if(instruction.opcode == HIPE_OP_ADD_STYLE_RULE) {
        if(Sanitation::isAllowedCSS(arg1) && Sanitation::isAllowedCSS(arg2))
            stylesheet += arg1 + "{" + arg2 + "}\n";
        applyStylesheet();
    } else if(instruction.opcode == HIPE_OP_SET_TITLE) {
        setTitle(arg1);
    } else if(instruction.opcode == HIPE_OP_GET_FIRST_CHILD) {
        //answer the location request.
        client->sendInstruction(HIPE_OP_LOCATION_RETURN, instruction.requestor,
                                getIndexOfElement(location.firstChild()), "", "");
    } else if(instruction.opcode == HIPE_OP_GET_LAST_CHILD) {
        //answer the location request.
        client->sendInstruction(HIPE_OP_LOCATION_RETURN, instruction.requestor,
                                getIndexOfElement(location.lastChild()), "", "");
    } else if(instruction.opcode == HIPE_OP_GET_NEXT_SIBLING) {
        //answer the location request.
        client->sendInstruction(HIPE_OP_LOCATION_RETURN, instruction.requestor,
                                getIndexOfElement(location.nextSibling()), "", "");
    } else if(instruction.opcode == HIPE_OP_GET_PREV_SIBLING) {
        //answer the location request.
        client->sendInstruction(HIPE_OP_LOCATION_RETURN, instruction.requestor,
                                getIndexOfElement(location.previousSibling()), "", "");
    } else if(instruction.opcode == HIPE_OP_GET_BY_ID) {
        client->sendInstruction(HIPE_OP_LOCATION_RETURN, instruction.requestor,
                                getIndexOfElement(webElement.findFirst(QString("#") + arg1)), "", "");
    } else if(instruction.opcode == HIPE_OP_SET_ATTRIBUTE) {
        if(Sanitation::isAllowedAttribute(arg1)) {
            if(arg1=="value") { //workaround for updating input boxes after creation
                location.evaluateJavaScript("this.value='" + Sanitation::sanitisePlainText(arg2) + "';");
            } else {
                location.setAttribute(arg1, Sanitation::sanitisePlainText(arg2));
            }
        }
    } else if(instruction.opcode == HIPE_OP_SET_STYLE) {
        if(Sanitation::isAllowedCSS(arg1) && Sanitation::isAllowedCSS(arg2)) {
            if(!locationSpecified) { //we need to be sure the body has been initialised first.
                if(webElement.isNull())
                    setBody("");
                webElement.setStyleProperty(arg1, arg2);

                if(getParent()) { //since a parent frame exists, we should possibly notify the parent of a new colour scheme.
                    QString fg, bg;
                    while(!bg.size()) //poll repeatedly until we get a non-null response, if required (frame might not have rendered yet).
                        bg = webElement.styleProperty("background-color", QWebElement::ComputedStyle);
                    while(!fg.size())
                        fg = webElement.styleProperty("color", QWebElement::ComputedStyle);

                    //check if foreground or background colours are defined by this client. If so, notify the parent, and the
                    //parent will update its own metadata for this frame, to determine whether to send the relevant event.
                    getParent()->receiveSubFrameEvent(HIPE_FRAME_EVENT_BACKGROUND_CHANGED, webElement.webFrame(), bg.toStdString());
                    getParent()->receiveSubFrameEvent(HIPE_FRAME_EVENT_COLOR_CHANGED, webElement.webFrame(), fg.toStdString());
                }
            } else {
                location.setStyleProperty(arg1, arg2);
            }
        }
    } else if(instruction.opcode == HIPE_OP_FREE_LOCATION) {
        removeReferenceableElement(instruction.location);
    } else if(instruction.opcode == HIPE_OP_EVENT_REQUEST) {
        QString locStr = QString::number(instruction.location);
        QString reqStr = QString::number(requestor);
        QString evtDetailArgs;
        arg1 = arg1.toLower(); //sanitise against user overriding event attributes with uppercase equivalents.
        if(arg1 == "mousemove" || arg1 == "mousedown" || arg1 == "mouseup" || arg1 == "mouseenter" || arg1 == "mouseleave" || arg1 == "mouseover" || arg1 == "mouseout")
            evtDetailArgs = "'' + event.which + ',' + event.pageX + ',' + event.pageY + ',' + (event.pageX-this.offsetLeft) + ',' + (event.pageY-this.offsetTop)";
        else
            evtDetailArgs = "event.which";
        if(arg1 == "keydown" && !locationSpecified) { //keydown on body element is a special case.
            reportKeydownOnBody=true;
            keyDownOnBodyRequestor=instruction.requestor;
        } else if(arg2 == "keydown" && !locationSpecified) { //keyup on body element is a special case.
            reportKeyupOnBody=true;
            keyUpOnBodyRequestor=instruction.requestor;
        } else
            location.setAttribute(QString("on") + arg1, QString("c.receiveGuiEvent(") + locStr + "," + reqStr + ",'" + arg1 + "'," + evtDetailArgs + ")");
    } else if(instruction.opcode == HIPE_OP_EVENT_CANCEL) {
        location.removeAttribute(QString("on") + arg1);
        if(arg2 == "1") { //reply requested. Send back an EVENT_CANCEL instruction to tell the client it can clean up event listeners for this event now.
            client->sendInstruction(HIPE_OP_EVENT_CANCEL, instruction.requestor, instruction.location, arg1.toStdString(), arg2.toStdString());
        }
    } else if(instruction.opcode == HIPE_OP_GET_GEOMETRY) {
        QString left = location.evaluateJavaScript("this.offsetLeft;").toString();
        QString top = location.evaluateJavaScript("this.offsetTop;").toString();
        if(arg1.at(0) != '1') { //get position
            client->sendInstruction(HIPE_OP_POSITION_RETURN, instruction.requestor, instruction.location,
                                    left.toStdString(), top.toStdString());
        } else { //get size
            QString width = location.evaluateJavaScript("this.offsetWidth;").toString();
            QString height = location.evaluateJavaScript("this.offsetHeight;").toString();
            client->sendInstruction(HIPE_OP_SIZE_RETURN, instruction.requestor, instruction.location,
                                    width.toStdString(), height.toStdString());
        }
    } else if(instruction.opcode == HIPE_OP_GET_ATTRIBUTE) {
        QString attrVal;
        if(arg1 == "value") {
            attrVal = location.evaluateJavaScript("this.value;").toString();
        } else if(arg1 == "checked") { //special case for checkboxes and radiobuttons -- the element might be set or unset, without a value. Return the value "checked" if checked.
            bool checkedState = location.evaluateJavaScript("this.checked;").toBool();
            attrVal = checkedState ? "checked" : "";
        } else {
            attrVal = location.attribute(arg1);
        }
        client->sendInstruction(HIPE_OP_ATTRIBUTE_RETURN, instruction.requestor, instruction.location,
                                arg1.toStdString(), attrVal.toStdString());
    } else if(instruction.opcode == HIPE_OP_SET_SRC) {
        QString dataURI = QString("data:") + arg2 + ";base64,";

        QByteArray b64Data = QByteArray(instruction.arg1, instruction.arg1Length).toBase64();

        dataURI += QString::fromLocal8Bit(b64Data);
        location.setAttribute("src", dataURI);
    } else if(instruction.opcode == HIPE_OP_SET_BACKGROUND_SRC) {
        QString dataURI = QString("data:") + arg2 + ";base64,";
        QByteArray b64Data = QByteArray(instruction.arg1, instruction.arg1Length).toBase64();
        dataURI += QString::fromLocal8Bit(b64Data);
        location.setStyleProperty("background-image", QString("url(\"") + dataURI + "\")");
    } else if(instruction.opcode == HIPE_OP_ADD_STYLE_RULE_SRC) {
        QString dataURI = QString("data:image/png;base64,");
        QByteArray b64Data = QByteArray(instruction.arg2, instruction.arg2Length).toBase64();
        dataURI += QString::fromLocal8Bit(b64Data);
        if(Sanitation::isAllowedCSS(arg1))
            stylesheet += arg1 + "{background-image:url(\"" + dataURI + "\");}\n";
        applyStylesheet();
    } else if(instruction.opcode == HIPE_OP_GET_FRAME_KEY) {
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
                    subFrames.push_back({location, frame, hostKey, requestor, "", "", 0, "", ""}); //add new entry to the table.
                    break;
                }
            }
        }
        if(frameID.size()) location.setAttribute("id", frameID); //restore any previous tag id.
        else location.removeAttribute("id");

        //return the host key to the client if element was found, else return blank string.
        client->sendInstruction(HIPE_OP_KEY_RETURN, instruction.requestor, instruction.location, found ? hostKey.toStdString() : "", "");
    } else if(instruction.opcode == HIPE_OP_FRAME_CLOSE) {
        //find the relevant client
        for(FrameData& fd : subFrames) {
            if(fd.we == location) { //found
                Container* target = globalContainerManager->findContainer(fd.wf); //find the corresponding container.
                if(target) {
                    if(!arg1.size() || arg1[0] == '\0')
                        delete target->client; //hard disconnection -- if this causes issues with Qt we might try target->client->deleteLater().
                    else
                        target->containerClosed(); //soft close request.
                }
                break;
            }
        }
    } else if(instruction.opcode == HIPE_OP_TOGGLE_CLASS) {
        location.toggleClass(arg1);
    } else if(instruction.opcode == HIPE_OP_SET_FOCUS) {
        location.setFocus();
    } else if(instruction.opcode == HIPE_OP_TAKE_SNAPSHOT) {
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
            payload.opcode = HIPE_OP_FILE_RETURN;
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
    } else if(instruction.opcode == HIPE_OP_USE_CANVAS) {
        webElement.evaluateJavaScript(QString("canvascontext=document.getElementById(\"")
                                      + location.attribute("id") + "\").getContext(\"" + arg1 + "\");");
        //TODO: sanitise arg1 against e.g. quotation marks.
        //Don't allow parentheses or semicolons.
    } else if(instruction.opcode == HIPE_OP_CANVAS_ACTION) {
        webElement.evaluateJavaScript(QString("canvascontext.") + arg1 + "(" + arg2 + ");");
        //TODO sanitise arg1 and arg2 against javascript injections.
        //Don't allow parentheses or semicolons.
    } else if(instruction.opcode == HIPE_OP_CANVAS_SET_PROPERTY) {
        webElement.evaluateJavaScript(QString("canvascontext.") + arg1 + "=" + arg2 + ";");
        //TODO sanitise arg1 and arg2 against javascript injections.
        //Don't allow parentheses, semicolons, etc.
    } else if(instruction.opcode == HIPE_OP_SET_ICON) {
        setIcon(instruction.arg1, instruction.arg1Length);
    } else if(instruction.opcode == HIPE_OP_REMOVE_ATTRIBUTE) {
        if(Sanitation::isAllowedAttribute(arg1))
            location.removeAttribute(arg1);
    } else if(instruction.opcode == HIPE_OP_MESSAGE) {
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
            target->receiveMessage(HIPE_OP_MESSAGE, requestor, std::string(instruction.arg1,instruction.arg1Length), std::string(instruction.arg2, instruction.arg2Length), sourceframe);
        }
    }
}

void Container::containerClosed()
//Called when the container is requested to be closed by the user.
//(We need to disconnect the connection to the client and free all the associated resources of this instance.)
//This function sends a message to the client to request disconnection at the client's end.
//The client needs to check for this message and deal with it.
{
    //client->deleteLater();
    client->sendInstruction(HIPE_OP_FRAME_CLOSE, 0, 0, "", "");
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
                return (Container*) new ContainerFrame(c, QString(clientName.c_str()), fd.wf, this);
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
                if(detail.compare(sf.bg) != 0) { //check frame metadata in sf, if background has not changed, return without sending event.
                    sf.bg = detail;
                } else
                    return; //no change to background colour.
            } else if(evtType == HIPE_FRAME_EVENT_COLOR_CHANGED) { //foreground colour
                if(detail.compare(sf.fg) != 0) { //similarly as for background colour
                    sf.fg = detail;
                } else
                    return; //no change to foreground colour.
            }

            client->sendInstruction(HIPE_OP_FRAME_EVENT, sf.requestor, findReferenceableElement(sf.we),
                                    evtTypeString, detail);

            if(evtType == HIPE_FRAME_EVENT_CLIENT_DISCONNECTED)
                subFrames.remove(sf);

            break;
        }
    }
}

void Container::receiveMessage(char opcode, int64_t requestor, std::string arg1, std::string arg2, QWebFrame* sender, bool propagateToParent) {
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

    client->sendInstruction(opcode, requestor, location, arg1, arg2);

    //propagate to parent (and grandparent, etc.) if flag specified.
    if(propagateToParent && getParent())
        getParent()->receiveMessage(opcode, requestor, arg1, arg2, webElement.webFrame(), true);
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

    //Determine if an onkeydown/onkeyup attribute is attached to this element. Fire off an event if so.
    if(keyUp && childFrame.hasAttribute("onkeyup"))
        client->sendInstruction(HIPE_OP_EVENT, 0 /*fixme: how can we find out the requestor?*/, location, "keyup", keycode.toStdString());
    else if(!keyUp && childFrame.hasAttribute("onkeydown"))
        client->sendInstruction(HIPE_OP_EVENT, 0 /*fixme: how can we find out the requestor?*/, location, "keydown", keycode.toStdString());

    if(getParent()) { //propagate this up to *our* parent and so on, in case they need this keyboard event.
        getParent()->keyEventOnChildFrame(webElement.webFrame(), keyUp, keycode);
    }

}


void Container::_receiveGuiEvent(quint64 location, quint64 requestor, QString event, QString detail)
{
    client->sendInstruction(HIPE_OP_EVENT, requestor, location, event.toStdString(), detail.toStdString());
}

void Container::_receiveKeyEventOnBody(bool keyUp, QString keycode)
//keyup and keydown events are treated as a special case when they happen on the body element.
//receiveGuiEvent is not called directly, instead this slot is ALWAYS called, since we want to receive
//the event and propagate it up the client tree regardless of whether the user has asked to be notified of it.
{
    if(keyUp && reportKeyupOnBody)
        _receiveGuiEvent(0, keyUpOnBodyRequestor, "keyup", keycode);
    else if(!keyUp && reportKeydownOnBody)
        _receiveGuiEvent(0, keyDownOnBodyRequestor, "keydown", keycode);

    // the whole point of this function is that we'll now notify the parent of the event.
    // if this frame has a onkeydown or onkeyup attribute specified in the parent, we'll fire off an event on that iframe.
    // Regardless, we then propagate to *that* element's parent as well.
    if(getParent()) { //propagate this up to *our* parent and so on, in case they need this keyboard event.
        getParent()->keyEventOnChildFrame(webElement.webFrame(), keyUp, keycode);
    }

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
