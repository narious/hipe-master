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

#include <QWebFrame>
#include <QWebPage>

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
    } else if(instruction.opcode == HIPE_OPCODE_APPEND_TAG) { //todo -- run thru an 'isTagAllowed()' function to eliminate tags like <script>.
        arg1 = sanitisePlainText(arg1);
        arg2 = sanitisePlainText(arg2);
        QString newTagString = "<";
        newTagString += arg1;
        if(arg2.size()) newTagString += " id=\"" + arg2 + "\"";
        newTagString += "></" + arg1 + ">";
        if(!locationSpecified) setBody(newTagString, false);
        location.appendInside(newTagString);
    } else if(instruction.opcode == HIPE_OPCODE_SET_TEXT) {
        arg1 = sanitisePlainText(arg1);
        if(!locationSpecified) setBody(arg1);
        location.setInnerXml(arg1);
    } else if(instruction.opcode == HIPE_OPCODE_APPEND_TEXT) {
        arg1 = sanitisePlainText(arg1);
        if(!locationSpecified) setBody(arg1, false);
        location.appendInside(arg1);
    } else if(instruction.opcode == HIPE_OPCODE_ADD_STYLE_RULE) {
        if(isAllowedCSS(arg1) && isAllowedCSS(arg2))
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
        if(isAllowedAttribute(arg1))
            location.setAttribute(arg1, sanitisePlainText(arg2));
    } else if(instruction.opcode == HIPE_OPCODE_SET_STYLE) {
        if(isAllowedCSS(arg1) && isAllowedCSS(arg2))
            location.setStyleProperty(arg1, arg2);
    } else if(instruction.opcode == HIPE_OPCODE_FREE_LOCATION) {
        removeReferenceableElement(instruction.location);
    } else if(instruction.opcode == HIPE_OPCODE_EVENT_REQUEST) {
        QString locStr = QString::number(instruction.location);
        QString reqStr = QString::number(requestor);
        location.setAttribute(QString("on") + arg1, QString("c.receiveGuiEvent(") + locStr + "," + reqStr + ",'" + arg1 + "',event.which)");
    } else if(instruction.opcode == HIPE_OPCODE_EVENT_CANCEL) {
        location.removeAttribute(QString("on") + arg1);
    } else if(instruction.opcode == HIPE_OPCODE_GET_GEOMETRY) {
        QRect geom = location.geometry();
        if(arg1.at(0) != '1') { //get position
            client->sendInstruction(HIPE_OPCODE_POSITION_RETURN, instruction.requestor, instruction.location,
                                    QString::number(geom.x()).toStdString(), QString::number(geom.y()).toStdString());
        } else { //get size
            client->sendInstruction(HIPE_OPCODE_POSITION_RETURN, instruction.requestor, instruction.location,
                                    QString::number(geom.width()).toStdString(), QString::number(geom.height()).toStdString());
        }
    } else if(instruction.opcode == HIPE_OPCODE_GET_ATTRIBUTE) {
        client->sendInstruction(HIPE_OPCODE_ATTRIBUTE_RETURN, instruction.requestor, instruction.location,
                                arg1.toStdString(), location.attribute(arg1).toStdString());
    } else if(instruction.opcode == HIPE_OPCODE_SET_SRC) {
        QString dataURI = QString("data:") + arg1 + ";base64,";

        QByteArray b64Data = QByteArray(instruction.arg2, instruction.arg2Length).toBase64();

        dataURI += QString::fromLocal8Bit(b64Data);
        location.setAttribute("src", dataURI);
    } else if(instruction.opcode == HIPE_OPCODE_GET_FRAME_KEY) {
        //Check if the location is already represented in the frame table.
        QString frameID = location.attribute("id"); //Need this for matching the frame. (Webkit seems to supply IDs automatically when none are provided by the user.)
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

QString Container::sanitisePlainText(QString input)
//Processes input string, replaces special HTML characters like < with their equivalent nonfunctional
//representations, like &lt;. This is used to prevent HTML injection attacks.
{
    QString output;
    for(int i=0; i<input.size(); i++) {
        if(input[i] == '<')
            output += "&lt;";
        else if(input[i] == '>')
            output += "&gt;";
        else if(input[i] == '"')
            output += "&quot;";
        else if(input[i] == '\'')
            output += "&#39;";
        else
            output += input[i];
    }
    return output;
}

bool Container::isAllowedAttribute(QString input)
//returns true iff use of the tag attribute is permitted by Hipe.
//List of attributes obtained from https://www.w3.org/TR/html4/index/attributes.html
//This is a whitelist of safe attributes that the user can use freely without special instructions.
{
    if(input == "abbr") return true;
    if(input == "accept-charset") return true;
    if(input == "accesskey") return true;
    if(input == "align") return true;
    if(input == "alt") return true;
    if(input == "border") return true;
    if(input == "cellpadding") return true;
    if(input == "cellspacing") return true;
    if(input == "char") return true;
    if(input == "charoff") return true;
    if(input == "checked") return true;
    if(input == "cols") return true;
    if(input == "colspan") return true;
    if(input == "coords") return true;
    if(input == "dir") return true;
    if(input == "disabled") return true;
    if(input == "for") return true;
    if(input == "frame") return true;
    if(input == "frameborder") return true;
    if(input == "headers") return true;
    if(input == "height") return true;
    if(input == "id") return true;
    if(input == "label") return true;
    if(input == "maxlength") return true;
    if(input == "multiple") return true;
    if(input == "name") return true;
    if(input == "noresize") return true;
    if(input == "readonly") return true;
    if(input == "rows") return true;
    if(input == "rowspan") return true;
    if(input == "rules") return true;
    if(input == "scope") return true;
    if(input == "scrolling") return true;
    if(input == "selected") return true;
    if(input == "shape") return true;
    if(input == "size") return true;
    if(input == "span") return true;
    if(input == "summary") return true;
    if(input == "tabindex") return true;
    if(input == "title") return true;
    if(input == "type") return true;
    if(input == "usemap") return true;
    if(input == "valign") return true;
    if(input == "value") return true;
    if(input == "width") return true;

    return false;
}

bool Container::isAllowedTag(QString input)
//List taken from https://www.w3.org/community/webed/wiki/HTML/Elements, with forbidden/nonapplicable tags deleted.
{
    if(input == "section") return true;
    if(input == "nav") return true;
    if(input == "article") return true;
    if(input == "aside") return true;
    if(input == "h1") return true;
    if(input == "h2") return true;
    if(input == "h3") return true;
    if(input == "h4") return true;
    if(input == "h5") return true;
    if(input == "h6") return true;
    if(input == "hgroup") return true;
    if(input == "header") return true;
    if(input == "footer") return true;
    if(input == "address") return true;
    if(input == "p") return true;
    if(input == "hr") return true;
    if(input == "pre") return true;
    if(input == "blockquote") return true;
    if(input == "ol") return true;
    if(input == "ul") return true;
    if(input == "li") return true;
    if(input == "dl") return true;
    if(input == "dt") return true;
    if(input == "dd") return true;
    if(input == "figure") return true;
    if(input == "figcaption") return true;
    if(input == "div") return true;
    if(input == "center") return true;
    if(input == "a") return true;
    if(input == "abbr") return true;
    if(input == "b") return true;
    if(input == "bdo") return true;
    if(input == "big") return true;
    if(input == "br") return true;
    if(input == "cite") return true;
    if(input == "code") return true;
    if(input == "dfn") return true;
    if(input == "em") return true;
    if(input == "i") return true;
    if(input == "kbd") return true;
    if(input == "mark") return true;
    if(input == "q") return true;
    if(input == "rp") return true;
    if(input == "rt") return true;
    if(input == "ruby") return true;
    if(input == "s") return true;
    if(input == "samp") return true;
    if(input == "small") return true;
    if(input == "spacer") return true;
    if(input == "span") return true;
    if(input == "strong") return true;
    if(input == "sub") return true;
    if(input == "sup") return true;
    if(input == "time") return true;
    if(input == "tt") return true;
    if(input == "u") return true;
    if(input == "var") return true;
    if(input == "wbr") return true;
    if(input == "ins") return true;
    if(input == "del") return true;
    if(input == "img") return true;
    if(input == "iframe") return true;
    if(input == "video") return true;
    if(input == "audio") return true;
    if(input == "source") return true;
    if(input == "track") return true;
    if(input == "canvas") return true; //will need to make an instruction for manipulating canvas.
    if(input == "map") return true;
    if(input == "area") return true;
    if(input == "svg") return true;
    if(input == "frame") return true;
    if(input == "frameset") return true;
    if(input == "table") return true;
    if(input == "caption") return true;
    if(input == "colgroup") return true;
    if(input == "col") return true;
    if(input == "tbody") return true;
    if(input == "thead") return true;
    if(input == "tfoot") return true;
    if(input == "tr") return true;
    if(input == "td") return true;
    if(input == "th") return true;
    if(input == "form") return true;
    if(input == "fieldset") return true;
    if(input == "legend") return true;
    if(input == "label") return true;
    if(input == "input") return true;
    if(input == "button") return true;
    if(input == "select") return true;
    if(input == "datalist") return true;
    if(input == "optgroup") return true;
    if(input == "option") return true;
    if(input == "textarea") return true;
    if(input == "output") return true;
    if(input == "progress") return true;
    if(input == "meter") return true;
    if(input == "details") return true;
    if(input == "summary") return true;
    if(input == "command") return true;
    if(input == "menu") return true;

    return false;
}

bool Container::isAllowedCSS(QString input)
{
    for(int i=0; i<input.size(); i++) {
        if(input[i] == '<') return false; //don't let the user break out of the stylesheet to inject html code.
        if(input[i] == '>') return false;
        if(input[i] == '{') return false; //the user isn't allowed to fill in a whole stylesheet directly, so has no need of these.
        if(input[i] == '}') return false;
        if((input[i] == 'u' || input[i] == 'U') && i+3 < input.size()) { //screen for URLs, which are not allowed to be entered directly.
            if((input[i+1] == 'r' || input[i+1] == 'R')
                    && (input[i+1] == 'l' || input[i+1] == 'L')) {
                i+=3;
                //we've detected an instance of the string "url".
                //at this point, we'll reject the input if there is a '(' or whitespace followed by a '('.
                while(i<input.size()) //skip any whitespace.
                    if(input[i].isSpace())
                        i++;
                if(i<input.size() && input[i] == '(') return false;
            }
        }
    }
    return true;
}


bool FrameData::operator==(const FrameData& other) {
    return (this->we == other.we);
}
