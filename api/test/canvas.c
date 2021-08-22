
#include <hipe.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

hipe_session session;

hipe_loc getLocById(char* id) {
    hipe_send(session, HIPE_OP_GET_BY_ID, 0, 0, 1, id); 
    hipe_instruction instruction;
    hipe_instruction_init(&instruction);
    hipe_await_instruction(session, &instruction, HIPE_OP_LOCATION_RETURN);
    return instruction.location;
}

int main(int argc, char** argv) {

    /*Request new application frame from Hipe display server.*/
    session = hipe_open_session(argc>1 ? argv[1] : 0,0,0,"canvas");
    if(!session) exit(1);

    hipe_send(session, HIPE_OP_SET_STYLE, 0,0, 2, "margin", "0");
    hipe_send(session, HIPE_OP_SET_STYLE, 0,0, 2, "background-color", "grey");
    hipe_send(session, HIPE_OP_APPEND_TAG, 0,0, 2, "canvas", "canv");
    hipe_loc canvas = getLocById("canv");
    hipe_send(session, HIPE_OP_SET_STYLE, 0,canvas, 2, "background-color", "white");
    hipe_send(session, HIPE_OP_SET_STYLE, 0,canvas, 2, "border", "2px inset");
    hipe_send(session, HIPE_OP_SET_STYLE, 0,canvas, 2, "height", "600");
    hipe_send(session, HIPE_OP_SET_STYLE, 0,canvas, 2, "width", "800");
    hipe_send(session, HIPE_OP_SET_ATTRIBUTE, 0,canvas, 2, "height", "600");
    hipe_send(session, HIPE_OP_SET_ATTRIBUTE, 0,canvas, 2, "width", "800");
    hipe_send(session, HIPE_OP_USE_CANVAS, 0,canvas, 2, "2d", 0); /*Set drawing context*/

    hipe_send(session, HIPE_OP_EVENT_REQUEST, 0,canvas, 1, "mouseup");
    hipe_send(session, HIPE_OP_EVENT_REQUEST, 0,canvas, 1, "mousemove");
    hipe_send(session, HIPE_OP_EVENT_REQUEST, 0,canvas, 1, "mousedown");

    /*Event loop*/
    hipe_instruction hi;
    hipe_instruction_init(&hi);
    int button, x, y;
    short nowDrawing = 0;
    char* xy;
    while(1) {
        hipe_next_instruction(session, &hi, 1);
        if(hi.opcode == HIPE_OP_EVENT) {

            /*What kind of event?*/
            if(strncmp("mousemove", hi.arg[0], hi.arg_length[0]) == 0
                       || strncmp("mousedown", hi.arg[0], hi.arg_length[0]) == 0
                       || strncmp("mouseup", hi.arg[0], hi.arg_length[0]) == 0 ) {
            /*Mouse event on canvas. Extract info.*/
                button = hi.arg[1][0];
                xy = malloc(hi.arg_length[1] - 1);
                strncpy(xy, &(hi.arg[1][2]), hi.arg_length[1]-2);
                xy[hi.arg_length[1]-2] = '\0'; /*null terminate.*/
                sscanf(xy, "%d,%d", &x, &y);

                if(button == '1' && !nowDrawing) {  /*start line.*/
                    hipe_send(session, HIPE_OP_CANVAS_ACTION, 0,0, 2, "beginPath", 0);
                    hipe_send(session, HIPE_OP_CANVAS_ACTION, 0,0, 2, "moveTo", xy);
                    nowDrawing = 1;
                } else if(button == '0' && nowDrawing) {
                    hipe_send(session, HIPE_OP_CANVAS_ACTION, 0,0, 2, "lineTo", xy);
                    hipe_send(session, HIPE_OP_CANVAS_ACTION, 0,0, 2, "stroke", 0);
                    nowDrawing = 0;
                } else if(nowDrawing) {
                    hipe_send(session, HIPE_OP_CANVAS_ACTION, 0,0, 2, "lineTo", xy);
                    hipe_send(session, HIPE_OP_CANVAS_ACTION, 0,0, 2, "stroke", 0);
                    hipe_send(session, HIPE_OP_CANVAS_ACTION, 0,0, 2, "beginPath", 0);
                    hipe_send(session, HIPE_OP_CANVAS_ACTION, 0,0, 2, "moveTo", xy);
                }

                free(xy);

            }
        } else if(hi.opcode == HIPE_OP_FRAME_CLOSE) { //close button clicked.
            return 0;
        }
    }
}

