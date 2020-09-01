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

#ifndef CLIENTWINDOW_H
#define CLIENTWINDOW_H

#include <QMainWindow>
#include <QWebView>
#include <QWebElement>
#include <QCloseEvent>
#include "container.h"
class Client;


class WebView : public QWebView {
    Q_OBJECT
public:
    WebView();
protected:
    void contextMenuEvent(QContextMenuEvent*) {;} //reimplement to disable the default context menu
};

class WebWindow : public QMainWindow {
    Q_OBJECT
public:
    WebWindow(Container* cc);

    QWebElement initBoilerplate(std::string html);

    WebView* webView;
    QPalette pal;

private:
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
    explicit ContainerTopLevel(Connection*, std::string clientName);
    ~ContainerTopLevel();

    Container* getParent();

    bool findText(std::string userQuery, bool searchBackwards, bool wrapAtEnd, bool caseSensitive);
    std::string getGlobalSelection(bool asHtml);
    //if asHtml is false, the selection is returned as plain text.

    void triggerClipboardAction(char action);
private:
    WebWindow* w;
protected:
    void setTitle(std::string newTitle);
    void setBody(std::string newBodyHtml, bool overwrite=true);
    void setIcon(const char* imgData, size_t length);

};

#endif // CLIENTWINDOW_H
