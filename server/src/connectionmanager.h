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

#ifndef SERVER_H
#define SERVER_H

#include <QLocalServer>
class Connection;

class ConnectionManager : public QLocalServer
{
    Q_OBJECT
public:
    explicit ConnectionManager(QObject *parent = 0);
private:
signals:
public slots:
    void acceptConnection();
    void timerEvent(); //bound to the main event loop.
};

#endif // SERVER_H
