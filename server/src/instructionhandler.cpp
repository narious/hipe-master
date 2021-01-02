#include "instructionhandler.h"
#include "connection.h"
#include "keylist.h"
#include "sanitation.h"
#include "main.hpp"


void handle_CLEAR(Container* c, hipe_instruction*, bool locationSpecified, QWebElement location) {
    if(!locationSpecified) c->setBody("",true);
    else location.setInnerXml("");
}


void handle_DELETE(Container*, hipe_instruction*, bool locationSpecified, QWebElement location) {
    if(locationSpecified)
        location.removeFromDocument(); //Qt doc says this also makes location a 'null element'
}

void handle_FREE_LOCATION(Container* c, hipe_instruction* instruction, bool, QWebElement) {
    c->removeReferenceableElement(instruction->location);
}

void handle_GET_FIRST_CHILD(Container* c, hipe_instruction* instruction, bool, QWebElement location) {
    //answer the location request.
    c->client->sendInstruction(HIPE_OP_LOCATION_RETURN, instruction->requestor,
                                c->getIndexOfElement(location.firstChild()));
}

void handle_GET_LAST_CHILD(Container* c, hipe_instruction* instruction, bool, QWebElement location) {
    //answer the location request.
    c->client->sendInstruction(HIPE_OP_LOCATION_RETURN, instruction->requestor,
                                c->getIndexOfElement(location.lastChild()));
}

void handle_GET_NEXT_SIBLING(Container* c, hipe_instruction* instruction, bool, QWebElement location) {
    //answer the location request.
    c->client->sendInstruction(HIPE_OP_LOCATION_RETURN, instruction->requestor,
                                c->getIndexOfElement(location.nextSibling()));
}

void handle_GET_PREV_SIBLING(Container* c, hipe_instruction* instruction, bool, QWebElement location) {
    //answer the location request.
    c->client->sendInstruction(HIPE_OP_LOCATION_RETURN, instruction->requestor,
                                c->getIndexOfElement(location.previousSibling()));
}

void handle_SET_FOCUS(Container*, hipe_instruction*, bool, QWebElement location) {
    location.evaluateJavaScript("this.focus()");
}

void handle_GET_GEOMETRY(Container* c, hipe_instruction* instruction, bool, QWebElement location) {
    std::string left = location.evaluateJavaScript("this.offsetLeft;").toString().toStdString();
    std::string top = location.evaluateJavaScript("this.offsetTop;").toString().toStdString();
    std::string width = location.evaluateJavaScript("this.offsetWidth;").toString().toStdString();
    std::string height = location.evaluateJavaScript("this.offsetHeight;").toString().toStdString();
    c->client->sendInstruction(HIPE_OP_GEOMETRY_RETURN, instruction->requestor, instruction->location,
                            {left, top, width, height});
}

void handle_GET_SCROLL_GEOMETRY(Container* c, hipe_instruction* instruction, bool, QWebElement location) {
    std::string left = location.evaluateJavaScript("this.scrollLeft;").toString().toStdString();
    std::string top = location.evaluateJavaScript("this.scrollTop;").toString().toStdString();
    std::string width = location.evaluateJavaScript("this.scrollWidth;").toString().toStdString();
    std::string height = location.evaluateJavaScript("this.scrollHeight;").toString().toStdString();
    c->client->sendInstruction(HIPE_OP_GEOMETRY_RETURN, instruction->requestor, instruction->location,
                                {left, top, width, height});
}

void handle_GET_FRAME_KEY(Container* c, hipe_instruction* instruction, bool, QWebElement location) {
    //Check if the location is already represented in the frame table.
    QString frameID = location.attribute("id"); //Need this for matching the frame.
    bool found = false;
    QString hostKey = "";
    for(FrameData& fd : c->subFrames) {
        if(fd.we == location) { //found
            found = true;
            hostKey = fd.hostkey;
            break;
        }
    }

    //If not, we need to match the unique ID of the iframe DOM element to its corresponding QWebFrame.
    if(!found) {
        hostKey = c->keyList->generateContainerKey().c_str();

        //now traverse child frames to find the one with the same ID.
        auto frames = c->webElement.webFrame()->childFrames();
        for(QWebFrame* frame : frames) {
            if(frame->frameName() == frameID) {
                found = true; //match found.
                c->subFrames.push_back({location, frame, hostKey, instruction->requestor, "", "", 0, "", ""}); //add new entry to the table.
                break;
            }
        }
    }
    if(frameID.size()) location.setAttribute("id", frameID); //restore any previous tag id.
    else location.removeAttribute("id");

    //return the host key to the client if element was found, else return blank string.
    c->client->sendInstruction(HIPE_OP_KEY_RETURN, instruction->requestor, instruction->location, {found ? hostKey.toStdString() : ""});

}


//REQUIRES 3 ARGS
void handle_APPEND_TAG(Container* c, hipe_instruction* instruction, bool locationSpecified, QWebElement location, std::string arg[]) {
    arg[0] = Sanitation::sanitisePlainText(arg[0]);
    arg[1] = Sanitation::sanitisePlainText(arg[1]);
    if(Sanitation::isAllowedTag(arg[0])) { //eliminate forbidden tags.
        std::string newTagString = "<";
        newTagString += arg[0];
        if(arg[1].size()) {
            newTagString += " id=\"" + arg[1] + "\"";
        } else if(arg[0] == "iframe" || arg[0] == "canvas") { //these tags don't function properly without an ID. Make a random one.
            std::string randomID = c->keyList->generateContainerKey();
            c->keyList->claimKey(randomID); //burn through a contaner key in order to get a random string out of it.
            newTagString += " id=\"";
            newTagString += randomID;
            newTagString += "\"";
        }
        newTagString += "></" + arg[0] + ">";
        if(!locationSpecified) {
            c->setBody(newTagString, false /*append mode*/);
            location = c->webElement; //webElement may have been redefined in setBody().
        }
        else location.appendInside(newTagString.c_str());
    }
    //if the optional arg[2] is "1", we automatically bundle in a
    //'get last child' request, so the user can create a tag and get its location
    //in one go.
    if(arg[2] == "1") {
        QWebElement newElement;
        newElement = location.lastChild();
        c->client->sendInstruction(HIPE_OP_LOCATION_RETURN,
             instruction->requestor, c->getIndexOfElement(newElement));
    }
}


//REQUIRES 2 ARGS
void handle_SET_TEXT(Container* c, hipe_instruction*, bool locationSpecified, QWebElement location, std::string arg[]) {
    arg[0] = Sanitation::sanitisePlainText(arg[0], (bool)(arg[1]=="1"));
    if(!locationSpecified) c->setBody(arg[0]);
    else location.setInnerXml(arg[0].c_str());
}


//REQUIRES 2 ARGS
void handle_APPEND_TEXT(Container* c, hipe_instruction*, bool locationSpecified, QWebElement location, std::string arg[]) {
    arg[0] = Sanitation::sanitisePlainText(arg[0], (bool)(arg[1]=="1"));
    if(!locationSpecified) c->setBody(arg[0], false);
    else location.appendInside(arg[0].c_str());
}


//REQUIRES 1 ARG
void handle_GET_BY_ID(Container* c, hipe_instruction* instruction, bool, QWebElement, std::string arg[]) {
    c->client->sendInstruction(HIPE_OP_LOCATION_RETURN, instruction->requestor,
                c->getIndexOfElement(c->webElement.findFirst(QString("#") + arg[0].c_str())));
}


//REQUIRES 2 ARGS
void handle_ADD_STYLE_RULE(Container* c, hipe_instruction*, bool, QWebElement, std::string arg[]) {
    if(Sanitation::isAllowedCSS(arg[0]) && Sanitation::isAllowedCSS(arg[1]))
        c->stylesheet += arg[0] + "{" + arg[1] + "}\n";
    c->applyStylesheet();
}


//REQUIRES 2 ARGS -- third arg is extracted from the instruction directly due to
//the larger size of its contents.
void handle_ADD_FONT(Container* c, hipe_instruction* instruction, bool, QWebElement, std::string arg[]) {
//arg[0] is font family name, arg[1] is mime type, arg[2] is raw data.
    if(Sanitation::isAllowedCSS(arg[0]) && Sanitation::isAllowedCSS(arg[1]))
        c->stylesheet += "@fontface {font-family:\"" + arg[0] + "\"; src:url(\"data:"
                + arg[1] + ";base64,"
                + Sanitation::toBase64(instruction->arg[2],instruction->arg_length[2])
                + "\");}\n";
    c->applyStylesheet();
}


//REQUIRES 1 ARG
void handle_SET_TITLE(Container* c, hipe_instruction*, bool, QWebElement, std::string arg[]) {
    c->setTitle(arg[0]);
}


//REQUIRES 2 ARGS
void handle_SET_ATTRIBUTE(Container*, hipe_instruction*, bool, QWebElement location, std::string arg[]) {
    if(Sanitation::isAllowedAttribute(arg[0])) {
        if(arg[0]=="value") { //workaround for updating input boxes after creation
            location.evaluateJavaScript(QString("this.value='") + Sanitation::sanitisePlainText(arg[1]).c_str() + "';");
        } else {
            //location.setAttribute(arg[0].c_str(), Sanitation::sanitisePlainText(arg[1]).c_str());
            location.evaluateJavaScript(QString("this.setAttribute(\"") + arg[0].c_str() + "\",\"" + Sanitation::sanitisePlainText(arg[1]).c_str() + "\");");
        }
    }
}


//REQUIRES 2 ARGS
void handle_SET_STYLE(Container* c, hipe_instruction*, bool locationSpecified, QWebElement location, std::string arg[]) {
    if(Sanitation::isAllowedCSS(arg[0]) && Sanitation::isAllowedCSS(arg[1])) {
        if(!locationSpecified) { //we need to be sure the body has been initialised first.
            if(c->webElement.isNull())
                c->setBody("");
            c->webElement.setStyleProperty(arg[0].c_str(), arg[1].c_str());

            if(c->getParent()) { //since a parent frame exists, we should possibly notify the parent of a new colour scheme.
                QString fg, bg;
                while(!bg.size()) //poll repeatedly until we get a non-null response, if required (frame might not have rendered yet).
                    bg = c->webElement.styleProperty("background-color", QWebElement::ComputedStyle);
                while(!fg.size())
                    fg = c->webElement.styleProperty("color", QWebElement::ComputedStyle);

                //check if foreground or background colours are defined by this client. If so, notify the parent, and the
                //parent will update its own metadata for this frame, to determine whether to send the relevant event.
                c->getParent()->receiveSubFrameEvent(HIPE_FRAME_EVENT_BACKGROUND_CHANGED, 
                        c->webElement.webFrame(), bg.toStdString());
                c->getParent()->receiveSubFrameEvent(HIPE_FRAME_EVENT_COLOR_CHANGED, 
                        c->webElement.webFrame(), fg.toStdString());
            }
        } else {
            location.setStyleProperty(arg[0].c_str(), arg[1].c_str());
        }
    }
}


//REQUIRES 1 ARG
void handle_EVENT_REQUEST(Container* c, hipe_instruction* instruction, bool locationSpecified, QWebElement location, std::string arg[]) {
    QString locStr = QString::number(instruction->location, 16);
    QString reqStr = QString::number(instruction->requestor, 16); //represent as hex strings
    QString evtDetailArgs;
    arg[0] = Sanitation::toLower(arg[0].c_str(), arg[0].size()); //sanitise against user overriding event attributes with uppercase equivalents.
    if(arg[0] == "mousemove" || arg[0] == "mousedown" || arg[0] == "mouseup" 
                || arg[0] == "mouseenter" || arg[0] == "mouseleave" 
                || arg[0] == "mouseover" || arg[0] == "mouseout")
        evtDetailArgs = "'' + event.which + ',' + event.pageX + ',' + event.pageY + ',' + (event.pageX-this.offsetLeft) + ',' + (event.pageY-this.offsetTop)";
    else
        evtDetailArgs = "event.which";
    if(arg[0] == "keydown" && !locationSpecified) { //keydown on body element is a special case.
        c->reportKeydownOnBody=true;
        c->keyDownOnBodyRequestor=instruction->requestor;
    } else if(arg[0] == "keyup" && !locationSpecified) { //keyup on body element is a special case.
        c->reportKeyupOnBody=true;
        c->keyUpOnBodyRequestor=instruction->requestor;
    } else
        location.setAttribute(QString("on") + arg[0].c_str(), QString("c.receiveGuiEvent('") + locStr + "','" + reqStr + "','" + arg[0].c_str() + "'," + evtDetailArgs + ")");
        //Note: since Javascript's max integer range is only about 2^52, 64 bit numbers
        //need to be represented as strings to avoid loss of accuracy.
}


//REQUIRES 2 ARGS
void handle_EVENT_CANCEL(Container* c, hipe_instruction* instruction, bool, QWebElement location, std::string arg[]) {
    location.removeAttribute(QString("on") + arg[0].c_str());
    if(arg[1] == "1") { //reply requested. Send back an EVENT_CANCEL instruction to tell the client it can clean up event listeners for this event now.
        c->client->sendInstruction(HIPE_OP_EVENT_CANCEL, instruction->requestor, 
                                    instruction->location, {arg[0], arg[1]});
    }
}


//REQUIRES 3 ARGS
void handle_SCROLL_BY(Container*, hipe_instruction*, bool, QWebElement location, std::string arg[]) {
    //if arg[2] is "%", then the units are percentage of scroll track. Otherwise
    //units are pixels of positive offet at the top-left of the viewable area.
    bool percentage = false;
    if(arg[2] == "%")
        percentage = true;

    if(arg[0].size()) { //left offset which may have decimal places if zoomed in.
        try {
            float leftVal = std::stof(arg[0]); //may throw exception if invalid.
            if(!percentage)
                location.evaluateJavaScript(QString("this.scrollLeft+=") + QString::number(leftVal) + ";");
            else
                location.evaluateJavaScript(QString("this.scrollLeft+=") + QString::number(leftVal)
                        + "*(this.scrollWidth - this.clientWidth)/100.0;" );
        } catch(...) {} //just do nothing on error.
    }
    if(arg[1].size()) { //top offset
        try {
            float topVal = std::stof(arg[1]); //may throw exception if invalid.
            if(!percentage)
                location.evaluateJavaScript(QString("this.scrollTop+=") + QString::number(topVal) + ";");
            else
                location.evaluateJavaScript(QString("this.scrollTop+=") + QString::number(topVal)
                        + "*(this.scrollHeight - this.clientHeight)/100.0;" );
        } catch(...) {} //just do nothing on error.
    }
}


//REQUIRES 3 ARGS
void handle_SCROLL_TO(Container*, hipe_instruction*, bool, QWebElement location, std::string arg[]) {
    //if arg[2] is "%", then the units are percentage of scroll track. Otherwise
    //units are pixels of positive offet at the top-left of the viewable area.
    bool percentage = false;
    if(arg[2] == "%")
        percentage = true;

    if(arg[0].size()) { //left offset which may have decimal places if zoomed in.
        try {
            float leftVal = std::stof(arg[0]); //may throw exception if invalid.
            if(!percentage)
                location.evaluateJavaScript(QString("this.scrollLeft=") + QString::number(leftVal) + ";");
            else
                location.evaluateJavaScript(QString("this.scrollLeft=") + QString::number(leftVal)
                        + "*(this.scrollWidth - this.clientWidth)/100.0;" );
        } catch(...) {} //just do nothing on error.
    }
    if(arg[1].size()) { //top offset
        try {
            float topVal = std::stof(arg[1]); //may throw exception if invalid.
            if(!percentage)
                location.evaluateJavaScript(QString("this.scrollTop=") + QString::number(topVal) + ";");
            else
                location.evaluateJavaScript(QString("this.scrollTop=") + QString::number(topVal)
                        + "*(this.scrollHeight - this.clientHeight)/100.0;" );
        } catch(...) {} //just do nothing on error.
    }
}


//REQUIRES 1 ARG
void handle_GET_ATTRIBUTE(Container* c, hipe_instruction* instruction, bool, QWebElement location, std::string arg[]) {
    QString attrVal;
    if(arg[0] == "value") {
        attrVal = location.evaluateJavaScript("this.value;").toString();
    } else if(arg[0] == "checked") { 
    //special case for checkboxes and radiobuttons -- the element might be set or unset, without a value. Return the value "checked" if checked.
        bool checkedState = location.evaluateJavaScript("this.checked;").toBool();
        attrVal = checkedState ? "checked" : "";
    } else {
        attrVal = location.attribute(arg[0].c_str());
    }
    c->client->sendInstruction(HIPE_OP_ATTRIBUTE_RETURN, instruction->requestor, 
                            instruction->location,
                            {arg[0], attrVal.toStdString()});
}


//REQUIRES 1 ARG
void handle_FRAME_CLOSE(Container* c, hipe_instruction*, bool, QWebElement location, std::string arg[]) {
    //find the relevant client
    for(FrameData& fd : c->subFrames) {
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
}
