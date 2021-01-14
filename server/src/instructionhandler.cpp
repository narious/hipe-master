#include "instructionhandler.h"
#include "connection.h"
#include "keylist.h"
#include "sanitation.h"


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


//REQUIRES 1 ARGS
void handle_GET_BY_ID(Container* c, hipe_instruction* instruction, bool, QWebElement, std::string arg[]) {
    c->client->sendInstruction(HIPE_OP_LOCATION_RETURN, instruction->requestor,
                c->getIndexOfElement(c->webElement.findFirst(QString("#") + arg[0].c_str())));
}

