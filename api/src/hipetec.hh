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



#include <hipe.h>
#include <string>
#include <map>
#include <stdexcept>

namespace hipe {



class loc {
private:
    hipe_session session;
    hipe_loc location;
    
    static std::map<hipe_loc, size_t> referenceCounts; //local reference count for each location ID.
    //keep track of these counts so we can tell hipe to free resources that we no longer have references to.
    
    static void incrementReferenceCount(hipe_loc location) {
    //increments our local reference count for the location
        try { //try to increment referenceCounts.at(location)
        //the [] operator creates elements if they don't exist, while .at() throws exception.
            referenceCounts[location] = referenceCounts.at(location) + 1;
        } catch(const std::out_of_range& e) { //throws execption if reference nonexistent.
            referenceCounts[location] = 1;
        }
    }
    
    static void decrementReferenceCount(hipe_loc location) {
    //decrmements local reference count for a particular location, or frees the location
    //if its reference count is zero.
        size_t currentCount = referenceCounts[location];
        if(currentCount == 0) {
            //TODO: free this location.
        } else {
            referenceCounts[location] = currentCount-1;
        }
    }
    

    loc(hipe_session session, hipe_loc location) {
    //construct a new element object using its location value
        this->session = session;
        this->location = location;
        incrementReferenceCount(location);
    }
public:
    loc(hipe_session session) {
    //create a new reference to the root element.
        this->session = session;
        location = 0;
    }
    
    ~loc() {
    //destructor. Should decrement the reference count to the hipe_loc allocation, and
    //send an instruction to free the allocation if the count reaches zero.
        if(location != 0)
            decrementReferenceCount(location);
    }
    
    loc firstChild() {
    //return first child node of this element
        hipe_send(session, HIPE_OPCODE_GET_FIRST_CHILD, 0, location, 0, 0);
        hipe_instruction instruction;
        hipe_instruction_init(&instruction);
        hipe_await_instruction(session, &instruction, HIPE_OPCODE_LOCATION_RETURN);
        return loc(session, instruction.location);
    }
    
    loc lastChild() {
    //return the last child node of this element.
        hipe_send(session, HIPE_OPCODE_GET_LAST_CHILD, 0, location, 0, 0);
        hipe_instruction instruction;
        hipe_instruction_init(&instruction);
        hipe_await_instruction(session, &instruction, HIPE_OPCODE_LOCATION_RETURN);
        return loc(session, instruction.location);
    }
    
    loc nextSibling() {
        hipe_send(session, HIPE_OPCODE_GET_NEXT_SIBLING, 0, location, 0, 0);
        hipe_instruction instruction;
        hipe_instruction_init(&instruction);
        hipe_await_instruction(session, &instruction, HIPE_OPCODE_LOCATION_RETURN);
        return loc(session, instruction.location);
    }
    
    loc prevSibling() {
        hipe_send(session, HIPE_OPCODE_GET_PREV_SIBLING, 0, location, 0, 0);
        hipe_instruction instruction;
        hipe_instruction_init(&instruction);
        hipe_await_instruction(session, &instruction, HIPE_OPCODE_LOCATION_RETURN);
        return loc(session, instruction.location);
    }
    
    operator hipe_loc() const {
    //allow casting to a hipe_loc variable for use with hipe API C functions.
        return location;
    }
    

    int send(char* requestor, std::string arg1, std::string arg2) {
    //sends an instruction with this element passed as the location to act on.
        
        int result;
        hipe_instruction instruction;
        hipe_instruction_init(&instruction);
        instruction.opcode = opcode;
        instruction.requestor = requestor;
        instruction.location = location;
        if(arg1) {
            instruction.arg1 = arg1.data();
            instruction.arg1Length = arg1.size();
        }
        if(arg2) {
            instruction.arg2 = arg2.data();
            instruction.arg2Length = arg2.size();
        }
        result = hipe_send_instruction(session, instruction);
        return result;
    }

};

std::map<hipe_loc, size_t> loc::referenceCounts;





};
