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
#include <QGraphicsView>
#include <QGraphicsWebView>
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

void ContainerTopLevel::setTitle(QString newTitle) {
    w->setWindowTitle(newTitle);
}

void ContainerTopLevel::setBody(QString newBodyHtml, bool overwrite) {
    if(!w->wasInitYet()) {
        webElement = w->initBoilerplate(QString("<html><head><style>") + stylesheet + "</style><script>var canvascontext;</script></head><body></body></html>"); //initialiser. If ommitted, resource images won't display (!)
        webElement.removeAllChildren();
    }
    if(overwrite) webElement.setInnerXml(newBodyHtml); //remove any existing body as the user wishes to overwrite it.
    else webElement.appendInside(newBodyHtml);
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

    QGraphicsView* gv;
    setCentralWidget(gv = new QGraphicsView);
    //clientWindow owns gv now and will delete it automatically.

    gv->setFrameShape(QFrame::NoFrame);
    scene = new QGraphicsScene(gv);
    gv->setScene(scene);
    gv->setBackgroundBrush(QBrush(Qt::red, Qt::FDiagPattern));
    //the red diagonal stripe pattern should never be seen. Indicates geometry is wrong.

    webView = new WebView();

    //Disable network access and link navigation:
    webView->page()->settings()->setAttribute(QWebSettings::LocalContentCanAccessFileUrls, false);
    webView->page()->settings()->setAttribute(QWebSettings::JavascriptEnabled, true); //is there a way to get events without javascript?

    webView->page()->networkAccessManager()->setNetworkAccessible(QNetworkAccessManager::NotAccessible);
    webView->page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);

    scene->addItem(webView);
    scene->setActiveWindow(webView);

    webView->setFocus();

    //Don't display the window yet, until boilerplate markup is added in initBoilerplate.
    //This should reduce a little flicker (e.g. white background appears for an instant before being restyled.)
}

bool WebWindow::wasInitYet()
//used to check if the webview object was initialised with boilerplate code yet.
{
    return initYet;
}

QWebElement WebWindow::initBoilerplate(QString html)
//If at least the boilerplate "<html><head></head><body></body></html>" is not specified,
//WebKit doesn't behave itself, for example Qt resource images do not display.
//RETURNS: the <body> element in the HTML boilerplate, where content can now be placed.
{
    webView->setHtml(html);
    initYet = true;
    QWebElement we = webView->page()->mainFrame()->documentElement();

    showMaximized(); //Now we're ready to show the window on the screen.

    return we.lastChild(); //body tag becomes the webElement in the base class.
}

void WebWindow::resizeEvent(QResizeEvent*) {
    webView->resize(centralWidget()->size());
    scene->setSceneRect(scene->itemsBoundingRect()); //crop the scene to the webView.
}

void WebWindow::closeEvent(QCloseEvent* event)
{
    cc->containerClosed();
    event->ignore();
}


WebView::WebView() : QGraphicsWebView()
{
    setRenderHint(QPainter::Antialiasing);
    setAutoFillBackground(true);
}
