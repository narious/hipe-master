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

    char editActionStatus(char action);
    void triggerEditAction(char action);

    std::string dialog(std::string title, std::string prompt, std::string choices, 
            bool editable, bool* cancelled);
    //choices is newline-separated. Displays modal dialog and returns result.
    //If the user selects cancel, the cancelled status is returned in a pointer argument.

    std::string selectFileResource(std::string defaultName, std::string metadata, std::string& accessMode);
    //implements top-level FIFO functionality through the OS's native open/save as interface.
    //Ordinary files are used instead of real FIFOs here.
    //The metadata argument spans multiple lines and contains caption and file filter info,
    //consistent with what's passed to HIPE_OP_FIFO_GET_PEER.

private:
    WebWindow* w;
protected:
    void setTitle(std::string newTitle);
    void setBody(std::string newBodyHtml, bool overwrite=true);
    void setIcon(const char* imgData, size_t length);

    QAction* getEditQtAction(char action);
    //utility function to convert our hipe EDIT action codes (e.g. 'x'==cut, 'c'==copy)
    //into a pointer to the corresponding QAction object with methods to trigger that
    //action and check its toggle state.
};

#endif // CLIENTWINDOW_H
