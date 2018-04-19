/*  Copyright (c) 2017 Daniel Kos, General Development Systems

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of this Software library.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
    THE SOFTWARE.
*/

//Idea: provide a C++ interface to Hipe where DOM elements are objects.
//A copy of a DOM element object still refers to the original, but when
//all copies are destroyed, the identifier is deallocated from Hipe.


//WRAP the C hipe API.


//an element object has methods used to call various 'navigation' instructions;
//to drill down to a child element, for example.


//we don't need to bother with event handling here -- the user can make use
//of the normal Hipe API.


#pragma once

#include <hipe.h>
#include <string>
#include <map>
#include <stdexcept>
#include <vector>

namespace hipe {

class session;

class loc {
///Provides an interface to managed hipe_loc objects.

    protected:
        hipe_loc location;
        session* _session;

        loc(hipe_loc location, session* s); //construct a new element object using its location value
        //This function is protected because exposing it may lead to incorrect reference count management.
    public:
        loc(); //create a new null instance to be reassigned later.
        loc(const loc& orig); //copy constructor

        loc(const hipe_loc& location);
        //create an instance that can be used for comparison only (no session or reference count info).

        loc& operator= (const loc& orig); //copy assignment operator

        loc& operator= (const hipe_loc& location);
        //assign a location that can be used for comparison only (no session or reference info).

        ~loc(); //destructor

        ///////////////////////////////////////////////////////////////////////

        loc firstChild(); //return first child element of this element
        loc lastChild(); //return the last child element of this element.
        loc nextSibling(); //return the element that follows this one at the current level.
        loc prevSibling(); //return the element that precedes this one at the current level.

        operator hipe_loc() const; //allow casting to a hipe_loc variable for use with hipe API C functions.
        bool operator== (hipe_loc) const; //allow comparison with hipe_loc location handle.
        bool operator== (const loc& loc) const; //allow comparison with another loc object.
        bool operator!= (const loc& loc) const; //allow comparison with another loc object.
        bool operator< (const loc& other) const;
        operator bool() const; //check if the loc object is well-defined.

        int send(char opcode, uint64_t requestor, const std::vector<std::string>& args={});
        //sends an instruction with this element passed as the location to act on.

};


class session : public loc {
//This class holds session information, and is also the root loc object,
//representing the <body> tag of the application's output.
    private:
        hipe_session hipeSession=0;

        std::map<hipe_loc, size_t> referenceCounts; //local reference count for each location ID.
        //keep track of these counts so we can tell hipe to free resources that we no longer have references to.
    protected:
        void incrementReferenceCount(hipe_loc location);
        //increments our local reference count for the location

        void decrementReferenceCount(hipe_loc location);
        //decrmements local reference count for a particular location, or frees the location
        //if its reference count is zero.

    public:
        session();

        bool open(const char* host_key, const char* socket_path, const char* key_path, const char* client_name);
        //open a new hipe session

        void close(); //close the hipe session

        operator hipe_session() const; //cast to the underlying hipe_session handle

        friend class loc;
};


///session class implementation
//////////////

inline session::session() : loc(0,this) {
}

inline void session::incrementReferenceCount(hipe_loc location) {
//increments our local reference count for the location
    if(location == 0) return; //the body element is always 0, not requiring allocation.
    try { //try to increment referenceCounts.at(location)
    //the [] operator creates elements if they don't exist, while .at() throws exception.
        referenceCounts[location] = referenceCounts.at(location) + 1;
    } catch(const std::out_of_range& e) { //throws execption if reference nonexistent.
        referenceCounts[location] = 1;
    }
}

inline void session::decrementReferenceCount(hipe_loc location) {
//decrmements local reference count for a particular location, or frees the location
//if its reference count is zero.
    if(location == 0) return; //the body element is always 0, not requiring allocation.
    size_t currentCount = referenceCounts[location];
    if(currentCount == 0) {
        //free this location.
        hipe_send((hipe_session) *this, HIPE_OP_FREE_LOCATION, 0, location, 0,0);
    } else {
        referenceCounts[location] = currentCount-1;
    }
}

inline bool session::open(const char* host_key, const char* socket_path, const char* key_path, const char* client_name) {
//open a new hipe session
    hipeSession = hipe_open_session(host_key, socket_path, key_path, client_name);
    return (bool) hipeSession; //return true on success.
}

inline void session::close() {
//close the hipe session
    hipe_close_session(hipeSession);
    hipeSession = 0;
}

inline session::operator hipe_session() const {
    return hipeSession;
}


///loc class implementation
//////////////

inline loc::loc(hipe_loc location, session* s) {
//construct a new element object using its location value
    this->location = location;
    this->_session = s;
    if(_session)
        _session->incrementReferenceCount(location);
}

inline loc::loc() {
//create a new loc instance set to null.
    location = 0;
    _session = 0;
}

inline loc::loc(const hipe_loc& location) {
//creates a loc instance that can be used for comparison purposes only.
//This allows a C hipe API location to be passed as a key to a map of hipe::loc elements.
    this->location = location;
    _session = 0;
}

inline loc::loc(const loc& orig) {
//copy constructor
    this->location = orig.location;
    _session = orig._session;
    if(_session)
        _session->incrementReferenceCount(location);
}

inline loc& loc::operator= (const loc& orig) {
//copy assignment operator
    if(&orig == this) return *this; //guard against self-assignment.
    if(_session)
        _session->decrementReferenceCount(location);
    this->location = orig.location;
    _session = orig._session;
    if(_session)
        _session->incrementReferenceCount(location);
    return *this;
}

inline loc& loc::operator= (const hipe_loc& location) {
    this->~loc(); //free what was.
    _session=0;
    this->location = location;
    return *this;
}

inline loc::~loc() {
//destructor. Should decrement the reference count to the hipe_loc allocation, and
//send an instruction to free the allocation if the count reaches zero.
    if(_session)
        _session->decrementReferenceCount(location);
}

inline loc loc::firstChild() {
//return first child node of this element
    hipe_send(*_session, HIPE_OP_GET_FIRST_CHILD, 0, location, 0, 0);
    hipe_instruction instruction;
    hipe_instruction_init(&instruction);
    hipe_await_instruction(*_session, &instruction, HIPE_OP_LOCATION_RETURN);
    hipe_loc location = instruction.location;
    hipe_instruction_clear(&instruction);
    return loc(location, _session);
}

inline loc loc::lastChild() {
//return the last child node of this element.
    hipe_send(*_session, HIPE_OP_GET_LAST_CHILD, 0, location, 0, 0);
    hipe_instruction instruction;
    hipe_instruction_init(&instruction);
    hipe_await_instruction(*_session, &instruction, HIPE_OP_LOCATION_RETURN);
    hipe_loc location = instruction.location;
    hipe_instruction_clear(&instruction);
    return loc(location, _session);
}

inline loc loc::nextSibling() {
    hipe_send(*_session, HIPE_OP_GET_NEXT_SIBLING, 0, location, 0, 0);
    hipe_instruction instruction;
    hipe_instruction_init(&instruction);
    hipe_await_instruction(*_session, &instruction, HIPE_OP_LOCATION_RETURN);
    hipe_loc location = instruction.location;
    hipe_instruction_clear(&instruction);
    return loc(location, _session);
}

inline loc loc::prevSibling() {
    hipe_send(*_session, HIPE_OP_GET_PREV_SIBLING, 0, location, 0, 0);
    hipe_instruction instruction;
    hipe_instruction_init(&instruction);
    hipe_await_instruction(*_session, &instruction, HIPE_OP_LOCATION_RETURN);
    hipe_loc location = instruction.location;
    hipe_instruction_clear(&instruction);
    return loc(location, _session);
}

inline loc::operator hipe_loc() const { //allow casting to a hipe_loc variable for use with hipe API C functions.
    return location;
}

inline int loc::send(char opcode, uint64_t requestor, const std::vector<std::string>& args) {
//sends an instruction with this element passed as the location to act on.

    int result;
    hipe_instruction instruction;
    hipe_instruction_init(&instruction);
    instruction.opcode = opcode;
    instruction.requestor = requestor;
    instruction.location = location;
    int i;
    for(i=0; i<args.size(); i++) {
        if(args[i].size()) {
            instruction.arg[i] = (char*) args[i].data();
            instruction.arg_length[i] = args[i].size();
        }
    }
    result = hipe_send_instruction(*_session, instruction);
    return result;
}

inline bool loc::operator== (hipe_loc other) const {
    return location == other;
}

inline bool loc::operator== (const loc& other) const {
    return location == other.location;
}

inline bool loc::operator!= (const loc& other) const {
    return location == other.location;
}

inline loc::operator bool() const {
//bool cast evaluates to true if the loc object is valid. A 'null' loc will evaluate to false.
    return (_session != 0); //note, location==0 would be valid as that's the root element.
}

inline bool loc::operator< (const loc& other) const {
//needed by C++ STL library for using loc as key in maps, etc.
//Note that it's valid to compare one loc with a defined session against another loc without a well-defined session instance.
    return (location < other.location);
}


};//end of hipe:: namespace
