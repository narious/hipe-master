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

/// Each client, and the global environment, holds a KeyList object.
/// It indexes a list of randomly generated keys which a client needs in order to become
/// parented by a particular host. A valid key can only be claimed once, afterwhich it's
/// removed from the pool. The KeyList object emits a signal when a key is claimed,
/// which can be bound to whatever function does the work of setting up the container
/// for the new client.

#ifndef KEYLIST_H
#define KEYLIST_H

#include <QObject>
#include <QMutex>
#include <list>

class KeyList : public QObject
{
    Q_OBJECT
private:
    static std::random_device rand;
    static unsigned int sequenceNumber;
    static QMutex sequenceGuard;

    std::string baseString;
    std::list<std::string> keys;
    QMutex listGuard;

    static char map6bitToAlphaNumeric(int num);

public:
    explicit KeyList(std::string baseString, QObject *parent = 0);
    std::string generateContainerKey();
    bool claimKey(std::string key);

signals:
    void keyGranted(std::string key);
public slots:

};

#endif // KEYLIST_H
