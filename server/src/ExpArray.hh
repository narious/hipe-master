/*  Copyright (c) 2018 Daniel Kos, General Development Systems

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

//ExpArray and AutoExpArray.

//ExpArray abstract data type
//useful for arrays that need to be resized in O(1) time while preserving pointers to existing elements.
//otherwise similar to ordinary arrays, including (theoretical) O(1) access time to known element index.

//has size 1 initially.

//note that elements are numbered from [1]. There is no [0].

#pragma once

#include <cstdlib>
#include <stddef.h> //for size_t type


template <class T> class ExpArray {
private:
    T** data; //data is an array of T arrays
    short capacityMsb; //capacity of the most-significant-bit array
    short sizeMsb; //most significant bit position in the currently allocated array size. E.g. sizeMsb==0 means 2^0==1

    //returns the place of the most significant bit of the binary representation of num
    //with the least significant bit represented as zero (2^0 = 1)
    //if num==0, -1 is returned.
    short msbOf(size_t num) {
        short msbExponent = -1;
        while(num) {
            num >>= 1;
            msbExponent++;
        }
        return msbExponent;
    }
public:
    ExpArray() {
        capacityMsb=0;
        data = (T**) malloc((capacityMsb+1) * sizeof(T**));
        sizeMsb=0; //initial size is 1==2^0
        data[0] = new T[1](); //allocate a size 1 array to store the first element
    }

    ExpArray(const ExpArray&) {
    //copy constructor to prevent dangerous copying.
        ExpArray(); //just make a new blank array for now.
        //TODO: make a proper copy.
    }

    ~ExpArray() {
        while(sizeMsb>=0) shrink();
        free(data);
    }

    size_t size() { //returns the number of elements available
        return ((1<<sizeMsb)<<1) - 1; //subtract 1 because element [0] unused
    }

    size_t shrunkSize() { //returns what the size() will become if the array is shrunk with shrink()
        return ((1<<(sizeMsb-1))<<1) - 1; //same form as size() with sizeMsb decremented
    }

    void grow() { //approximately double the array size
        if(sizeMsb == capacityMsb) //need to grow the data array-of-arrays to add the next array
            data = (T**) realloc(data, (++capacityMsb+1) * sizeof(T));
        sizeMsb++;
        data[sizeMsb] = new T[1<<sizeMsb]();
    }

    void shrink() { //approximately halve the array size
        delete[] data[sizeMsb--];
    }

    //elements numbered from [1]. Element [0] makes no sense in the context of this data structure.
    //the 'by reference' return allows this to be used for both accessing and assigning values.
    T& operator[](size_t i) {
        short msb = msbOf(i);
        i &= ~(1<<msb); //remove most significant bit
        return data[msb][i];
    }

};


//AutoExpArray -- a user-friendlyness wrapper for ExpArray.
//Purpose: Allow user to worry about number of elements used, rather than current allocated capacity.
//Automatically increase the capacity if the user requests more elements than allocated, and shrink
//automatically too to conserve memory.


//New usage: Use inheritance rather than wrapping (save 4 bytes per instance).
//Instead of having addElement and delElement functions, use setSize to resize the number of 'used'
//elements dynamically. Behind the scenes, the array grows and shrinks in 2^n increments -- not
//every setSize results in a resize behind the scenes. (So elements may or may not be freed following
//a setSize call. At any rate, they should no longer be referenced.
//- base class functions grow() or shrink(), these are called automatically,
//and we insulate the user from these.



template <class T> class AutoExpArray : protected ExpArray<T> {
    private:
        size_t used; //the number of elements in use
    public:
        AutoExpArray() {
            used = 0;
        }

        size_t size() {
        //use ExpArray::size() for base class meaning
            return used;
        }

        void setSize(size_t newSize) {
            if(newSize > used) {
                while(newSize > ExpArray<T>::size())
                    ExpArray<T>::grow();
            } else { //determine if shrinking is worthwhile based on amortised cost as discussed in Katajainen and Mortensen
                if(newSize < (ExpArray<T>::size() >> 2))
                    ExpArray<T>::shrink();
            }
            used = newSize;
        }

        T& operator[](size_t i) {
            return ExpArray<T>::operator[](i);
        }
};
