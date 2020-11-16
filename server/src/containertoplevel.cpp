/*  Copyright (c) 2015-2020 Daniel Kos, General Development Systems

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

#include "containertoplevel.h"
#include "connection.h"
#include "icondata.h"
#include "sanitation.h"
#include "main.hpp"
#include <QWebView>
#include <QWebFrame>
#include <QSizePolicy>
#include <QWebElement>
#include <QPalette>
#include <QDesktopWidget>
#include <QApplication>
#include <QPixmap>
#include <QAction>
#include <QInputDialog>
#include <QFileDialog>
#include <QMessageBox>
#include <sstream>

#include <iostream>


//a client window wraps a WebView (from QGraphicsWebView) object.
//At the top level, clients that request new frames are granted
//system-level windows instead of frames managed by other clients.

ContainerTopLevel::ContainerTopLevel(Connection* bridge, std::string clientName) : Container(bridge, clientName) {
    isTopLevel = true;
    initYet = false;
    w = new WebWindow(this);
    frame = w->webView->page()->mainFrame();
    connect(frame, SIGNAL(javaScriptWindowObjectCleared()), this, SLOT(frameCleared()));
    //frame->addToJavaScriptWindowObject("c",this);
    //make this container object accessible to the webview frame via javascript.

    //set default toplevel icon to Hipe icon in case run in a desktop environment.
    setIcon((char*) hipeicon_rootless128_png, hipeicon_rootless128_png_len);
}

ContainerTopLevel::~ContainerTopLevel() {
    delete w;
}

Container* ContainerTopLevel::getParent()
{
    return nullptr; //a top level container has no parent.
}

void ContainerTopLevel::setTitle(std::string newTitle) {
    w->setWindowTitle(newTitle.c_str());
}

void ContainerTopLevel::setBody(std::string newBodyHtml, bool overwrite) {
    if(!initYet) {
        webElement = w->initBoilerplate(
            std::string("<html><head><style>")
            + stylesheet
            + "</style><script>var canvascontext;</script></head>"
            "<body onkeydown=\"c.receiveKeyEventOnBody(false, event.which);\" onkeyup=\"c.receiveKeyEventOnBody(true, event.which);\" ondragstart=\"return false\">"
            "</body></html>"
        ); //initialiser. If ommitted, resource images won't display (!)
        initYet = true;
        webElement.removeAllChildren();
    }
    if(overwrite) {
        webElement.setInnerXml(newBodyHtml.c_str());
        //remove any existing body as the user wishes to overwrite it.
        //c_str() conversion is adequate here since any binary data in the
        //stylesheet will be expressed in base64 anyway.
    } else {
        webElement.appendInside(newBodyHtml.c_str());
    }
}

void ContainerTopLevel::setIcon(const char* imgData, size_t length)
{
    QPixmap iconData;
    iconData.loadFromData((const uchar*) imgData, (uint) length);
    QIcon icon(iconData);
    w->setWindowIcon(icon);
}

bool ContainerTopLevel::findText(std::string userQuery, bool searchBackwards, bool wrapAtEnd, bool caseSensitive) {
    QWebPage::FindFlags flags = 0;
    if(searchBackwards) flags |= QWebPage::FindBackward;
    if(wrapAtEnd)       flags |= QWebPage::FindWrapsAroundDocument;
    if(caseSensitive)   flags |= QWebPage::FindCaseSensitively;
    flags |= QWebPage::HighlightAllOccurrences;
    return w->webView->findText(userQuery.c_str(), flags);
}

std::string ContainerTopLevel::getGlobalSelection(bool asHtml) {
    if(!asHtml) return w->webView->selectedText().toStdString();
    else return w->webView->selectedHtml().toStdString();
}

char ContainerTopLevel::editActionStatus(char action) {
//for a particular action (e.g. 'x' is cut, 'i' is italic toggle, etc.
//returns a char to indicate the status of that action:
//'0' -- available and not toggled
//'1' -- available and toggled
//'e' -- not enabled/not applicable in current context
//'?' -- status unavailable (applicable to non-top-level frames w/o permission)
//The returned status will depend on what element the user currently has
//focused and whether the user has selected content.

    auto actionObj = getEditQtAction(action);
    if(!actionObj->isEnabled()) return 'e'; //not enabled
    if(actionObj->isChecked()) return '1';
    else return '0';
}

void ContainerTopLevel::triggerEditAction(char action) {
//the action to be done is specified by a char: 'x', 'c', 'v' or 'V'
    auto actionObj = getEditQtAction(action);
    if(actionObj) actionObj->trigger();
}

QAction* ContainerTopLevel::getEditQtAction(char action) {
    return w->webView->page()->action(Sanitation::editCodeLookup(action));
}

std::string ContainerTopLevel::dialog(std::string title, std::string prompt, 
                    std::string choiceLines, bool editable, bool* cancelled) {
    QStringList items;
    QString separator = "⸻";
    //what if the user selects a separator? Treat it the same as a Cancel.

    if(choiceLines.size()) { //if choices are specified (technically required)
        
        items = ((QString)(choiceLines.c_str())).split("\n");

        //blank lines should contain a separator string.
        for(QString& s : items) {
            if(s == "")
                s = separator;
        }
    }

    bool ok;
    
    QString item = QInputDialog::getItem(NULL, QString(title.c_str()),
        QString(prompt.c_str()), items, 0, editable, &ok);

    if(ok && item != separator) {
        *cancelled = false;
        return item.toStdString();
    } else {
        *cancelled = true;
        return "";
    }
}


std::string ContainerTopLevel::selectFileResource(std::string defaultName, std::string metadata, std::string& accessMode) {
//accessModeStr will be modified by ref. to remove items not available. In particular, random seek modes are not
//available since we are dealing with native files (not a FIFO-host) and we do not intercept such requests.
//(The client does not know that these are native files and is thus forbidden from using calls like lseek)

    //check access modes and order of precedence... Only r and w are supported.
    bool r=false;
    bool w=false;
    size_t readPosition, writePosition;
    readPosition = accessMode.find("r");
    writePosition = accessMode.find("w");
    if(readPosition != std::string::npos) r = true;
    if(writePosition != std::string::npos) w = true;

    bool initialRead = r && (!w || readPosition < writePosition);

    if(!(r|w)) { //error, none of the specified access modes are r or w.
        QMessageBox::information(this->w, "Error", "Required access mode not supported");
        return "";
    }
    
    //parse out the metadata...
    std::stringstream ss(metadata); //allow reading incrementally as a stream
    std::string caption;
    std::getline(ss,caption); //the first line is the dialog caption. (src, dest)

    //now for the remaining lines in the metadata...
    std::string metaLine;
    std::string qtFilterStr;
    while(std::getline(ss,metaLine)) {

        //split into 2 strings by the Colon : -- left string is extension patterns. right string is description.
        size_t splitPosition = metaLine.find(":");
        std::string pattern, description;
        if(splitPosition != std::string::npos) { //":" was found
            pattern = metaLine.substr(0,splitPosition);
            description = metaLine.substr(splitPosition+1);
        } else { //":" was not found.
            pattern = metaLine; //the pattern must then be the whole thing.
            description = "";
        }

        std::string filterEntry; //in here, compose a filter entry in the format Qt requires.
        filterEntry = description + " (";

        //for extension patterns, split by semicolons. (e.g. "bmp;jpg;jpeg;gif")
        size_t startPosition=0; //the starting index of the next file type pattern
        size_t endPosition;
        do {
            endPosition = pattern.find(";",startPosition);
            if(endPosition == std::string::npos) endPosition = pattern.size();
            //technically we have found end position +1 (we don't want the semicolon or end of str itself)

            std::string patternSegment = pattern.substr(startPosition, endPosition-startPosition);

            //stick the patternSegment into Qt format and build it into the filter.
            filterEntry += "*." + patternSegment + " ";

            //next segment will start after endPosition;
            startPosition = endPosition+1;
        } while(startPosition < pattern.size());

        filterEntry += ")";

        //ignore patterns that contain "/" -- opening a directory this way is unsupported.

        //reformat back into a Qt style filter string: Description (*.aaa *.bbb);;Next filer (*.* *.abc);;...

        if(qtFilterStr.size() > 0) qtFilterStr += ";;";
        qtFilterStr += filterEntry;

    }


    QString filename;
    if(initialRead)
        filename = QFileDialog::getOpenFileName(this->w, caption.c_str(), defaultName.c_str(), qtFilterStr.c_str());
    else
        filename = QFileDialog::getSaveFileName(this->w, caption.c_str(), defaultName.c_str(), qtFilterStr.c_str());

    accessMode = (r && w) ? (initialRead ? "rw" : "wr") : (r ? "r" : "w");

    return filename.toStdString();

    //todo: return the absolute path rather than relative to hiped's cwd
}


WebWindow::WebWindow(Container* cc)
{
    this->cc = cc;

    setWindowTitle("Hipe client");

    if(fillscreen) {
    //in the absense of a window manager, this is a 'root' window that fills
    //the screen.
        move(0,0);
        resize(QApplication::desktop()->screenGeometry().width(),
                        QApplication::desktop()->screenGeometry().height());
    }

    webView = new WebView();
    webView->setRenderHint(QPainter::Antialiasing);
    setCentralWidget(webView);

    //Disable network access and link navigation:
    webView->page()->networkAccessManager()->setNetworkAccessible(QNetworkAccessManager::NotAccessible);
    webView->page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);

    webView->setFocus();

    //Don't display the window yet, until boilerplate markup is added in initBoilerplate.
    //This should reduce a little flicker (e.g. white background appears for an instant before being restyled.)
}

QWebElement WebWindow::initBoilerplate(std::string html)
//If at least the boilerplate "<html><head></head><body></body></html>" is not specified,
//WebKit doesn't behave itself, for example Qt resource images do not display.
//RETURNS: the <body> element in the HTML boilerplate, where content can now be placed.
{
    webView->setHtml(html.c_str());
    if(fillscreen) //Now we're ready to show the window on the screen.
        showFullScreen();
    else
        show();

    QWebElement we = webView->page()->mainFrame()->documentElement();

    return we.lastChild(); //body tag becomes the webElement in the base class.
}

void WebWindow::resizeEvent(QResizeEvent*) {
    webView->resize(centralWidget()->size());
}

void WebWindow::closeEvent(QCloseEvent* event)
{
    cc->containerClosed();
    event->ignore();
}


WebView::WebView() : QWebView()
{

}
