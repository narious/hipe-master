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

#include "containertoplevel.h"
#include "connection.h"
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

ContainerTopLevel::ContainerTopLevel(Connection* bridge, QString clientName) : Container(bridge, clientName) {
    w = new WebWindow(this);
    frame = w->webView->page()->mainFrame();
    connect(frame, SIGNAL(javaScriptWindowObjectCleared()), this, SLOT(frameCleared()));
    //frame->addToJavaScriptWindowObject("c",this);
    //make this container object accessible to the webview frame via javascript.
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
    if(!w->wasInitYet()) {
        webElement = w->initBoilerplate(std::string("<html><head><style>") + stylesheet + "</style><script>var canvascontext;</script></head><body  onkeydown=\"c.receiveKeyEventOnBody(false, event.which);\" onkeyup=\"c.receiveKeyEventOnBody(true, event.which);\"></body></html>"); //initialiser. If ommitted, resource images won't display (!)
        webElement.removeAllChildren();
    }
    if(overwrite) webElement.setInnerXml(newBodyHtml.c_str()); //remove any existing body as the user wishes to overwrite it.
    //c_str() conversion is adequate here since any binary data in the stylesheet will be expressed in base64 anyway.
    else webElement.appendInside(newBodyHtml.c_str());
}

void ContainerTopLevel::applyStylesheet() {
    if(!w->wasInitYet()) return; //no-op. Styles will be applied in the <head> when setBody is called.

    //appending new style rules after </head> is not supposed to be valid, but we might get away with it.
    webElement.appendInside(QString("<style>") + stylesheet.c_str() + "</style>");
    stylesheet = ""; //clear after application.
}

void ContainerTopLevel::setIcon(const char* imgData, size_t length)
{
    QPixmap iconData;
    iconData.loadFromData((const uchar*) imgData, (uint) length);
    QIcon icon(iconData);
    w->setWindowIcon(icon);
}


WebWindow::WebWindow(Container* cc)
{
    initYet = false;
    this->cc = cc;

    setWindowTitle("Hipe client");
    move(0,0); //in the absense of a window manager, this is a 'root' window that fills the screen.
    resize(desktop->screenGeometry().width(), desktop->screenGeometry().height());

    webView = new WebView();
    webView->setRenderHint(QPainter::Antialiasing);
    setCentralWidget(webView);

    //Disable network access and link navigation:
    webView->page()->settings()->setAttribute(QWebSettings::LocalContentCanAccessFileUrls, false);
    webView->page()->settings()->setAttribute(QWebSettings::JavascriptEnabled, true); //is there a way to get events without javascript?

    webView->page()->networkAccessManager()->setNetworkAccessible(QNetworkAccessManager::NotAccessible);
    webView->page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);

    webView->setFocus();

    //Don't display the window yet, until boilerplate markup is added in initBoilerplate.
    //This should reduce a little flicker (e.g. white background appears for an instant before being restyled.)
}

bool WebWindow::wasInitYet()
//used to check if the webview object was initialised with boilerplate code yet.
{
    return initYet;
}

QWebElement WebWindow::initBoilerplate(std::string html)
//If at least the boilerplate "<html><head></head><body></body></html>" is not specified,
//WebKit doesn't behave itself, for example Qt resource images do not display.
//RETURNS: the <body> element in the HTML boilerplate, where content can now be placed.
{
    webView->setHtml(html.c_str());
    showMaximized(); //Now we're ready to show the window on the screen.

    initYet = true;
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
