/*  Copyright (c) 2016-2020 Daniel Kos, General Development Systems

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

#include <QWebFrame>
#include <QWebPage>
#include <QPrinter>
#include <QDesktopServices>
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
    std::string arg[HIPE_NARGS];
    arg[0] = std::string(instruction.arg[0], instruction.arg_length[0]);
    arg[1] = std::string(instruction.arg[1], instruction.arg_length[1]);
    //if an operation needs additional instructions, initialise these explicitly in the same way.

    uint64_t requestor = instruction.requestor;
    bool locationSpecified = (bool) instruction.location;
    QWebElement location = locationSpecified ? getReferenceableElement(instruction.location)
                                             : webElement; //may need to update this after calling setBody!!


    //check instructions in order of most common. A map would also be useful here.

    if(instruction.opcode == HIPE_OP_APPEND_TAG) { //This is by far the most common instruction. Check first.
        arg[0] = Sanitation::sanitisePlainText(arg[0]);
        arg[1] = Sanitation::sanitisePlainText(arg[1]);
        if(Sanitation::isAllowedTag(arg[0])) { //eliminate forbidden tags.
            std::string newTagString = "<";
            newTagString += arg[0];
            if(arg[1].size()) {
                newTagString += " id=\"" + arg[1] + "\"";
            } else if(arg[0] == "iframe" || arg[0] == "canvas") { //these tags don't function properly without an ID. Make a random one.
                std::string randomID = keyList->generateContainerKey();
                keyList->claimKey(randomID); //burn through a contaner key in order to get a random string out of it.
                newTagString += " id=\"";
                newTagString += randomID;
                newTagString += "\"";
            }
            newTagString += "></" + arg[0] + ">";
            if(!locationSpecified) {
                setBody(newTagString, false /*append mode*/);
                location = webElement; //webElement may have been redefined in setBody().
            }
            else location.appendInside(newTagString.c_str());
        }
        if(instruction.arg_length[2]) {
        //if the optional arg[2] is "1", we automatically bundle in a
        //'get last child' request, so the user can create a tag and get its location
        //in one go.
            arg[2] = std::string(instruction.arg[2], instruction.arg_length[2]);
            if(arg[2] == "1") {
                QWebElement newElement;
                newElement = location.lastChild();
                client->sendInstruction(HIPE_OP_LOCATION_RETURN,
                     instruction.requestor, getIndexOfElement(newElement));
            }
        }
    } else if(instruction.opcode == HIPE_OP_CLEAR) {
        if(!locationSpecified) setBody("",true);
        else location.setInnerXml("");
    } else if(instruction.opcode == HIPE_OP_DELETE) {
        if(locationSpecified)
            location.removeFromDocument(); //Qt doc says this also makes location a 'null element'
    } else if(instruction.opcode == HIPE_OP_SET_TEXT) {
        arg[0] = Sanitation::sanitisePlainText(arg[0], (bool)(arg[1]=="1"));
        if(!locationSpecified) setBody(arg[0]);
        else location.setInnerXml(arg[0].c_str());
    } else if(instruction.opcode == HIPE_OP_APPEND_TEXT) {
        arg[0] = Sanitation::sanitisePlainText(arg[0], (bool)(arg[1]=="1"));
        if(!locationSpecified) setBody(arg[0], false);
        else location.appendInside(arg[0].c_str());
    } else if(instruction.opcode == HIPE_OP_ADD_STYLE_RULE) {
        if(Sanitation::isAllowedCSS(arg[0]) && Sanitation::isAllowedCSS(arg[1]))
            stylesheet += arg[0] + "{" + arg[1] + "}\n";
        applyStylesheet();
    } else if(instruction.opcode == HIPE_OP_ADD_FONT) {
        //arg[0] is font family name, arg[1] is mime type, arg[2] is raw data.
        if(Sanitation::isAllowedCSS(arg[0]) && Sanitation::isAllowedCSS(arg[1]))
            stylesheet += "@fontface {font-family:\"" + arg[0] + "\"; src:url(\"data:"
                    + arg[1] + ";base64,"
                    + Sanitation::toBase64(std::string(instruction.arg[2],instruction.arg_length[2]))
                    + "\");}\n";
        applyStylesheet();
    } else if(instruction.opcode == HIPE_OP_SET_TITLE) {
        setTitle(arg[0]);
    } else if(instruction.opcode == HIPE_OP_GET_FIRST_CHILD) {
        //answer the location request.
        client->sendInstruction(HIPE_OP_LOCATION_RETURN, instruction.requestor,
                                getIndexOfElement(location.firstChild()));
    } else if(instruction.opcode == HIPE_OP_GET_LAST_CHILD) {
        //answer the location request.
        client->sendInstruction(HIPE_OP_LOCATION_RETURN, instruction.requestor,
                                getIndexOfElement(location.lastChild()));
    } else if(instruction.opcode == HIPE_OP_GET_NEXT_SIBLING) {
        //answer the location request.
        client->sendInstruction(HIPE_OP_LOCATION_RETURN, instruction.requestor,
                                getIndexOfElement(location.nextSibling()));
    } else if(instruction.opcode == HIPE_OP_GET_PREV_SIBLING) {
        //answer the location request.
        client->sendInstruction(HIPE_OP_LOCATION_RETURN, instruction.requestor,
                                getIndexOfElement(location.previousSibling()));
    } else if(instruction.opcode == HIPE_OP_GET_BY_ID) {
        client->sendInstruction(HIPE_OP_LOCATION_RETURN, instruction.requestor,
                                getIndexOfElement(webElement.findFirst(QString("#") + arg[0].c_str())));
    } else if(instruction.opcode == HIPE_OP_SET_ATTRIBUTE) {
        if(Sanitation::isAllowedAttribute(arg[0])) {
            if(arg[0]=="value") { //workaround for updating input boxes after creation
                location.evaluateJavaScript(QString("this.value='") + Sanitation::sanitisePlainText(arg[1]).c_str() + "';");
            } else {
                //location.setAttribute(arg[0].c_str(), Sanitation::sanitisePlainText(arg[1]).c_str());
                location.evaluateJavaScript(QString("this.setAttribute(\"") + arg[0].c_str() + "\",\"" + Sanitation::sanitisePlainText(arg[1]).c_str() + "\");");
            }
        }
    } else if(instruction.opcode == HIPE_OP_SET_STYLE) {
        if(Sanitation::isAllowedCSS(arg[0]) && Sanitation::isAllowedCSS(arg[1])) {
            if(!locationSpecified) { //we need to be sure the body has been initialised first.
                if(webElement.isNull())
                    setBody("");
                webElement.setStyleProperty(arg[0].c_str(), arg[1].c_str());

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
                location.setStyleProperty(arg[0].c_str(), arg[1].c_str());
            }
        }
    } else if(instruction.opcode == HIPE_OP_FREE_LOCATION) {
        removeReferenceableElement(instruction.location);
    } else if(instruction.opcode == HIPE_OP_EVENT_REQUEST) {
        QString locStr = QString::number(instruction.location, 16);
        QString reqStr = QString::number(requestor, 16); //represent as hex strings
        QString evtDetailArgs;
        arg[0] = Sanitation::toLower(arg[0].c_str(), arg[0].size()); //sanitise against user overriding event attributes with uppercase equivalents.
        if(arg[0] == "mousemove" || arg[0] == "mousedown" || arg[0] == "mouseup" || arg[0] == "mouseenter" || arg[0] == "mouseleave" || arg[0] == "mouseover" || arg[0] == "mouseout")
            evtDetailArgs = "'' + event.which + ',' + event.pageX + ',' + event.pageY + ',' + (event.pageX-this.offsetLeft) + ',' + (event.pageY-this.offsetTop)";
        else
            evtDetailArgs = "event.which";
        if(arg[0] == "keydown" && !locationSpecified) { //keydown on body element is a special case.
            reportKeydownOnBody=true;
            keyDownOnBodyRequestor=instruction.requestor;
        } else if(arg[0] == "keyup" && !locationSpecified) { //keyup on body element is a special case.
            reportKeyupOnBody=true;
            keyUpOnBodyRequestor=instruction.requestor;
        } else
            location.setAttribute(QString("on") + arg[0].c_str(), QString("c.receiveGuiEvent('") + locStr + "','" + reqStr + "','" + arg[0].c_str() + "'," + evtDetailArgs + ")");
            //Note: since Javascript's max integer range is only about 2^52, 64 bit numbers
            //need to be represented as strings to avoid loss of accuracy.
    } else if(instruction.opcode == HIPE_OP_EVENT_CANCEL) {
        location.removeAttribute(QString("on") + arg[0].c_str());
        if(arg[1] == "1") { //reply requested. Send back an EVENT_CANCEL instruction to tell the client it can clean up event listeners for this event now.
            client->sendInstruction(HIPE_OP_EVENT_CANCEL, instruction.requestor, instruction.location, {arg[0], arg[1]});
        }
    } else if(instruction.opcode == HIPE_OP_GET_GEOMETRY) {
        std::string left = location.evaluateJavaScript("this.offsetLeft;").toString().toStdString();
        std::string top = location.evaluateJavaScript("this.offsetTop;").toString().toStdString();
        std::string width = location.evaluateJavaScript("this.offsetWidth;").toString().toStdString();
        std::string height = location.evaluateJavaScript("this.offsetHeight;").toString().toStdString();
        client->sendInstruction(HIPE_OP_GEOMETRY_RETURN, instruction.requestor, instruction.location,
                                    {left, top, width, height});
    } else if(instruction.opcode == HIPE_OP_GET_SCROLL_GEOMETRY) {
        std::string left = location.evaluateJavaScript("this.scrollLeft;").toString().toStdString();
        std::string top = location.evaluateJavaScript("this.scrollTop;").toString().toStdString();
        std::string width = location.evaluateJavaScript("this.scrollWidth;").toString().toStdString();
        std::string height = location.evaluateJavaScript("this.scrollHeight;").toString().toStdString();
        client->sendInstruction(HIPE_OP_GEOMETRY_RETURN, instruction.requestor, instruction.location,
                                    {left, top, width, height});
    } else if(instruction.opcode == HIPE_OP_SCROLL_BY) {
        //if arg[2] is "%", then the units are percentage of scroll track. Otherwise
        //units are pixels of positive offet at the top-left of the viewable area.
        bool percentage = false;
        if(instruction.arg_length[2] && instruction.arg[2][0] == '%')
            percentage = true;

        if(instruction.arg_length[0]) { //left offset which may have decimal places if zoomed in.
            try {
                float leftVal = std::stof(arg[0]); //may throw exception if invalid.
                if(!percentage)
                    location.evaluateJavaScript(QString("this.scrollLeft+=") + QString::number(leftVal) + ";");
                else
                    location.evaluateJavaScript(QString("this.scrollLeft+=") + QString::number(leftVal)
                            + "*(this.scrollWidth - this.clientWidth)/100.0;" );
            } catch(...) {} //just do nothing on error.
        }
        if(instruction.arg_length[1]) { //top offset
            try {
                float topVal = std::stof(arg[1]); //may throw exception if invalid.
                if(!percentage)
                    location.evaluateJavaScript(QString("this.scrollTop+=") + QString::number(topVal) + ";");
                else
                    location.evaluateJavaScript(QString("this.scrollTop+=") + QString::number(topVal)
                            + "*(this.scrollHeight - this.clientHeight)/100.0;" );
            } catch(...) {} //just do nothing on error.
        }
    } else if(instruction.opcode == HIPE_OP_SCROLL_TO) {

        //if arg[2] is "%", then the units are percentage of scroll track. Otherwise
        //units are pixels of positive offet at the top-left of the viewable area.
        bool percentage = false;
        if(instruction.arg_length[2] && instruction.arg[2][0] == '%')
            percentage = true;

        if(instruction.arg_length[0]) { //left offset which may have decimal places if zoomed in.
            try {
                float leftVal = std::stof(arg[0]); //may throw exception if invalid.
                if(!percentage)
                    location.evaluateJavaScript(QString("this.scrollLeft=") + QString::number(leftVal) + ";");
                else
                    location.evaluateJavaScript(QString("this.scrollLeft=") + QString::number(leftVal)
                            + "*(this.scrollWidth - this.clientWidth)/100.0;" );
            } catch(...) {} //just do nothing on error.
        }
        if(instruction.arg_length[1]) { //top offset
            try {
                float topVal = std::stof(arg[1]); //may throw exception if invalid.
                if(!percentage)
                    location.evaluateJavaScript(QString("this.scrollTop=") + QString::number(topVal) + ";");
                else
                    location.evaluateJavaScript(QString("this.scrollTop=") + QString::number(topVal)
                            + "*(this.scrollHeight - this.clientHeight)/100.0;" );
            } catch(...) {} //just do nothing on error.
        }
    } else if(instruction.opcode == HIPE_OP_GET_ATTRIBUTE) {
        QString attrVal;
        if(arg[0] == "value") {
            attrVal = location.evaluateJavaScript("this.value;").toString();
        } else if(arg[0] == "checked") { //special case for checkboxes and radiobuttons -- the element might be set or unset, without a value. Return the value "checked" if checked.
            bool checkedState = location.evaluateJavaScript("this.checked;").toBool();
            attrVal = checkedState ? "checked" : "";
        } else {
            attrVal = location.attribute(arg[0].c_str());
        }
        client->sendInstruction(HIPE_OP_ATTRIBUTE_RETURN, instruction.requestor, instruction.location,
                                {arg[0], attrVal.toStdString()});
    } else if(instruction.opcode == HIPE_OP_SET_SRC) {
        std::string dataURI = std::string("data:") + arg[1] + ";base64," + Sanitation::toBase64(arg[0]);
        location.setAttribute("src", dataURI.c_str());
    } else if(instruction.opcode == HIPE_OP_SET_STYLE_SRC) {
        arg[2] = std::string(instruction.arg[2], instruction.arg_length[2]); //mime type
        arg[3] = std::string(instruction.arg[3], instruction.arg_length[3]); //supplementary value as suffix.

        std::string dataURI = std::string("data:") + arg[2] + ";base64," + Sanitation::toBase64(arg[1]);
        if(Sanitation::isAllowedCSS(arg[0]))
            location.setStyleProperty(arg[0].c_str(), QString("url(\"") + dataURI.c_str() + "\") " + arg[3].c_str());
    } else if(instruction.opcode == HIPE_OP_ADD_STYLE_RULE_SRC) {
        std::string dataURI = std::string("data:image/png;base64,") + Sanitation::toBase64(arg[1]);
        if(Sanitation::isAllowedCSS(arg[0]))
            stylesheet += arg[0] + "{background-image:url(\"" + dataURI + "\");}\n";
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
        client->sendInstruction(HIPE_OP_KEY_RETURN, instruction.requestor, instruction.location, {found ? hostKey.toStdString() : ""});
    } else if(instruction.opcode == HIPE_OP_FRAME_CLOSE) {
        //find the relevant client
        for(FrameData& fd : subFrames) {
            if(fd.we == location) { //found
                Connection* target = identifyFromFrame(fd.wf); //find the corresponding container.
                if(target) {
                    if(!arg[0].size() || arg[0][0] == '\0')
                        target->disconnect(); //Hard disconnection. Will be cleaned up in the next service cycle.
                    else
                        target->container->containerClosed(); //soft close request.
                }
                break;
            }
        }
    } else if(instruction.opcode == HIPE_OP_TOGGLE_CLASS) {
        location.toggleClass(arg[0].c_str());
    } else if(instruction.opcode == HIPE_OP_SET_FOCUS) {
        location.evaluateJavaScript("this.focus()");
    } else if(instruction.opcode == HIPE_OP_TAKE_SNAPSHOT) {
        if(Sanitation::toLower(arg[0].c_str(), arg[0].size()) == "pdf") { //vector screenshot.
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
                payload.arg[0] = fData;
                payload.arg_length[0] = size;
                payload.arg[1] = 0; payload.arg_length[1] = 0;
            } else {
                payload.arg[0] = 0; payload.arg_length[0] = 0;
                payload.arg[1] = (char*) "File error.";
                payload.arg_length[1] = 11;
            }

            client->sendInstruction(payload);
            free(fData);
            remove(snapshotFile.toStdString().c_str()); //delete the temporary file.
        }
    } else if(instruction.opcode == HIPE_OP_USE_CANVAS) {
        arg[0] = Sanitation::sanitiseCanvasInstruction(arg[0]);
        webElement.evaluateJavaScript(QString("canvascontext=document.getElementById(\"")
                                      + location.attribute("id") + "\").getContext(\"" + arg[0].c_str() + "\");");
    } else if(instruction.opcode == HIPE_OP_CANVAS_ACTION) {
        arg[0] = Sanitation::sanitiseCanvasInstruction(arg[0]);
        arg[1] = Sanitation::sanitiseCanvasInstruction(arg[1]);
        webElement.evaluateJavaScript(QString("canvascontext.") + arg[0].c_str() + "(" + arg[1].c_str() + ");");
    } else if(instruction.opcode == HIPE_OP_CANVAS_SET_PROPERTY) {
        arg[0] = Sanitation::sanitiseCanvasInstruction(arg[0]);
        arg[1] = Sanitation::sanitiseCanvasInstruction(arg[1]);
        webElement.evaluateJavaScript(QString("canvascontext.") + arg[0].c_str() + "=" + arg[1].c_str() + ";");
    } else if(instruction.opcode == HIPE_OP_SET_ICON) {
        setIcon(instruction.arg[0], instruction.arg_length[0]);
    } else if(instruction.opcode == HIPE_OP_REMOVE_ATTRIBUTE) {
        if(Sanitation::isAllowedAttribute(arg[0]))
            location.removeAttribute(arg[0].c_str());
    } else if(instruction.opcode == HIPE_OP_MESSAGE
            || instruction.opcode == HIPE_OP_FIFO_ADD_ABILITY
            || instruction.opcode == HIPE_OP_FIFO_REMOVE_ABILITY
            || instruction.opcode == HIPE_OP_FIFO_OPEN
            || instruction.opcode == HIPE_OP_FIFO_CLOSE
            || instruction.opcode == HIPE_OP_FIFO_RESPONSE
            || instruction.opcode == HIPE_OP_FIFO_DROP_PEER
            || instruction.opcode == HIPE_OP_FIFO_GET_PEER
            || instruction.opcode == HIPE_OP_OPEN_LINK) {
        //Determine whether we need to send the message to the parent frame or a child frame.
        Container* target = nullptr;
        QWebFrame* sourceframe = nullptr;
        if(locationSpecified) {
            //find the relevant child frame client
            for(FrameData& fd : subFrames) {
                if(fd.we == location) { //found
                    target = identifyFromFrame(fd.wf)->container; //find the corresponding container.
                    break;
                }
            }
        } else { //send to parent element
            target = getParent();
            sourceframe = webElement.webFrame();
        }
        if(target) { //send the instruction to the destination. (at top level, target is nullptr)
            target->receiveMessage(instruction.opcode, requestor, 
                {std::string(instruction.arg[0],instruction.arg_length[0]), 
                    std::string(instruction.arg[1], instruction.arg_length[1])}, 
                sourceframe
            );
        } else if(instruction.opcode == HIPE_OP_FIFO_GET_PEER) {
        //special case for HIPE_OP_FIFO_GET_PEER instruction where a frame tries to
        //send it outside the top level. This would normally mean an application wishes
        //to import or export a file but is running in a top-level window in another
        //desktop environment. In this case, display an open/save dialog in order to
        //give the client application a real file to work with.

            std::string accessModeStr = arg[1];

            std::string filepath = ((ContainerTopLevel*)this)->selectFileResource(arg[0],
                       std::string(instruction.arg[2],instruction.arg_length[2]), accessModeStr);
            //accessModeStr is modified by-reference to now reflect the actual access modes granted.
            //(Only r and w are supported at top level)

            //separate filename and extension parts...
            size_t strPos = filepath.rfind("/");
            if(strPos == std::string::npos) strPos = 0; else strPos++;
            std::string filename = filepath.substr(strPos);
            strPos = filename.rfind("."); //isolate file extension
            std::string fileType;
            if(strPos != std::string::npos) {
                fileType = filename.substr(strPos+1);
            }
 
            //send reply to client
            this->receiveMessage(HIPE_OP_FIFO_RESPONSE, requestor, 
                       {filepath, accessModeStr, filename, fileType}, nullptr);
            
        } else if(instruction.opcode == HIPE_OP_OPEN_LINK) {
        //special case for opening URL in user's browser if already running at top level.
            QDesktopServices::openUrl(QUrl(arg[0].c_str()));
        }
    } else if(instruction.opcode == HIPE_OP_GET_CONTENT) {
        //get inner content of (extract data from) location.
        std::string contentStr;
        if(arg[0] == "0" || arg[0] == "") { //default: unformatted/plain text
            contentStr = location.evaluateJavaScript("this.textContent;").toString().toStdString();
        } else if(arg[0] == "1") { //html-formatted content requested from element.
            contentStr = location.evaluateJavaScript("this.innerHTML;").toString().toStdString();
        } else if(arg[0] == "2") {
            contentStr = location.evaluateJavaScript("this.innerText;").toString().toStdString();
        } else if(arg[0] == "3") { //some form elements require data to be read via a value attribute
            contentStr = location.evaluateJavaScript("this.value;").toString().toStdString();
        }
        client->sendInstruction(HIPE_OP_CONTENT_RETURN, instruction.requestor,
                                           instruction.location, {contentStr});
    } else if(instruction.opcode == HIPE_OP_CARAT_POSITION) {
        //set the selection start/end position...
        arg[0] = Sanitation::sanitisePlainText(arg[0]); //selection start
        arg[1] = Sanitation::sanitisePlainText(arg[1]); //selection end, if specified
        if(!arg[1].size()) arg[1] = arg[0]; //if unspecified, end=start means cursor without selection.
        location.evaluateJavaScript(QString("this.setSelectionRange(")+arg[0].c_str()+","+arg[1].c_str()+");");
    } else if(instruction.opcode == HIPE_OP_GET_CARAT_POSITION) {
        std::string selStart, selEnd;
        selStart = location.evaluateJavaScript("this.selectionStart;").toString().toStdString();
        selEnd = location.evaluateJavaScript("this.selectionEnd;").toString().toStdString();
        client->sendInstruction(HIPE_OP_CARAT_POSITION, instruction.requestor, instruction.location, {selStart, selEnd});
    } else if(instruction.opcode == HIPE_OP_FIND_TEXT) {
        if(isTopLevel) { //only the top level frame can use this instruction due
        //to a limitation in Qt; there is no findText() method for individual frames.
            ((ContainerTopLevel*)this)->findText(arg[0], false,false,false);
            //TODO: add support for search direction args, etc.
            //e.g. arg[1] might contain "bi" for 'backwards, case insensitive'
        }
    } else if(instruction.opcode == HIPE_OP_GET_AUDIOVIDEO_STATE) {
        std::string position = location.evaluateJavaScript("this.currentTime+','+this.duration;").toString().toStdString();
        //stores the position in the <audio>/<video> tag in the format
        //"currentTime,totalTime" where both times are in seconds, and separated
        //by a comma. When the user sends this data, they can ommit the total time,
        //which will be ignored.

        std::string speed = location.evaluateJavaScript("this.playbackRate;").toString().toStdString();

        bool playing = location.evaluateJavaScript("(!this.paused || this.currentTime);").toBool();
        //condition based on:https://stackoverflow.com/questions/9437228/html5-check-if-audio-is-playing
        //If playing is false, the element may be paused, ended or waiting for playback to begin.

        std::string volume = location.evaluateJavaScript("this.voume;").toString().toStdString();

        client->sendInstruction(HIPE_OP_AUDIOVIDEO_STATE, instruction.requestor,
                instruction.location, {position, speed, (playing?"1":"0"), volume});

    } else if(instruction.opcode == HIPE_OP_AUDIOVIDEO_STATE) {
        float argValue;

        //if arg[0] is non-null, parse the new playback position as the first "%f" in the string.
        if(sscanf(arg[0].c_str(), "%f", &argValue) == 1) {
            location.evaluateJavaScript(QString("this.currentTime=")+QString::number(argValue)+";");
        }

        //if arg[1] is non-null, take the playback speed as a float.
        if(sscanf(arg[1].c_str(), "%f", &argValue) == 1) {
            location.evaluateJavaScript(QString("this.playbackRate=")+QString::number(argValue)+";");
        }

        //if arg[2] is "1" call play() and if "0" then pause().
        if(instruction.arg_length[2]) {  //arg is specified.
            if(instruction.arg[2][0]=='1')
                location.evaluateJavaScript("this.play();");
            else if(instruction.arg[2][0]=='0')
                location.evaluateJavaScript("this.pause();");
        }

        //if arg[3] is non-null, take volume as a float.
        if(instruction.arg_length[3]) {
            arg[3] = std::string(instruction.arg[3],instruction.arg_length[3]);
            if(sscanf(arg[3].c_str(), "%f", &argValue) == 1)
                location.evaluateJavaScript(QString("this.volume=")+QString::number(argValue)+";");
        }
    } else if(instruction.opcode == HIPE_OP_DIALOG || instruction.opcode == HIPE_OP_DIALOG_INPUT) {
        Container* target = getParent();

        if(!target) { //we are the top level. Dialog is handled here directly
            arg[2] = std::string(instruction.arg[2],instruction.arg_length[2]);
            bool cancelled;
            bool editable = (bool) (instruction.opcode == HIPE_OP_DIALOG_INPUT);


            std::string userChoice = ((ContainerTopLevel*)this)->dialog(arg[0], 
                                        arg[1], arg[2], editable, &cancelled);

            if(!cancelled) { //dialog wasn't cancelled
                //find the index+1 of the choice selected...
                std::string itemIndexStr = "";
                QStringList items = ((QString)(arg[2].c_str())).split("\n");

                for(int i=0; i<items.size(); i++) {
                    if(items[i] == ((QString)(userChoice.c_str()))) {
                        itemIndexStr = std::to_string(i+1);
                        break;
                    }
                    if(itemIndexStr=="") itemIndexStr = 1; //in case of free-form text entry.
                }
                client->sendInstruction(HIPE_OP_DIALOG_RETURN, requestor,
                                0, {userChoice, itemIndexStr});

            } else { //cancelled
                client->sendInstruction(HIPE_OP_DIALOG_RETURN, requestor, 0, {"","0"});
            }
        } else { //relay to parent node.
            if(instruction.arg_length[2]) arg[2]=std::string(instruction.arg[2],instruction.arg_length[2]);
            else arg[2] = "";
            if(instruction.arg_length[3]) arg[3]=std::string(instruction.arg[3],instruction.arg_length[3]);
            else arg[3] = "";

            target->receiveMessage(HIPE_OP_DIALOG, requestor, {arg[0],arg[1],
                std::string(instruction.arg[2],instruction.arg_length[2]),
                std::string(instruction.arg[3],instruction.arg_length[3])},
                webElement.webFrame(), false);
            //this sends the instruction to the parent's client.
        }
    } else if(instruction.opcode == HIPE_OP_DIALOG_RETURN) {
        if(locationSpecified) {
        //The fact we're receiving from the client and not sending this,
        //means a location is mandatory. We simply relay this instruction to the child frame.

            Container* target = nullptr;
            //find the relevant child frame client
            for(FrameData& fd : subFrames) {
                if(fd.we == location) { //found
                    target = identifyFromFrame(fd.wf)->container; //find the corresponding container.
                    break;
                }
            }

            if(target) {
                if(instruction.arg_length[2]) arg[2]=std::string(instruction.arg[2],instruction.arg_length[2]);
                else arg[2] = "";
                if(instruction.arg_length[3]) arg[3]=std::string(instruction.arg[3],instruction.arg_length[3]);
                else arg[3] = "";

                target->receiveMessage(HIPE_OP_DIALOG_RETURN, requestor, {arg[0],arg[1],
                    std::string(instruction.arg[2],instruction.arg_length[2]),
                    std::string(instruction.arg[3],instruction.arg_length[3])},
                    nullptr, false);
            }

        }
    } else if(instruction.opcode == HIPE_OP_GET_SELECTION) {
        std::string selectedText = ""; //if nothing is selected, or the functionality is
        //outside the client's purview to see, a blank string will be returned.
        if(arg[0] == "1") { //a top-level window has requested the global selection.
            if(isTopLevel) {
                selectedText = ((ContainerTopLevel*)this)->getGlobalSelection(false);
            }
        } else { //get local (this frame's) selection using javascript.
            selectedText = location.evaluateJavaScript("document.getSelection().toString();").toString().toStdString();
        }
        //return the contents of the selection...
        client->sendInstruction(HIPE_OP_CONTENT_RETURN, instruction.requestor,
                                           instruction.location, {selectedText});
    } else if(instruction.opcode == HIPE_OP_EDIT_ACTION) {
        if(isTopLevel) {
            ((ContainerTopLevel*)this)->triggerEditAction(arg[0][0]);
        }
    } else if(instruction.opcode == HIPE_OP_EDIT_STATUS) {
    //for this op, the user specifies a string of edit function codes to check,
    //e.g. "xcvbiu" to check cut,copy,paste,bold,italic,underline.
        std::string resultingStates = "";
        char state;
        for(size_t i=0; i<arg[0].size(); i++) { //for each edit function to be checked
            state = this->editActionStatus(arg[0][i]);
            resultingStates += state;
        }
        client->sendInstruction(HIPE_OP_EDIT_STATUS, instruction.requestor,
                                      instruction.location, {arg[0], resultingStates});
    }
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

void Container::receiveMessage(char opcode, int64_t requestor, const std::vector<std::string>& args, /*std::string arg1, std::string arg2,*/ QWebFrame* sender, bool propagateToParent) {
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
