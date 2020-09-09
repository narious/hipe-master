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
#include "main.hpp"
#include <QWebView>
#include <QWebFrame>
#include <QSizePolicy>
#include <QWebElement>
#include <QPalette>
#include <QDesktopWidget>
#include <QApplication>
#include <QPixmap>

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
//'?' -- available and in fuzzy toggle state -- (not currently implemented)
//The returned status will depend on what element the user currently has
//focused and whether the user has selected content.

    bool enabled=false, toggled=false; 



    return '?'; //default catch-all

}

void ContainerTopLevel::triggerEditAction(char action) {
//the action to be done is specified by a char: 'x', 'c', 'v' or 'V'
    switch(action) {
    case 'x': //cut
        w->webView->page()->triggerAction(QWebPage::Cut);
        break;
    case 'c': //copy
        w->webView->page()->triggerAction(QWebPage::Copy);
        break;
    case 'v': //paste matching destination formatting
        w->webView->page()->triggerAction(QWebPage::PasteAndMatchStyle);
        break;
    case 'V': //paste preserving source formatting
        w->webView->page()->triggerAction(QWebPage::Paste);
        break;

    case 'b': //bold toggle
        w->webView->page()->triggerAction(QWebPage::ToggleBold);
        break;
    case 'i': //italic toggle
        w->webView->page()->triggerAction(QWebPage::ToggleItalic);
        break;
    case 'u': //underline toggle
        w->webView->page()->triggerAction(QWebPage::ToggleUnderline);
        break;
    case 'k': //strikethrough toggle
        w->webView->page()->triggerAction(QWebPage::ToggleStrikethrough);
        break;

    case 'l': //left align
        w->webView->page()->triggerAction(QWebPage::AlignLeft);
        break;
    case 'e': //center align
        w->webView->page()->triggerAction(QWebPage::AlignCenter);
        break;
    case 'r': //right align
        w->webView->page()->triggerAction(QWebPage::AlignRight);
        break;
    case 'j': //justified align
        w->webView->page()->triggerAction(QWebPage::AlignJustified);
        break;
    }

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
