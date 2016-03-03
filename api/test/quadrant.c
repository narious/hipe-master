//quadrant: a simple framing manager divided into four panes, with the top-left pane reserved for system information.


#include <hipe.h>
#include <unistd.h> //for sleep function
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

hipe_session session;


hipe_loc getLoc(char* id) {
    hipe_send(session, HIPE_OPCODE_GET_BY_ID, 0, 0, id, 0); 
    hipe_instruction instruction;
    hipe_instruction_init(&instruction);
    hipe_await_instruction(session, &instruction, HIPE_OPCODE_LOCATION_RETURN);
    return instruction.location;
}

hipe_loc getLastChild(hipe_loc parent) {
    hipe_send(session, HIPE_OPCODE_GET_LAST_CHILD, 0, parent, 0, 0); 
    hipe_instruction instruction;
    hipe_instruction_init(&instruction);
    hipe_await_instruction(session, &instruction, HIPE_OPCODE_LOCATION_RETURN);
    return instruction.location;
}

int main(int argc, char** argv) {
    int i;

    printf("To take a vector screenshot, focus the top-right Quadrant frame and press Print Screen.\nThe screenshot will be saved in /tmp/.\n");

    //Request a new top-level application frame from the Hipe server
    session = hipe_open_session(argc>1 ? argv[1] : 0,0,0,"Quadrant");
    if(!session) return 1;


    //Specify initial screen contents

    hipe_send(session, HIPE_OPCODE_ADD_STYLE_RULE, 0,0, "html", "box-sizing:border-box;");
    hipe_send(session, HIPE_OPCODE_ADD_STYLE_RULE, 0,0, "*,*:before,*:after", "box-sizing: inherit;");
    hipe_send(session, HIPE_OPCODE_ADD_STYLE_RULE, 0,0, ".caption","position:absolute;width:12.5%;height:12.5%;color:white;overflow:hidden;text-align:right;padding:0.25em;visibility:hidden;border:1px outset");
    hipe_send(session, HIPE_OPCODE_ADD_STYLE_RULE, 0,0, ".frame","position:fixed;height:50%;width:50%;background-color:white;border:2px inset;");
    hipe_send(session, HIPE_OPCODE_ADD_STYLE_RULE, 0,0, ".frame.maximised","top:12.5% !important;height:87.5%;width:100%;z-index:99;");
    hipe_send(session, HIPE_OPCODE_ADD_STYLE_RULE, 0,0, ".caption.maximised","top:0 !important; left:0 !important; width:100%;");

    hipe_send(session, HIPE_OPCODE_APPEND_TAG, 0,0, "div", 0);
    hipe_loc infoFrame = getLastChild(0);
    hipe_send(session, HIPE_OPCODE_TOGGLE_CLASS, 0, infoFrame, "frame", 0);
    hipe_send(session, HIPE_OPCODE_SET_STYLE, 0, infoFrame, "top", "0");
    hipe_send(session, HIPE_OPCODE_SET_STYLE, 0, infoFrame, "right", "0");
    hipe_send(session, HIPE_OPCODE_SET_STYLE, 0, infoFrame, "overflow", "auto");
    hipe_send(session, HIPE_OPCODE_APPEND_TAG,0, infoFrame, "div", 0);
    hipe_loc infoText = getLastChild(infoFrame); //contains information about quadrant environment
    hipe_send(session, HIPE_OPCODE_SET_STYLE, 0, infoText, "padding", "1em");
    hipe_send(session, HIPE_OPCODE_SET_STYLE, 0, infoText, "padding-bottom", "50%");
    hipe_send(session, HIPE_OPCODE_SET_STYLE, 0, infoText, "text-align", "right");
    hipe_send(session, HIPE_OPCODE_APPEND_TEXT,0,infoText, "Welcome to Quadrant: an unusual desktop\
        environment for Hipe. Quadrant can display up to three applications as fixed-size or maximised\
        tiles. To start one of the Hipe sample applications in this Quadrant session, start it with the\
        following key as its last command line argument:", 0);
    hipe_send(session, HIPE_OPCODE_APPEND_TAG,0, infoText, "hr", 0); //next key display
    hipe_send(session, HIPE_OPCODE_APPEND_TAG,0, infoText, "pre", "nextkeydisp"); //next key display

    hipe_send(session, HIPE_OPCODE_APPEND_TAG, 0,0, "iframe", "f0");
    hipe_send(session, HIPE_OPCODE_APPEND_TAG, 0,0, "iframe", "f1");
    hipe_send(session, HIPE_OPCODE_APPEND_TAG, 0,0, "iframe", "f2");
    hipe_send(session, HIPE_OPCODE_APPEND_TAG, 0,0, "div", "f0info");
    hipe_send(session, HIPE_OPCODE_APPEND_TAG, 0,0, "div", "f1info");
    hipe_send(session, HIPE_OPCODE_APPEND_TAG, 0,0, "div", "f2info");

    hipe_instruction instruction; //will use to store incomming instructions.
    hipe_instruction_init(&instruction);

    hipe_loc f[3];

    f[0] = getLoc("f0");
    hipe_send(session, HIPE_OPCODE_SET_STYLE, 0, f[0], "top", "0");
    hipe_send(session, HIPE_OPCODE_SET_STYLE, 0, f[0], "left", "0");
    hipe_send(session, HIPE_OPCODE_SET_STYLE, 0, f[0], "border-color", "blue");

    f[1] = getLoc("f1");
    hipe_send(session, HIPE_OPCODE_SET_STYLE, 0, f[1], "bottom", "0");
    hipe_send(session, HIPE_OPCODE_SET_STYLE, 0, f[1], "left", "0");
    hipe_send(session, HIPE_OPCODE_SET_STYLE, 0, f[1], "border-color", "red");

    f[2] = getLoc("f2");
    hipe_send(session, HIPE_OPCODE_SET_STYLE, 0, f[2], "bottom", "0");
    hipe_send(session, HIPE_OPCODE_SET_STYLE, 0, f[2], "right", "0");
    hipe_send(session, HIPE_OPCODE_SET_STYLE, 0, f[2], "border-color", "green");
    
    hipe_loc fInfo[3];

    fInfo[0] = getLoc("f0info");
    hipe_send(session, HIPE_OPCODE_SET_STYLE, 0, fInfo[0], "top", "25%");
    hipe_send(session, HIPE_OPCODE_SET_STYLE, 0, fInfo[0], "left", "50%");
    hipe_send(session, HIPE_OPCODE_SET_STYLE, 0, fInfo[0], "background-color", "blue");

    fInfo[1] = getLoc("f1info");
    hipe_send(session, HIPE_OPCODE_SET_STYLE, 0, fInfo[1], "top", "37.5%");
    hipe_send(session, HIPE_OPCODE_SET_STYLE, 0, fInfo[1], "left", "50%");
    hipe_send(session, HIPE_OPCODE_SET_STYLE, 0, fInfo[1], "background-color", "red");

    fInfo[2] = getLoc("f2info");
    hipe_send(session, HIPE_OPCODE_SET_STYLE, 0, fInfo[2], "top", "37.5%");
    hipe_send(session, HIPE_OPCODE_SET_STYLE, 0, fInfo[2], "left", "62.5%");
    hipe_send(session, HIPE_OPCODE_SET_STYLE, 0, fInfo[2], "background-color", "green");

    //set up the caption elements: title text, close button, maximise button.
    hipe_loc captionText[3];
    hipe_loc closeButton[3];
    hipe_loc maxButton[3];
    for(i=0; i<3; i++) {
        hipe_send(session, HIPE_OPCODE_TOGGLE_CLASS, 0, f[i], "frame", 0);
        hipe_send(session, HIPE_OPCODE_TOGGLE_CLASS, 0, fInfo[i], "caption", 0);

        hipe_send(session, HIPE_OPCODE_APPEND_TAG, 0, fInfo[i], "span", 0);
        hipe_send(session, HIPE_OPCODE_APPEND_TAG, 0, fInfo[i], "button", 0);
        hipe_send(session, HIPE_OPCODE_APPEND_TAG, 0, fInfo[i], "button", 0);

        hipe_send(session, HIPE_OPCODE_GET_FIRST_CHILD, 0, fInfo[i], 0,0);
        hipe_await_instruction(session, &instruction, HIPE_OPCODE_LOCATION_RETURN);
        captionText[i] = instruction.location;

        hipe_send(session, HIPE_OPCODE_GET_NEXT_SIBLING, 0, captionText[i], 0,0);
        hipe_await_instruction(session, &instruction, HIPE_OPCODE_LOCATION_RETURN);
        closeButton[i] = instruction.location;
        hipe_send(session, HIPE_OPCODE_SET_TEXT, 0, closeButton[i], "&times;",0);
        hipe_send(session, HIPE_OPCODE_SET_STYLE, 0, closeButton[i], "position", "absolute");
        hipe_send(session, HIPE_OPCODE_SET_STYLE, 0, closeButton[i], "bottom", "0");
        hipe_send(session, HIPE_OPCODE_SET_STYLE, 0, closeButton[i], "left", "0");
        hipe_send(session, HIPE_OPCODE_SET_STYLE, 0, closeButton[i], "width", "2em");

        hipe_send(session, HIPE_OPCODE_GET_LAST_CHILD, 0, fInfo[i], 0,0);
        hipe_await_instruction(session, &instruction, HIPE_OPCODE_LOCATION_RETURN);
        maxButton[i] = instruction.location;
        hipe_send(session, HIPE_OPCODE_SET_TEXT, 0, maxButton[i], "+",0);
        hipe_send(session, HIPE_OPCODE_SET_STYLE, 0, maxButton[i], "position", "absolute");
        hipe_send(session, HIPE_OPCODE_SET_STYLE, 0, maxButton[i], "bottom", "0");
        hipe_send(session, HIPE_OPCODE_SET_STYLE, 0, maxButton[i], "left", "2em");
        hipe_send(session, HIPE_OPCODE_SET_STYLE, 0, maxButton[i], "width", "2em");

        //request events from the close and maximise buttons...
        hipe_send(session, HIPE_OPCODE_EVENT_REQUEST, i, closeButton[i], "click", 0);
        hipe_send(session, HIPE_OPCODE_EVENT_REQUEST, i, maxButton[i], "click", 0);

    }

    //request keyboard events...
    hipe_send(session, HIPE_OPCODE_EVENT_REQUEST, 'K', 0, "keydown", "");


    int fAvailable[3];
    fAvailable[0] = 1;  //boolean flags indicate which frames are vacant for client connections.
    fAvailable[1] = 1;
    fAvailable[2] = 1;

    int fMaxed[3];
    fMaxed[0] = 0; //boolean flags indicate whether a frame is maximised.
    fMaxed[1] = 0;
    fMaxed[2] = 0;

    char hostkey[3][40]; //assume hostkeys won't be longer than 40 characters. A little risky perhaps but we should be OK.

    //Get hostkey of each frame
    for(i=0; i<3; i++) {
        hipe_send(session, HIPE_OPCODE_GET_FRAME_KEY, 0, f[i], 0, 0); 
        hipe_await_instruction(session, &instruction, HIPE_OPCODE_KEY_RETURN);

        strncpy(hostkey[i], instruction.arg1, instruction.arg1Length);
        hostkey[i][instruction.arg1Length] = '\0'; //received arguments don't have null-terminators, so add one.
    }

    hipe_loc keyDisp = getLoc("nextkeydisp");
    hipe_send(session, HIPE_OPCODE_SET_TEXT, 0, keyDisp, hostkey[0], 0);

    hipe_instruction hi;
    hipe_instruction_init(&hi); //each received instruction will be read into hi.
    while(1) { //Main event loop for Quadrant
        hipe_next_instruction(session, &hi, 1);
        if(hi.opcode == HIPE_OPCODE_SERVER_DENIED) return 0; //main frame closed.
        if(hi.opcode == HIPE_OPCODE_FRAME_EVENT) {
            if(hi.arg1[0] == HIPE_FRAME_EVENT_CLIENT_CONNECTED || hi.arg1[0] == HIPE_FRAME_EVENT_TITLE_CHANGED) {
                char newTitle[hi.arg2Length+1]; //copy the new title into a null-terminated string.
                strncpy(newTitle, hi.arg2, hi.arg2Length);
                newTitle[hi.arg2Length] = '\0';
                for(i=0; i<3; i++) {
                    if(hi.location == f[i]) {
                        hipe_send(session, HIPE_OPCODE_SET_TEXT, 0, captionText[i], newTitle, 0); //update the title.
                        hipe_send(session, HIPE_OPCODE_SET_STYLE,0, fInfo[i], "visibility","visible"); //make caption visible.
                        fAvailable[i] = 0;
                        hostkey[i][0] = '\0'; //clear the hostkey as it's been claimed.
                    }
                }
            } else if(hi.arg1[0] == HIPE_FRAME_EVENT_CLIENT_DISCONNECTED) {
                for(i=0; i<3; i++) {
                    if(hi.location == f[i]) {
                        hipe_send(session, HIPE_OPCODE_CLEAR, 0, captionText[i], 0, 0); //blank the title display
                        hipe_send(session, HIPE_OPCODE_SET_STYLE,0, fInfo[i], "visibility","hidden"); //make caption invisible.
                        fAvailable[i] = 1; //now available.

                        if(fMaxed[i]) { //need to unmaximise the frame so the now-empty frame isn't obscuring the user's view.
                            hipe_send(session, HIPE_OPCODE_TOGGLE_CLASS, 0, f[i], "maximised",0);
                            hipe_send(session, HIPE_OPCODE_TOGGLE_CLASS, 0, fInfo[i], "maximised",0);
                            fMaxed[i] = 0;
                        }

                        //obtain new hostkey so this frame can be reused.
                        hipe_send(session, HIPE_OPCODE_GET_FRAME_KEY, 0, f[i], 0, 0); 
                        hipe_await_instruction(session, &instruction, HIPE_OPCODE_KEY_RETURN);
                        strncpy(hostkey[i], instruction.arg1, instruction.arg1Length);
                        hostkey[i][instruction.arg1Length] = '\0'; //received arguments don't have null-terminators, so add one.
                    }
                }
            }

            //determine the next hostkey to display to the user
            if(hi.arg1[0] == HIPE_FRAME_EVENT_CLIENT_CONNECTED || hi.arg1[0] == HIPE_FRAME_EVENT_CLIENT_DISCONNECTED) {
                for(i=0; i<3; i++) {
                    if(fAvailable[i]) {
                        hipe_send(session, HIPE_OPCODE_SET_TEXT, 0, keyDisp, hostkey[i], 0);
                        break;
                    }
                }
                if(i==3) //session full.
                    hipe_send(session, HIPE_OPCODE_SET_TEXT, 0, keyDisp, "[Session full]", 0);
            }
        } else if(hi.opcode == HIPE_OPCODE_EVENT) {
            i = hi.requestor;
            if(i == 'K') { //keyboard event.
                if(hi.arg2Length==2 && hi.arg2[0] == '4' && hi.arg2[1] == '4') { //44 is the keycode for PrintScreen
                    printf("Taking screenshot.\n");
                    hipe_send(session, HIPE_OPCODE_TAKE_SNAPSHOT, 0,0, "pdf", 0);
                    hipe_await_instruction(session, &instruction, HIPE_OPCODE_FILE_RETURN);
                    FILE* fp;
                    fp = fopen("/tmp/quadrant_screenshot.pdf", "w");
                    fwrite(instruction.arg1, 1, instruction.arg1Length, fp);
                    fclose(fp);
                }
            } else if(hi.location == closeButton[i]) {
                hipe_send(session, HIPE_OPCODE_FRAME_CLOSE, 0, f[i], 0,0);
                //close the application
            } else if(hi.location == maxButton[i]) {
                fMaxed[i] = !fMaxed[i];
                hipe_send(session, HIPE_OPCODE_TOGGLE_CLASS, 0, f[i], "maximised",0);
                hipe_send(session, HIPE_OPCODE_TOGGLE_CLASS, 0, fInfo[i], "maximised",0);
            }
        }
    }
}


