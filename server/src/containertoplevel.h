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

#ifndef CLIENTWINDOW_H
#define CLIENTWINDOW_H

#include <QMainWindow>
#include <QGraphicsWebView>
#include <QGraphicsScene>
#include <QWebElement>
#include "container.h"
class Client;


class WebView : public QGraphicsWebView {
    Q_OBJECT
public:
    WebView();
protected:
    void contextMenuEvent(QGraphicsSceneContextMenuEvent*) {;} //reimplement to disable the default context menu
};

class WebWindow : public QMainWindow {
    Q_OBJECT
public:
    WebWindow(Container* cc);

    bool wasInitYet();
    QWebElement initBoilerplate(QString html);

    QGraphicsScene* scene;
    WebView* webView;
    QPalette pal;

private:
    bool initYet; //becomes true once boilerplate html has been set.
    Container* cc;
public slots:
    void resizeEvent(QResizeEvent *);
    void closeEvent(QCloseEvent *);
};

//--------------------------
//THE CLASS YOU'RE HERE FOR:

class ContainerTopLevel : public Container
{

public:

    explicit ContainerTopLevel(Connection*, QString clientName);
    ~ContainerTopLevel();

private:
    WebWindow* w;
protected:
    void setTitle(QString newTitle);
    void setBody(QString newBodyHtml, bool overwrite=true);
    void setIcon(const char* imgData, size_t length);
};

#endif // CLIENTWINDOW_H
