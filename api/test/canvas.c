
#include <hipe.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

hipe_session session;

hipe_loc getLoc(char* id) {
    hipe_send(session, HIPE_OPCODE_GET_BY_ID, 0, 0, id, 0); 
    hipe_instruction instruction;
    hipe_instruction_init(&instruction);
    hipe_await_instruction(session, &instruction, HIPE_OPCODE_LOCATION_RETURN);
    return instruction.location;
}

int main(int argc, char** argv) {

    /*Request new application frame from Hipe display server.*/
    session = hipe_open_session(argc>1 ? argv[1] : 0,0,0,"canvas");
    if(!session) exit(1);

    hipe_send(session, HIPE_OPCODE_APPEND_TAG, 0,0, "canvas", "canv");
    hipe_send(session, HIPE_OPCODE_SET_STYLE, 0,0, "margin", "0");
    hipe_send(session, HIPE_OPCODE_SET_STYLE, 0,0, "background-color", "grey");
    hipe_loc canvas = getLoc("canv");
    hipe_send(session, HIPE_OPCODE_SET_STYLE, 0,canvas, "background-color", "white");
    hipe_send(session, HIPE_OPCODE_SET_STYLE, 0,canvas, "border", "2px inset");
    hipe_send(session, HIPE_OPCODE_SET_STYLE, 0,canvas, "height", "600");
    hipe_send(session, HIPE_OPCODE_SET_STYLE, 0,canvas, "width", "800");
    hipe_send(session, HIPE_OPCODE_SET_ATTRIBUTE, 0,canvas, "height", "600");
    hipe_send(session, HIPE_OPCODE_SET_ATTRIBUTE, 0,canvas, "width", "800");
    hipe_send(session, HIPE_OPCODE_USE_CANVAS, 0,canvas, "2d", 0); /*Set drawing context*/

    hipe_send(session, HIPE_OPCODE_EVENT_REQUEST, 0,canvas, "mouseup", "");
    hipe_send(session, HIPE_OPCODE_EVENT_REQUEST, 0,canvas, "mousemove", "");
    hipe_send(session, HIPE_OPCODE_EVENT_REQUEST, 0,canvas, "mousedown", "");

    /*Event loop*/
    hipe_instruction hi;
    hipe_instruction_init(&hi);
    int button, x, y;
    short nowDrawing = 0;
    char* xy;
    while(1) {
        hipe_next_instruction(session, &hi, 1);
        if(hi.opcode == HIPE_OPCODE_EVENT) {

            /*What kind of event?*/
            if(strncmp("mousemove", hi.arg1, hi.arg1Length) == 0
                       || strncmp("mousedown", hi.arg1, hi.arg1Length) == 0
                       || strncmp("mouseup", hi.arg1, hi.arg1Length) == 0 ) {
            /*Mouse event on canvas. Extract info.*/
                button = hi.arg2[0];
                xy = malloc(hi.arg2Length - 1);
                strncpy(xy, &(hi.arg2[2]), hi.arg2Length-2);
                xy[hi.arg2Length-2] = '\0'; /*null terminate.*/
                sscanf(xy, "%d,%d", &x, &y);

                if(button == '1' && !nowDrawing) {  /*start line.*/
                    hipe_send(session, HIPE_OPCODE_CANVAS_ACTION, 0,canvas, "beginPath", 0);
                    hipe_send(session, HIPE_OPCODE_CANVAS_ACTION, 0,canvas, "moveTo", xy);
                    nowDrawing = 1;
                } else if(button == '0' && nowDrawing) {
                    hipe_send(session, HIPE_OPCODE_CANVAS_ACTION, 0,canvas, "lineTo", xy);
                    hipe_send(session, HIPE_OPCODE_CANVAS_ACTION, 0,canvas, "stroke", 0);
                    nowDrawing = 0;
                } else if(nowDrawing) {
                    hipe_send(session, HIPE_OPCODE_CANVAS_ACTION, 0,canvas, "lineTo", xy);
                    hipe_send(session, HIPE_OPCODE_CANVAS_ACTION, 0,canvas, "stroke", 0);
                }

                free(xy);

            }
        }
    }
}

