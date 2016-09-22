/*  Copyright (c) 2016 Daniel Kos, General Development Systems

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


//Limitations:
//for simplicity, hipetec supports and manages a single session only.


#include <hipe.h>
#include <string>
#include <map>
#include <stdexcept>

namespace hipe {

class __internal {
    protected:
        hipe_session session=0;

        std::map<hipe_loc, size_t> referenceCounts; //local reference count for each location ID.
        //keep track of these counts so we can tell hipe to free resources that we no longer have references to.
        
        friend class session;
};

__internal _internal;

class loc;

class session {
//This class stores static (effectively global) variables that are hidden from users
//of this library
    private:
    protected:
        static void incrementReferenceCount(hipe_loc location) {
        //increments our local reference count for the location
            if(location == 0) return; //the body element is always 0, not requiring allocation.
            try { //try to increment referenceCounts.at(location)
            //the [] operator creates elements if they don't exist, while .at() throws exception.
                _internal.referenceCounts[location] = _internal.referenceCounts.at(location) + 1;
            } catch(const std::out_of_range& e) { //throws execption if reference nonexistent.
                _internal.referenceCounts[location] = 1;
            }
        }
    
        static void decrementReferenceCount(hipe_loc location) {
        //decrmements local reference count for a particular location, or frees the location
        //if its reference count is zero.
            if(location == 0) return; //the body element is always 0, not requiring allocation.
            size_t currentCount = _internal.referenceCounts[location];
            if(currentCount == 0) {
                //free this location.
                hipe_send(session::get_session(), HIPE_OPCODE_FREE_LOCATION, 0, location, 0,0);
            } else {
                _internal.referenceCounts[location] = currentCount-1;
            }
        }
    

    public:
        static bool open(const char* host_key, const char* socket_path, const char* key_path, const char* client_name) {
        //open a new hipe session
            _internal.session = hipe_open_session(host_key, socket_path, key_path, client_name);
            return (bool) _internal.session; //return true on success.
        }

        static void close() {
        //close the hipe session
            hipe_close_session(_internal.session);
            _internal.session = 0;
        }

        static hipe_session get_session() {
            return _internal.session;
        }

        friend class loc;
};


class loc {
private:
    hipe_loc location;
    
    
    loc(hipe_loc location) {
    //construct a new element object using its location value
        this->location = location;
        session::incrementReferenceCount(location);
    }
public:
    loc() {
    //create a new reference to the root element.
        location = 0;
    }

    loc(const loc& orig) {
    //copy constructor
        this->location = orig.location;
        session::incrementReferenceCount(location);
    }

    loc& operator= (const loc& orig) {
    //copy assignment operator
        session::decrementReferenceCount(location);
        this->location = orig.location;
        session::incrementReferenceCount(location);
    }
    
    ~loc() {
    //destructor. Should decrement the reference count to the hipe_loc allocation, and
    //send an instruction to free the allocation if the count reaches zero.
        session::decrementReferenceCount(location);
    }
    
    loc firstChild() {
    //return first child node of this element
        hipe_send(session::get_session(), HIPE_OPCODE_GET_FIRST_CHILD, 0, location, 0, 0);
        hipe_instruction instruction;
        hipe_instruction_init(&instruction);
        hipe_await_instruction(session::get_session(), &instruction, HIPE_OPCODE_LOCATION_RETURN);
        return loc(instruction.location);
    }
    
    loc lastChild() {
    //return the last child node of this element.
        hipe_send(session::get_session(), HIPE_OPCODE_GET_LAST_CHILD, 0, location, 0, 0);
        hipe_instruction instruction;
        hipe_instruction_init(&instruction);
        hipe_await_instruction(session::get_session(), &instruction, HIPE_OPCODE_LOCATION_RETURN);
        return loc(instruction.location);
    }
    
    loc nextSibling() {
        hipe_send(session::get_session(), HIPE_OPCODE_GET_NEXT_SIBLING, 0, location, 0, 0);
        hipe_instruction instruction;
        hipe_instruction_init(&instruction);
        hipe_await_instruction(session::get_session(), &instruction, HIPE_OPCODE_LOCATION_RETURN);
        return loc(instruction.location);
    }
    
    loc prevSibling() {
        hipe_send(session::get_session(), HIPE_OPCODE_GET_PREV_SIBLING, 0, location, 0, 0);
        hipe_instruction instruction;
        hipe_instruction_init(&instruction);
        hipe_await_instruction(session::get_session(), &instruction, HIPE_OPCODE_LOCATION_RETURN);
        return loc(instruction.location);
    }
    
    operator hipe_loc() const {
    //allow casting to a hipe_loc variable for use with hipe API C functions.
        return location;
    }
    

    int send(char opcode, uint64_t requestor, std::string arg1, std::string arg2) {
    //sends an instruction with this element passed as the location to act on.
        
        int result;
        hipe_instruction instruction;
        hipe_instruction_init(&instruction);
        instruction.opcode = opcode;
        instruction.requestor = requestor;
        instruction.location = location;
        if(arg1.size()) {
            instruction.arg1 = (char*) arg1.data();
            instruction.arg1Length = arg1.size();
        }
        if(arg2.size()) {
            instruction.arg2 = (char*) arg2.data();
            instruction.arg2Length = arg2.size();
        }
        result = hipe_send_instruction(session::get_session(), instruction);
        return result;
    }

};


};
