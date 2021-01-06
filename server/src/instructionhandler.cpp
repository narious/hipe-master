#include "instructionhandler.h"
#include "connection.h"
#include "keylist.h"
#include "sanitation.h"
#include "main.hpp"

#include <QPrinter>
#include <QDesktopServices>


//A value that exceeds the highest value HIPE_OP_ constant for hipe instructions.
#define MAX_OP_CODE 120 //update if ever the max hipe instruction code value exceeds this.

//global variables used for mapping functions to function pointers...
struct {
    //the function pointer will have a different signature if it takes pre-converted
    //arguments.
    union {
        void (*noargs)(Container*, hipe_instruction*, bool, QWebElement);
        void (*withargs)(Container*, hipe_instruction*, bool, QWebElement, std::string[]);
    } ptrtype;

    short numargs; //the number of arguments to pre-convert to std::string

} handlerInfo[MAX_OP_CODE];
//The HIPE_OP number of the instruction is used to index the array.


void initInstructionMap() {
    for(int i=0; i<MAX_OP_CODE; i++) {
    //initialise all elements to nullptrs in case an unknown instruction comes through later.
        handlerInfo[i].ptrtype.noargs = nullptr;
        handlerInfo[i].numargs = 0;
    }

    //populate the handlerInfo array with hipe instruction handlers...

    handlerInfo[HIPE_OP_CLEAR].ptrtype.noargs = handle_CLEAR;

    handlerInfo[HIPE_OP_DELETE].ptrtype.noargs = handle_DELETE;

    handlerInfo[HIPE_OP_FREE_LOCATION].ptrtype.noargs = handle_FREE_LOCATION;

    handlerInfo[HIPE_OP_GET_FIRST_CHILD].ptrtype.noargs = handle_GET_FIRST_CHILD;

    handlerInfo[HIPE_OP_GET_LAST_CHILD].ptrtype.noargs = handle_GET_LAST_CHILD;

    handlerInfo[HIPE_OP_GET_NEXT_SIBLING].ptrtype.noargs = handle_GET_NEXT_SIBLING;

    handlerInfo[HIPE_OP_GET_PREV_SIBLING].ptrtype.noargs = handle_GET_PREV_SIBLING;

    handlerInfo[HIPE_OP_SET_FOCUS].ptrtype.noargs = handle_SET_FOCUS;

    handlerInfo[HIPE_OP_GET_GEOMETRY].ptrtype.noargs = handle_GET_GEOMETRY;

    handlerInfo[HIPE_OP_GET_SCROLL_GEOMETRY].ptrtype.noargs = handle_GET_SCROLL_GEOMETRY;

    handlerInfo[HIPE_OP_GET_FRAME_KEY].ptrtype.noargs = handle_GET_FRAME_KEY;

    handlerInfo[HIPE_OP_SET_ICON].ptrtype.noargs = handle_SET_ICON;

    handlerInfo[HIPE_OP_SET_SRC].ptrtype.noargs = handle_SET_SRC;

    handlerInfo[HIPE_OP_SET_STYLE_SRC].ptrtype.noargs = handle_SET_STYLE_SRC;

    handlerInfo[HIPE_OP_ADD_STYLE_RULE_SRC].ptrtype.noargs = handle_ADD_STYLE_RULE_SRC;

    handlerInfo[HIPE_OP_GET_CARAT_POSITION].ptrtype.noargs = handle_GET_CARAT_POSITION;

    handlerInfo[HIPE_OP_GET_AUDIOVIDEO_STATE].ptrtype.noargs = handle_GET_AUDIOVIDEO_STATE;
    
    handlerInfo[HIPE_OP_APPEND_TAG].ptrtype.withargs = handle_APPEND_TAG;
    handlerInfo[HIPE_OP_APPEND_TAG].numargs = 3;

    handlerInfo[HIPE_OP_SET_TEXT].ptrtype.withargs = handle_SET_TEXT;
    handlerInfo[HIPE_OP_SET_TEXT].numargs = 2;

    handlerInfo[HIPE_OP_APPEND_TEXT].ptrtype.withargs = handle_APPEND_TEXT;
    handlerInfo[HIPE_OP_APPEND_TEXT].numargs = 2;

    handlerInfo[HIPE_OP_GET_BY_ID].ptrtype.withargs = handle_GET_BY_ID;
    handlerInfo[HIPE_OP_GET_BY_ID].numargs = 1;

    handlerInfo[HIPE_OP_ADD_STYLE_RULE].ptrtype.withargs = handle_ADD_STYLE_RULE;
    handlerInfo[HIPE_OP_ADD_STYLE_RULE].numargs = 2;

    handlerInfo[HIPE_OP_ADD_FONT].ptrtype.withargs = handle_ADD_FONT;
    handlerInfo[HIPE_OP_ADD_FONT].numargs = 2;

    handlerInfo[HIPE_OP_SET_TITLE].ptrtype.withargs = handle_SET_TITLE;
    handlerInfo[HIPE_OP_SET_TITLE].numargs = 1;

    handlerInfo[HIPE_OP_SET_ATTRIBUTE].ptrtype.withargs = handle_SET_ATTRIBUTE;
    handlerInfo[HIPE_OP_SET_ATTRIBUTE].numargs = 2;

    handlerInfo[HIPE_OP_SET_STYLE].ptrtype.withargs = handle_SET_STYLE;
    handlerInfo[HIPE_OP_SET_STYLE].numargs = 2;

    handlerInfo[HIPE_OP_EVENT_REQUEST].ptrtype.withargs = handle_EVENT_REQUEST;
    handlerInfo[HIPE_OP_EVENT_REQUEST].numargs = 1;

    handlerInfo[HIPE_OP_EVENT_CANCEL].ptrtype.withargs = handle_EVENT_CANCEL;
    handlerInfo[HIPE_OP_EVENT_CANCEL].numargs = 2;

    handlerInfo[HIPE_OP_SCROLL_BY].ptrtype.withargs = handle_SCROLL_BY;
    handlerInfo[HIPE_OP_SCROLL_BY].numargs = 3;

    handlerInfo[HIPE_OP_SCROLL_TO].ptrtype.withargs = handle_SCROLL_TO;
    handlerInfo[HIPE_OP_SCROLL_TO].numargs = 3;

    handlerInfo[HIPE_OP_GET_ATTRIBUTE].ptrtype.withargs = handle_GET_ATTRIBUTE;
    handlerInfo[HIPE_OP_GET_ATTRIBUTE].numargs = 1;

    handlerInfo[HIPE_OP_FRAME_CLOSE].ptrtype.withargs = handle_FRAME_CLOSE;
    handlerInfo[HIPE_OP_FRAME_CLOSE].numargs = 1;

    handlerInfo[HIPE_OP_TAKE_SNAPSHOT].ptrtype.withargs = handle_TAKE_SNAPSHOT;
    handlerInfo[HIPE_OP_TAKE_SNAPSHOT].numargs = 2;

    handlerInfo[HIPE_OP_USE_CANVAS].ptrtype.withargs = handle_USE_CANVAS;
    handlerInfo[HIPE_OP_USE_CANVAS].numargs = 1;

    handlerInfo[HIPE_OP_CANVAS_ACTION].ptrtype.withargs = handle_CANVAS_ACTION;
    handlerInfo[HIPE_OP_CANVAS_ACTION].numargs = 2;

    handlerInfo[HIPE_OP_CANVAS_SET_PROPERTY].ptrtype.withargs = handle_CANVAS_SET_PROPERTY;
    handlerInfo[HIPE_OP_CANVAS_SET_PROPERTY].numargs = 2;

    handlerInfo[HIPE_OP_REMOVE_ATTRIBUTE].ptrtype.withargs = handle_REMOVE_ATTRIBUTE;
    handlerInfo[HIPE_OP_REMOVE_ATTRIBUTE].numargs = 1;

    handlerInfo[HIPE_OP_GET_CONTENT].ptrtype.withargs = handle_GET_CONTENT;
    handlerInfo[HIPE_OP_GET_CONTENT].numargs = 1;

    handlerInfo[HIPE_OP_CARAT_POSITION].ptrtype.withargs = handle_CARAT_POSITION;
    handlerInfo[HIPE_OP_CARAT_POSITION].numargs = 2;

    handlerInfo[HIPE_OP_FIND_TEXT].ptrtype.withargs = handle_FIND_TEXT;
    handlerInfo[HIPE_OP_FIND_TEXT].numargs = 1;

    handlerInfo[HIPE_OP_AUDIOVIDEO_STATE].ptrtype.withargs = handle_AUDIOVIDEO_STATE;
    handlerInfo[HIPE_OP_AUDIOVIDEO_STATE].numargs = 4;

    handlerInfo[HIPE_OP_DIALOG].ptrtype.withargs = handle_DIALOG;
    handlerInfo[HIPE_OP_DIALOG_INPUT].ptrtype.withargs = handle_DIALOG;
    handlerInfo[HIPE_OP_DIALOG].numargs = 4;
    handlerInfo[HIPE_OP_DIALOG_INPUT].numargs = 4;

    handlerInfo[HIPE_OP_DIALOG_RETURN].ptrtype.withargs = handle_DIALOG_RETURN;
    handlerInfo[HIPE_OP_DIALOG_RETURN].numargs = 4;    

    handlerInfo[HIPE_OP_GET_SELECTION].ptrtype.withargs = handle_GET_SELECTION;
    handlerInfo[HIPE_OP_GET_SELECTION].numargs = 1;

    handlerInfo[HIPE_OP_EDIT_ACTION].ptrtype.withargs = handle_EDIT_ACTION;
    handlerInfo[HIPE_OP_EDIT_ACTION].numargs = 1;

    handlerInfo[HIPE_OP_EDIT_STATUS].ptrtype.withargs = handle_EDIT_STATUS;
    handlerInfo[HIPE_OP_EDIT_STATUS].numargs = 1;

    handlerInfo[HIPE_OP_MESSAGE].ptrtype.withargs = handle_MESSAGE;
    handlerInfo[HIPE_OP_FIFO_ADD_ABILITY].ptrtype.withargs = handle_MESSAGE;
    handlerInfo[HIPE_OP_FIFO_REMOVE_ABILITY].ptrtype.withargs = handle_MESSAGE;
    handlerInfo[HIPE_OP_FIFO_OPEN].ptrtype.withargs = handle_MESSAGE;
    handlerInfo[HIPE_OP_FIFO_CLOSE].ptrtype.withargs = handle_MESSAGE;
    handlerInfo[HIPE_OP_FIFO_RESPONSE].ptrtype.withargs = handle_MESSAGE;
    handlerInfo[HIPE_OP_FIFO_DROP_PEER].ptrtype.withargs = handle_MESSAGE;
    handlerInfo[HIPE_OP_FIFO_GET_PEER].ptrtype.withargs = handle_MESSAGE;
    handlerInfo[HIPE_OP_OPEN_LINK].ptrtype.withargs = handle_MESSAGE;
    handlerInfo[HIPE_OP_MESSAGE].numargs = 4;
    handlerInfo[HIPE_OP_FIFO_ADD_ABILITY].numargs = 4;
    handlerInfo[HIPE_OP_FIFO_REMOVE_ABILITY].numargs = 4;
    handlerInfo[HIPE_OP_FIFO_OPEN].numargs = 4;
    handlerInfo[HIPE_OP_FIFO_CLOSE].numargs = 4;
    handlerInfo[HIPE_OP_FIFO_RESPONSE].numargs = 4;
    handlerInfo[HIPE_OP_FIFO_DROP_PEER].numargs = 4;
    handlerInfo[HIPE_OP_FIFO_GET_PEER].numargs = 4;
    handlerInfo[HIPE_OP_OPEN_LINK].numargs = 4;

}


void invoke_handler(Container* c, hipe_instruction* instruction, bool locationSpecified, QWebElement location) {
    short opcode = instruction->opcode;
    if(opcode >= MAX_OP_CODE || opcode <0) return; //out of range.
    if(!handlerInfo[opcode].ptrtype.noargs) return; //null function pointer.
    //(union means it doesn't matter what ptrtype we check for null condition)

    std::string args[HIPE_NARGS];
    short requestedArgs = handlerInfo[opcode].numargs;
    //pre-convert args if required
    for(short i=0; i<requestedArgs; i++) {
        args[i] = std::string(instruction->arg[i], instruction->arg_length[i]);
    }


    if(requestedArgs) {
        (handlerInfo[opcode].ptrtype.withargs)(c, instruction, locationSpecified, location, args);
    } else {
        (handlerInfo[opcode].ptrtype.noargs)(c, instruction, locationSpecified, location);
    }

}



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

void handle_SET_ICON(Container* c, hipe_instruction* instruction, bool, QWebElement) {
    c->setIcon(instruction->arg[0], instruction->arg_length[0]);
}


void handle_SET_SRC(Container*, hipe_instruction* instruction, bool, QWebElement location) {
    std::string mimetype(instruction->arg[1], instruction->arg_length[1]);
    
    std::string dataURI = std::string("data:") + mimetype + ";base64,"
                 + Sanitation::toBase64(instruction->arg[0], instruction->arg_length[0]);
    location.setAttribute("src", dataURI.c_str());
}

void handle_SET_STYLE_SRC(Container*, hipe_instruction* instruction, bool, QWebElement location) {
    std::string arg[4];
    arg[0] = std::string(instruction->arg[0], instruction->arg_length[0]);
    //don't convert arg[1] here since this may contain a lot of bytes.
    arg[2] = std::string(instruction->arg[2], instruction->arg_length[2]); //mime type
    arg[3] = std::string(instruction->arg[3], instruction->arg_length[3]); //supplementary value as suffix.

    std::string dataURI = std::string("data:") + arg[2] + ";base64," + Sanitation::toBase64(instruction->arg[1], instruction->arg_length[1]);
    if(Sanitation::isAllowedCSS(arg[0]))
        location.setStyleProperty(arg[0].c_str(), QString("url(\"") + dataURI.c_str() + "\") " + arg[3].c_str());
}

void handle_ADD_STYLE_RULE_SRC(Container* c, hipe_instruction* instruction, bool, QWebElement) {
    std::string css_specifier(instruction->arg[0], instruction->arg_length[0]);
    std::string dataURI = std::string("data:image/png;base64,") + Sanitation::toBase64(instruction->arg[1], instruction->arg_length[1]);
    
    if(Sanitation::isAllowedCSS(css_specifier))
        c->stylesheet += css_specifier + "{background-image:url(\"" + dataURI + "\");}\n";
    c->applyStylesheet();
}


void handle_GET_CARAT_POSITION(Container* c, hipe_instruction* instruction, bool, QWebElement location) {
    std::string selStart, selEnd;
    selStart = location.evaluateJavaScript("this.selectionStart;").toString().toStdString();
    selEnd = location.evaluateJavaScript("this.selectionEnd;").toString().toStdString();
    c->client->sendInstruction(HIPE_OP_CARAT_POSITION, instruction->requestor, 
            instruction->location, {selStart, selEnd});
}


void handle_GET_AUDIOVIDEO_STATE(Container* c, hipe_instruction* instruction, bool, QWebElement location) {
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

    c->client->sendInstruction(HIPE_OP_AUDIOVIDEO_STATE, instruction->requestor,
            instruction->location, {position, speed, (playing?"1":"0"), volume});

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


//REQUIRES 2 ARGS
void handle_TAKE_SNAPSHOT(Container* c, hipe_instruction* instruction, bool, QWebElement, std::string arg[]) {
    if(Sanitation::toLower(arg[0].c_str(), arg[0].size()) == "pdf") { //vector screenshot.
        QPrinter pdfGen(QPrinter::ScreenResolution);
        pdfGen.setOutputFormat(QPrinter::PdfFormat);
        pdfGen.setFontEmbeddingEnabled(true);
        pdfGen.setFullPage(true);
        //scale paper size to whichever resolution the QPrinter object is using:
        pdfGen.setPaperSize(QSizeF(c->webElement.webFrame()->contentsSize().width()/pdfGen.resolution(), 
                    c->webElement.webFrame()->contentsSize().height()/pdfGen.resolution()), QPrinter::Inch);
        QString snapshotFile = QString("/tmp/hipe-uid") + uid.c_str() + "_snapshot.pdf";
        pdfGen.setOutputFileName(snapshotFile);
        c->webElement.webFrame()->print(&pdfGen);

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
        payload.requestor = instruction->requestor;
        payload.location = instruction->location;
        if(success) {
            payload.arg[0] = fData;
            payload.arg_length[0] = size;
            payload.arg[1] = 0; payload.arg_length[1] = 0;
        } else {
            payload.arg[0] = 0; payload.arg_length[0] = 0;
            payload.arg[1] = (char*) "File error.";
            payload.arg_length[1] = 11;
        }

        c->client->sendInstruction(payload);
        free(fData);
        remove(snapshotFile.toStdString().c_str()); //delete the temporary file.
    }

}


//REQUIRES 1 ARG
void handle_USE_CANVAS(Container* c, hipe_instruction*, bool, QWebElement location, std::string arg[]) {
    arg[0] = Sanitation::sanitiseCanvasInstruction(arg[0]);
    c->webElement.evaluateJavaScript(QString("canvascontext=document.getElementById(\"")
                                  + location.attribute("id") + "\").getContext(\"" + arg[0].c_str() + "\");");
}


//REQUIRES 2 ARGS
void handle_CANVAS_ACTION(Container* c, hipe_instruction*, bool, QWebElement, std::string arg[]) {
    arg[0] = Sanitation::sanitiseCanvasInstruction(arg[0]);
    arg[1] = Sanitation::sanitiseCanvasInstruction(arg[1]);
    c->webElement.evaluateJavaScript(QString("canvascontext.") + arg[0].c_str() + "(" + arg[1].c_str() + ");");
}


//REQUIRES 2 ARGS
void handle_CANVAS_SET_PROPERTY(Container* c, hipe_instruction*, bool, QWebElement, std::string arg[]) {
    arg[0] = Sanitation::sanitiseCanvasInstruction(arg[0]);
    arg[1] = Sanitation::sanitiseCanvasInstruction(arg[1]);
    c->webElement.evaluateJavaScript(QString("canvascontext.") + arg[0].c_str() + "=" + arg[1].c_str() + ";");
}


//REQUIRES 1 ARG
void handle_REMOVE_ATTRIBUTE(Container*, hipe_instruction*, bool, QWebElement location, std::string arg[]) {
    if(Sanitation::isAllowedAttribute(arg[0]))
        location.removeAttribute(arg[0].c_str());
}


//REQUIRES 1 ARG
void handle_GET_CONTENT(Container* c, hipe_instruction* instruction, bool, QWebElement location, std::string arg[]) {
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
    c->client->sendInstruction(HIPE_OP_CONTENT_RETURN, instruction->requestor,
                                       instruction->location, {contentStr});
}


//REQUIRES 2 ARGS
void handle_CARAT_POSITION(Container*, hipe_instruction*, bool, QWebElement location, std::string arg[]) {
    //set the selection start/end position...
    //(note, this instruction may also be sent from Hipe to indicate a current carat position)
    arg[0] = Sanitation::sanitiseCanvasInstruction(arg[0]); //selection start
    arg[1] = Sanitation::sanitiseCanvasInstruction(arg[1]); //selection end, if specified
    if(!arg[1].size()) arg[1] = arg[0]; //if unspecified, end=start means cursor without selection.
    location.evaluateJavaScript(QString("this.setSelectionRange(")+arg[0].c_str()+","+arg[1].c_str()+");");
}


//REQUIRES 1 ARG
void handle_FIND_TEXT(Container* c, hipe_instruction*, bool, QWebElement, std::string arg[]) {
    if(c->isTopLevel) { //only the top level frame can use this instruction due
    //to a limitation in Qt; there is no findText() method for individual frames.
        ((ContainerTopLevel*)c)->findText(arg[0], false,false,false);
        //TODO: add support for search direction args, etc.
        //e.g. arg[1] might contain "bi" for 'backwards, case insensitive'
    }
}


//REQUIRES 4 ARGS
void handle_AUDIOVIDEO_STATE(Container*, hipe_instruction*, bool, QWebElement location, std::string arg[]) {
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
    if(arg[2].size()) {  //arg is specified.
        if(arg[2]=="1")
            location.evaluateJavaScript("this.play();");
        else if(arg[2]=="0")
            location.evaluateJavaScript("this.pause();");
    }

    //if arg[3] is non-null, take volume as a float.
    if(arg[3].size()) {
        if(sscanf(arg[3].c_str(), "%f", &argValue) == 1)
            location.evaluateJavaScript(QString("this.volume=")+QString::number(argValue)+";");
    }
}


//REQUIRES 4 ARGS - HANDLES BOTH HIPE_OP_DIALOG and HIPE_OP_DIALOG_INPUT
void handle_DIALOG(Container* c, hipe_instruction* instruction, bool, QWebElement, std::string arg[]) {

    Container* target = c->getParent();

    if(!target) { //we are the top level. Dialog is handled here directly
        bool cancelled;
        bool editable = (bool) (instruction->opcode == HIPE_OP_DIALOG_INPUT);

        std::string userChoice = ((ContainerTopLevel*)c)->dialog(arg[0], 
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
            c->client->sendInstruction(HIPE_OP_DIALOG_RETURN, instruction->requestor,
                            0, {userChoice, itemIndexStr});

        } else { //cancelled
            c->client->sendInstruction(HIPE_OP_DIALOG_RETURN, instruction->requestor, 0, {"","0"});
        }
    } else { //relay to parent frame.
        target->receiveMessage(instruction->opcode, instruction->requestor, 
                {arg[0],arg[1],arg[2],arg[3]}, c->webElement.webFrame(), false);
        //this sends the instruction to the parent's client.
    }
}


//REQUIRES 4 ARGS - HANDLES BOTH HIPE_OP_DIALOG
void handle_DIALOG_RETURN(Container* c, hipe_instruction* instruction, bool locationSpecified, QWebElement location, std::string arg[]) {
    if(locationSpecified) {
    //The fact we're receiving from the client and not sending this,
    //means a location is mandatory. We simply relay this instruction to the child frame.

        Container* target = nullptr;
        //find the relevant child frame client
        for(FrameData& fd : c->subFrames) {
            if(fd.we == location) { //found
                target = identifyFromFrame(fd.wf)->container; //find the corresponding container.
                break;
            }
        }

        if(target) {
            target->receiveMessage(HIPE_OP_DIALOG_RETURN, instruction->requestor, {arg[0],arg[1],
                    arg[2],arg[3]}, nullptr, false);
        }
    }
}


//REQUIRES 1 ARG
void handle_GET_SELECTION(Container* c, hipe_instruction* instruction, bool, QWebElement location, std::string arg[]) {
    std::string selectedText = ""; //if nothing is selected, or the functionality is
    //outside the client's purview to see, a blank string will be returned.
    if(arg[0] == "1") { //a top-level window has requested the global selection.
        if(c->isTopLevel) {
            selectedText = ((ContainerTopLevel*)c)->getGlobalSelection(false);
        }
    } else { //get local (this frame's) selection using javascript.
        selectedText = location.evaluateJavaScript("document.getSelection().toString();").toString().toStdString();
    }
    //return the contents of the selection...
    c->client->sendInstruction(HIPE_OP_CONTENT_RETURN, instruction->requestor,
                                       instruction->location, {selectedText});
}


//REQUIRES 1 ARG
void handle_EDIT_ACTION(Container* c, hipe_instruction*, bool, QWebElement, std::string arg[]) {
    if(c->isTopLevel) {
        ((ContainerTopLevel*)c)->triggerEditAction(arg[0][0]);
    }
}


//REQUIRES 1 ARG
void handle_EDIT_STATUS(Container* c, hipe_instruction* instruction, bool, QWebElement, std::string arg[]) {
    //for this op, the user specifies a string of edit function codes to check,
    //e.g. "xcvbiu" to check cut,copy,paste,bold,italic,underline.
    std::string resultingStates = "";
    char state;
    for(size_t i=0; i<arg[0].size(); i++) { //for each edit function to be checked
        state = c->editActionStatus(arg[0][i]);
        resultingStates += state;
    }
    c->client->sendInstruction(HIPE_OP_EDIT_STATUS, instruction->requestor,
                            instruction->location, {arg[0], resultingStates});
}


//REQUIRES 4 ARGS - handles several different instructions including FIFO_* and OPEN_LINK
void handle_MESSAGE(Container* c, hipe_instruction* instruction, bool locationSpecified, QWebElement location, std::string arg[]) {
    //Determine whether we need to send the message to the parent frame or a child frame.
    Container* target = nullptr;
    QWebFrame* sourceframe = nullptr;
    if(locationSpecified) {
        //find the relevant child frame client
        for(FrameData& fd : c->subFrames) {
            if(fd.we == location) { //found
                target = identifyFromFrame(fd.wf)->container; //find the corresponding container.
                break;
            }
        }
    } else { //send to parent element
        target = c->getParent();
        sourceframe = c->webElement.webFrame();
    }
    if(target) { //send the instruction to the destination. (at top level, target is nullptr)
        target->receiveMessage(instruction->opcode, instruction->requestor, 
                {arg[0], arg[1], arg[2], arg[3]}, sourceframe);
    } else if(instruction->opcode == HIPE_OP_FIFO_GET_PEER) {
    //special case for HIPE_OP_FIFO_GET_PEER instruction where a frame tries to
    //send it outside the top level. This would normally mean an application wishes
    //to import or export a file but is running in a top-level window in another
    //desktop environment. In this case, display an open/save dialog in order to
    //give the client application a real file to work with.

        std::string accessModeStr = arg[1];

        std::string filepath = ((ContainerTopLevel*)c)->selectFileResource(arg[0],
                   arg[2], accessModeStr);
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
        c->receiveMessage(HIPE_OP_FIFO_RESPONSE, instruction->requestor, 
                   {filepath, accessModeStr, filename, fileType}, nullptr);
        
    } else if(instruction->opcode == HIPE_OP_OPEN_LINK) {
    //special case for opening URL in user's browser if already running at top level.
        QDesktopServices::openUrl(QUrl(arg[0].c_str()));
    }
}
