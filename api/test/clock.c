/* Testbed for the Hipe API */

#include <hipe.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

int main(int argc, char** argv)
{
    time_t timestamp;
    struct tm* timeinfo;
    char formattedHHMM[6]; //hours and minutes string HH:MM
    char formattedSS[4];   //formatted seconds string

    //Request a new top-level application frame from the Hipe server
    hipe_session session = hipe_open_session(argc>1 ? argv[1] : 0,0,0,"Digital Clock");
    if(!session) exit(1);

    //Apply initial styling rules
    hipe_send(session, HIPE_OP_ADD_STYLE_RULE, 0,0, 2, "body","background-color:black; font-family:arial,sans-serif; color:green;");

    //Specify initial window contents

    hipe_send(session, HIPE_OP_APPEND_TAG, 0,0, 1, "div");

    //request a reference to the <div> element so we can play with it. 
    hipe_send(session, HIPE_OP_GET_LAST_CHILD, 0,0,0); 
    hipe_instruction instruction;
    hipe_instruction_init(&instruction);
    hipe_await_instruction(session, &instruction, HIPE_OP_LOCATION_RETURN);
    hipe_loc div_tag = instruction.location;

    //Populate the <div> tag with some styling.
    hipe_send(session, HIPE_OP_SET_STYLE, 0, div_tag, 2, "background-color", "#052205");
    hipe_send(session, HIPE_OP_SET_STYLE, 0, div_tag, 2, "color", "white");
    hipe_send(session, HIPE_OP_SET_STYLE, 0, div_tag, 2, "vertical-align", "top");
    hipe_send(session, HIPE_OP_SET_STYLE, 0, div_tag, 2, "border", "3px inset green");
    hipe_send(session, HIPE_OP_SET_STYLE, 0, div_tag, 2, "border-radius", "0.1em");
    hipe_send(session, HIPE_OP_SET_STYLE, 0, div_tag, 2, "bottom", "20%");
    hipe_send(session, HIPE_OP_SET_STYLE, 0, div_tag, 2, "left", "25%");
    hipe_send(session, HIPE_OP_SET_STYLE, 0, div_tag, 2, "position", "absolute");
    hipe_send(session, HIPE_OP_SET_STYLE, 0, div_tag, 2, "font-size", "400%");
    hipe_send(session, HIPE_OP_SET_STYLE, 0, div_tag, 2, "padding", "0.25em");
    hipe_send(session, HIPE_OP_SET_STYLE, 0, div_tag, 2, "text-shadow", "2px 2px green");
    
    //add span tags inside the div for the main hour-minute display and smaller second displays.
    hipe_send(session, HIPE_OP_APPEND_TAG, 0, div_tag, 1, "span");
    hipe_send(session, HIPE_OP_APPEND_TAG, 0, div_tag, 1, "span");

    hipe_send(session, HIPE_OP_GET_FIRST_CHILD, 0, div_tag,0); 
    hipe_instruction_clear(&instruction);
    hipe_await_instruction(session, &instruction, HIPE_OP_LOCATION_RETURN);
    hipe_loc big_digits = instruction.location;

    hipe_send(session, HIPE_OP_GET_LAST_CHILD, 0, div_tag,0); 
    hipe_instruction_clear(&instruction);
    hipe_await_instruction(session, &instruction, HIPE_OP_LOCATION_RETURN);
    hipe_loc small_digits = instruction.location;

    hipe_send(session, HIPE_OP_SET_STYLE, 0, small_digits, 2, "font-size", "50%");
    hipe_send(session, HIPE_OP_EVENT_REQUEST, 0, small_digits, 1, "mouseover");
    hipe_send(session, HIPE_OP_EVENT_REQUEST, 0, small_digits, 1, "click");
    //hipe_send(session, HIPE_OP_EVENT_CANCEL, 0, small_digits, 1, "mouseover");

    while(1) {
        time(&timestamp);
        timeinfo = localtime(&timestamp);
        strftime(formattedHHMM,6,"%H:%M",timeinfo);
        strftime(formattedSS,4,":%S",timeinfo);

        hipe_send(session, HIPE_OP_SET_TEXT, 0, big_digits, 1, formattedHHMM); 
        if(hipe_send(session, HIPE_OP_SET_TEXT, 0, small_digits, 1, formattedSS) == -1)
            exit(0); //instruction not sent -- disconnected from server.
        
        if(hipe_next_instruction(session, &instruction, 0)) {
            printf("Instruction/event received\n");
            if(instruction.opcode == HIPE_OP_FRAME_CLOSE)  //close button clicked.
                return 0;
        }

        usleep(500000);
    }
    return 0;
}
