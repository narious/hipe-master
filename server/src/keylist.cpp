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

#include "keylist.h"
#include <sstream>
#include <QMutexLocker>

std::random_device KeyList::rand;
unsigned int KeyList::sequenceNumber = rand(); //who says we should start at zero? if it overflows it will wrap regardless.
QMutex KeyList::sequenceGuard;

KeyList::KeyList(std::string baseString, QObject *parent) :
    QObject(parent)
{
    this->baseString = baseString;
}

char KeyList::map6bitToAlphaNumeric(int num) {
//maps a number from 0-63 to a unique letter (case sensitive), number, tilde or underscore.
//PRECONDITION: 0 <= num < 64 (the value of num later gets incrememted out of this range to produce the return value)
    if(num < 10) return num+48; //character range 48-57 ('0'-'9')
    if(num < 36) return num+55; //character range 65-90 ('A'-'Z')
    if(num < 37) return num+59; //character 95 ('_')
    if(num < 63) return num+60; //character range 97-122 ('a'-'z')
    return num + 63;            //character 126 ('~')
}

std::string KeyList::generateContainerKey()
//takes a base string (such as the name of the host application) and produces from it a
//random string which aims to be something that never occurs again over the life of the
//process.
{
    /// key Ingredients:
    /// - the baseString, used as a prefix and followed by an underscore.
    /// - an integer which gets incremented each time this function is called, and is allowed to overflow.
    /// - a random number whose digits alternate with those of the integer.
    ///
    /// What we want to do with the latter two numbers is map them into other characters
    /// and produce a result where alternating odd and even characters correspond to the sequential
    /// and random integers respectively.
    /// We convert to base-64 and use map6bitToAlphaNumeric for this purpose.

    QMutexLocker nowLocking(&sequenceGuard); //get exclusivity to protect the sequence number and the rand object..

    unsigned int num1 = rand();
    unsigned int num2 = sequenceNumber++;  //increment it once each time this function is called.

    std::stringstream result;
    result << baseString;

    while(num1 || num2) {
        result << map6bitToAlphaNumeric(num1 % 64) << map6bitToAlphaNumeric(num2 % 64);
        num1 /= 64;
        num2 /= 64;
    }

    //The key string is formed as the concatenation of baseString, an alphanumeric
    //character sequence formed by incrementing a static variable,

    QMutexLocker nowLocking2(&listGuard);
    keys.push_front(result.str());

    return result.str();
}

bool KeyList::claimKey(std::string key)
{
    //1. search the list, 2. remove it from the list. 3. fire the signal.
    QMutexLocker nowLocking(&listGuard);

    for(std::string& k : keys) {
        if(k == key) {
            keyGranted(key);
            return true;
        }
    }
    return false;
}
