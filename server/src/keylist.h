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
/// removed from the pool.

#ifndef KEYLIST_H
#define KEYLIST_H

#include <mutex>
#include <list>
#include <random>


class KeyList {
private:
    static std::random_device* rand;
    static unsigned int sequenceNumber;

    static std::mutex mKeyList; //used to make KeyList operations thread-safe.
    //rarity of this operation means a single mutex can be shared between instances.

    std::string baseString;
    std::list<std::string> keys;

    static char map6bitToAlphaNumeric(int num);

public:
    /*
     * Must be called before any objects are created from this class.
     * randomDevice is a hardware device path, e.g. "/dev/random" which is optionally specified.
     * If ommitted, default behaviour ensues.
     *
     * O(1) worst case time complexity (or whatever the complexity of rand_device is).
     */
    static void initClass(std::string randomDevice=""); 

    /*
     * O(1) worst case time complexity.
     */
    explicit KeyList(std::string baseString);

    /*
     * O(1) worst case time complexity because it doesn't do anything (redundant).
     */
    ~KeyList();
    /*
     * O(n+m) worst case time complexity where n is the length of the generated key and m is
     * the length of the list used to store the keys.
     */
    std::string generateContainerKey();
    /*
     * O(n*m) worst case time complexity where n is the length of the list of keys and m is the
     * length of a key.
     */
    bool claimKey(std::string key);
};

#endif // KEYLIST_H
