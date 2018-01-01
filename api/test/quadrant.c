//quadrant: a simple framing manager divided into four panes, with the top-left pane reserved for system information.


#include <hipe.h>
#include <unistd.h> //for sleep function
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

hipe_session session;
int fAvailable[3]; //indicates which frames are available.
char hostkey[3][40]; //assume hostkeys won't be longer than 40 characters. A little risky perhaps but we should be OK.
hipe_loc keyDisp;

hipe_loc getLoc(char* id) {
    hipe_send(session, HIPE_OP_GET_BY_ID, 0, 0, 1, id); 
    hipe_instruction instruction;
    hipe_instruction_init(&instruction);
    hipe_await_instruction(session, &instruction, HIPE_OP_LOCATION_RETURN);
    return instruction.location;
}

hipe_loc getLastChild(hipe_loc parent) {
    hipe_send(session, HIPE_OP_GET_LAST_CHILD, 0, parent, 0); 
    hipe_instruction instruction;
    hipe_instruction_init(&instruction);
    hipe_await_instruction(session, &instruction, HIPE_OP_LOCATION_RETURN);
    return instruction.location;
}

void updateKey() {
    int i;
    for(i=0; i<3; i++) {
        if(fAvailable[i]) 
            break;
    }
    if(i==3) //session full.
        hipe_send(session, HIPE_OP_SET_TEXT, 0, keyDisp, 1, "[Session full]");
    else {
        hipe_send(session, HIPE_OP_SET_TEXT, 0, keyDisp, 1, hostkey[i]); //update display with new host key
        printf("Quadrant: new host key: %s\n", hostkey[i]);  //echo new host key to console
    }
}

int main(int argc, char** argv) {
    int i;

    printf("To take a vector screenshot, focus the top-right Quadrant frame and press Print Screen.\nThe screenshot will be saved in /tmp/.\n");

    //Request a new top-level application frame from the Hipe server
    session = hipe_open_session(argc>1 ? argv[1] : 0,0,0,"Quadrant");
    if(!session) return 1;


    //Specify initial screen contents

    hipe_send(session, HIPE_OP_ADD_STYLE_RULE, 0,0, 2, "html", "box-sizing:border-box;");
    hipe_send(session, HIPE_OP_ADD_STYLE_RULE, 0,0, 2, "*,*:before,*:after", "box-sizing: inherit;");
    hipe_send(session, HIPE_OP_ADD_STYLE_RULE, 0,0, 2, ".caption","position:absolute;width:12.5%;height:12.5%;color:white;overflow:hidden;text-align:right;padding:0.25em;visibility:hidden;border:1px outset");
    hipe_send(session, HIPE_OP_ADD_STYLE_RULE, 0,0, 2, ".frame","position:fixed;height:50%;width:50%;background-color:white;border:2px inset;");
    hipe_send(session, HIPE_OP_ADD_STYLE_RULE, 0,0, 2, ".frame.maximised","top:12.5% !important;height:87.5%;width:100%;z-index:99;");
    hipe_send(session, HIPE_OP_ADD_STYLE_RULE, 0,0, 2, ".caption.maximised","top:0 !important; left:0 !important; width:100%;");
    hipe_send(session, HIPE_OP_ADD_STYLE_RULE, 0,0, 2, "button","border-radius: 18%; font-size:small");
    hipe_send(session, HIPE_OP_ADD_STYLE_RULE, 0,0, 2, "button","margin:2px;");

    hipe_send(session, HIPE_OP_APPEND_TAG, 0,0, 1, "div");
    hipe_loc infoFrame = getLastChild(0);
    hipe_send(session, HIPE_OP_TOGGLE_CLASS, 0, infoFrame, 1, "frame");
    hipe_send(session, HIPE_OP_SET_STYLE, 0, infoFrame, 2, "top", "0");
    hipe_send(session, HIPE_OP_SET_STYLE, 0, infoFrame, 2, "right", "0");
    hipe_send(session, HIPE_OP_SET_STYLE, 0, infoFrame, 2, "overflow", "auto");
    hipe_send(session, HIPE_OP_APPEND_TAG,0, infoFrame, 1, "div");
    hipe_loc infoText = getLastChild(infoFrame); //contains information about quadrant environment
    hipe_send(session, HIPE_OP_SET_STYLE, 0, infoText, 2, "padding", "1em");
    hipe_send(session, HIPE_OP_SET_STYLE, 0, infoText, 2, "padding-bottom", "50%");
    hipe_send(session, HIPE_OP_SET_STYLE, 0, infoText, 2, "text-align", "right");
    hipe_send(session, HIPE_OP_APPEND_TEXT,0,infoText, 1, "Welcome to Quadrant: an unusual desktop\
        environment for Hipe. Quadrant can display up to three applications as fixed-size or maximised\
        tiles. To start one of the Hipe sample applications in this Quadrant session, start it with the\
        following key as its last command line argument:", 0);
    hipe_send(session, HIPE_OP_APPEND_TAG,0, infoText, 2, "hr", 0); //next key display
    hipe_send(session, HIPE_OP_APPEND_TAG,0, infoText, 2, "pre", "nextkeydisp"); //next key display

    hipe_send(session, HIPE_OP_APPEND_TAG, 0,0, 2, "iframe", "f0");
    hipe_send(session, HIPE_OP_APPEND_TAG, 0,0, 2, "iframe", "f1");
    hipe_send(session, HIPE_OP_APPEND_TAG, 0,0, 2, "iframe", "f2");
    hipe_send(session, HIPE_OP_APPEND_TAG, 0,0, 2, "div", "f0info");
    hipe_send(session, HIPE_OP_APPEND_TAG, 0,0, 2, "div", "f1info");
    hipe_send(session, HIPE_OP_APPEND_TAG, 0,0, 2, "div", "f2info");

    hipe_instruction instruction; //will use to store incomming instructions.
    hipe_instruction_init(&instruction);

    hipe_loc f[3];

    f[0] = getLoc("f0");
    hipe_send(session, HIPE_OP_SET_STYLE, 0, f[0], 2, "top", "0");
    hipe_send(session, HIPE_OP_SET_STYLE, 0, f[0], 2, "left", "0");
    hipe_send(session, HIPE_OP_SET_STYLE, 0, f[0], 2, "border-color", "blue");

    f[1] = getLoc("f1");
    hipe_send(session, HIPE_OP_SET_STYLE, 0, f[1], 2, "bottom", "0");
    hipe_send(session, HIPE_OP_SET_STYLE, 0, f[1], 2, "left", "0");
    hipe_send(session, HIPE_OP_SET_STYLE, 0, f[1], 2, "border-color", "red");

    f[2] = getLoc("f2");
    hipe_send(session, HIPE_OP_SET_STYLE, 0, f[2], 2, "bottom", "0");
    hipe_send(session, HIPE_OP_SET_STYLE, 0, f[2], 2, "right", "0");
    hipe_send(session, HIPE_OP_SET_STYLE, 0, f[2], 2, "border-color", "green");
    
    hipe_loc fInfo[3];

    fInfo[0] = getLoc("f0info");
    hipe_send(session, HIPE_OP_SET_STYLE, 0, fInfo[0], 2, "top", "25%");
    hipe_send(session, HIPE_OP_SET_STYLE, 0, fInfo[0], 2, "left", "50%");
    hipe_send(session, HIPE_OP_SET_STYLE, 0, fInfo[0], 2, "background-color", "blue");

    fInfo[1] = getLoc("f1info");
    hipe_send(session, HIPE_OP_SET_STYLE, 0, fInfo[1], 2, "top", "37.5%");
    hipe_send(session, HIPE_OP_SET_STYLE, 0, fInfo[1], 2, "left", "50%");
    hipe_send(session, HIPE_OP_SET_STYLE, 0, fInfo[1], 2, "background-color", "red");

    fInfo[2] = getLoc("f2info");
    hipe_send(session, HIPE_OP_SET_STYLE, 0, fInfo[2], 2, "top", "37.5%");
    hipe_send(session, HIPE_OP_SET_STYLE, 0, fInfo[2], 2, "left", "62.5%");
    hipe_send(session, HIPE_OP_SET_STYLE, 0, fInfo[2], 2, "background-color", "green");

    //set up the caption elements: title text, close button, maximise button.
    hipe_loc captionText[3];
    hipe_loc closeButton[3];
    hipe_loc maxButton[3];
    for(i=0; i<3; i++) {
        hipe_send(session, HIPE_OP_TOGGLE_CLASS, 0, f[i], 1, "frame");
        hipe_send(session, HIPE_OP_TOGGLE_CLASS, 0, fInfo[i], 1, "caption");

        hipe_send(session, HIPE_OP_APPEND_TAG, 0, fInfo[i], 1, "span");
        hipe_send(session, HIPE_OP_APPEND_TAG, 0, fInfo[i], 1, "button");
        hipe_send(session, HIPE_OP_APPEND_TAG, 0, fInfo[i], 1, "button");

        hipe_send(session, HIPE_OP_GET_FIRST_CHILD, 0, fInfo[i], 0);
        hipe_await_instruction(session, &instruction, HIPE_OP_LOCATION_RETURN);
        captionText[i] = instruction.location;

        hipe_send(session, HIPE_OP_GET_NEXT_SIBLING, 0, captionText[i], 0);
        hipe_await_instruction(session, &instruction, HIPE_OP_LOCATION_RETURN);
        closeButton[i] = instruction.location;
        hipe_send(session, HIPE_OP_SET_TEXT, 0, closeButton[i], 1, "&times;");
        hipe_send(session, HIPE_OP_SET_STYLE, 0, closeButton[i], 2, "position", "absolute");
        hipe_send(session, HIPE_OP_SET_STYLE, 0, closeButton[i], 2, "bottom", "0");
        hipe_send(session, HIPE_OP_SET_STYLE, 0, closeButton[i], 2, "left", "0");
        hipe_send(session, HIPE_OP_SET_STYLE, 0, closeButton[i], 2, "width", "2em");
        hipe_send(session, HIPE_OP_SET_STYLE, 0, closeButton[i], 2, "background-color", "darkred");
        hipe_send(session, HIPE_OP_SET_STYLE, 0, closeButton[i], 2, "border-color", "red");
        hipe_send(session, HIPE_OP_SET_STYLE, 0, closeButton[i], 2, "color", "white");
        hipe_send(session, HIPE_OP_SET_STYLE, 0, closeButton[i], 2, "font-weight", "bold");
        

        hipe_send(session, HIPE_OP_GET_LAST_CHILD, 0, fInfo[i], 0);
        hipe_await_instruction(session, &instruction, HIPE_OP_LOCATION_RETURN);
        maxButton[i] = instruction.location;
        hipe_send(session, HIPE_OP_SET_TEXT, 0, maxButton[i], 1, "+");
        hipe_send(session, HIPE_OP_SET_STYLE, 0, maxButton[i], 2, "position", "absolute");
        hipe_send(session, HIPE_OP_SET_STYLE, 0, maxButton[i], 2, "bottom", "0");
        hipe_send(session, HIPE_OP_SET_STYLE, 0, maxButton[i], 2, "left", "2em");
        hipe_send(session, HIPE_OP_SET_STYLE, 0, maxButton[i], 2, "width", "2em");
        hipe_send(session, HIPE_OP_SET_STYLE, 0, maxButton[i], 2, "background-color", "green");
        hipe_send(session, HIPE_OP_SET_STYLE, 0, maxButton[i], 2, "border-color", "lightgreen");
        hipe_send(session, HIPE_OP_SET_STYLE, 0, maxButton[i], 2, "color", "white");
        hipe_send(session, HIPE_OP_SET_STYLE, 0, maxButton[i], 2, "font-weight", "bold");

        //request events from the close and maximise buttons...
        hipe_send(session, HIPE_OP_EVENT_REQUEST, i, closeButton[i], 1, "click");
        hipe_send(session, HIPE_OP_EVENT_REQUEST, i, maxButton[i], 1, "click");

    }

    //request keyboard events...
    hipe_send(session, HIPE_OP_EVENT_REQUEST, 'K', 0, 1, "keydown");


    fAvailable[0] = 1;  //boolean flags indicate which frames are vacant for client connections.
    fAvailable[1] = 1;
    fAvailable[2] = 1;

    int fMaxed[3];
    fMaxed[0] = 0; //boolean flags indicate whether a frame is maximised.
    fMaxed[1] = 0;
    fMaxed[2] = 0;

    //Get hostkey of each frame
    for(i=0; i<3; i++) {
        hipe_send(session, HIPE_OP_GET_FRAME_KEY, 0, f[i], 0, 0); 
        hipe_await_instruction(session, &instruction, HIPE_OP_KEY_RETURN);

        strncpy(hostkey[i], instruction.arg[0], instruction.arg_length[0]);
        hostkey[i][instruction.arg_length[0]] = '\0'; //received arguments don't have null-terminators, so add one.
    }

    keyDisp = getLoc("nextkeydisp");
    updateKey();

    hipe_instruction hi;
    hipe_instruction_init(&hi); //each received instruction will be read into hi.
    while(1) { //Main event loop for Quadrant
        hipe_next_instruction(session, &hi, 1);
        if(hi.opcode == HIPE_OP_SERVER_DENIED) return 0; //main frame closed.
        if(hi.opcode == HIPE_OP_FRAME_EVENT) {
            if(hi.arg[0][0] == HIPE_FRAME_EVENT_CLIENT_CONNECTED || hi.arg[0][0] == HIPE_FRAME_EVENT_TITLE_CHANGED) {
                char newTitle[hi.arg_length[1]+1]; //copy the new title into a null-terminated string.
                strncpy(newTitle, hi.arg[1], hi.arg_length[1]);
                newTitle[hi.arg_length[1]] = '\0';
                for(i=0; i<3; i++) {
                    if(hi.location == f[i]) {
                        hipe_send(session, HIPE_OP_SET_TEXT, 0, captionText[i], 1, newTitle); //update the title.
                        hipe_send(session, HIPE_OP_SET_STYLE,0, fInfo[i], 2, "visibility","visible"); //make caption visible.
                        fAvailable[i] = 0;
                        hostkey[i][0] = '\0'; //clear the hostkey as it's been claimed.
                    }
                }
            } else if(hi.arg[0][0] == HIPE_FRAME_EVENT_CLIENT_DISCONNECTED) {
                for(i=0; i<3; i++) {
                    if(hi.location == f[i]) {
                        hipe_send(session, HIPE_OP_CLEAR, 0, captionText[i], 0); //blank the title display
                        hipe_send(session, HIPE_OP_SET_STYLE,0, fInfo[i], 2, "visibility","hidden"); //make caption invisible.
                        fAvailable[i] = 1; //now available.

                        if(fMaxed[i]) { //need to unmaximise the frame so the now-empty frame isn't obscuring the user's view.
                            hipe_send(session, HIPE_OP_TOGGLE_CLASS, 0, f[i], 1, "maximised");
                            hipe_send(session, HIPE_OP_TOGGLE_CLASS, 0, fInfo[i], 1, "maximised");
                            fMaxed[i] = 0;
                        }

                        //obtain new hostkey so this frame can be reused.
                        hipe_send(session, HIPE_OP_GET_FRAME_KEY, 0, f[i], 0, 0); 
                        hipe_await_instruction(session, &instruction, HIPE_OP_KEY_RETURN);
                        strncpy(hostkey[i], instruction.arg[0], instruction.arg_length[0]);
                        hostkey[i][instruction.arg_length[0]] = '\0'; //received arguments don't have null-terminators, so add one.
                    }
                }
            }

            //determine the next hostkey to display to the user
            if(hi.arg[0][0] == HIPE_FRAME_EVENT_CLIENT_CONNECTED || hi.arg[0][0] == HIPE_FRAME_EVENT_CLIENT_DISCONNECTED) {
                updateKey();
            }
        } else if(hi.opcode == HIPE_OP_EVENT) {
            i = hi.requestor;
            if(i == 'K') { //keyboard event.
                if(hi.arg_length[1]==2 && hi.arg[1][0] == '4' && hi.arg[1][1] == '4') { //44 is the keycode for PrintScreen
                    printf("Taking screenshot.\n");
                    hipe_send(session, HIPE_OP_TAKE_SNAPSHOT, 0,0, 1, "pdf");
                    hipe_await_instruction(session, &instruction, HIPE_OP_FILE_RETURN);
                    FILE* fp;
                    fp = fopen("/tmp/quadrant_screenshot.pdf", "w");
                    fwrite(instruction.arg[0], 1, instruction.arg_length[0], fp);
                    fclose(fp);
                }
            } else if(hi.location == closeButton[i]) {
                hipe_send(session, HIPE_OP_FRAME_CLOSE, 0, f[i], 0,0);
                //close the application
            } else if(hi.location == maxButton[i]) {
                fMaxed[i] = !fMaxed[i];
                hipe_send(session, HIPE_OP_TOGGLE_CLASS, 0, f[i], 1, "maximised");
                hipe_send(session, HIPE_OP_TOGGLE_CLASS, 0, fInfo[i], 1, "maximised");
            }
        } else if(hi.opcode == HIPE_OP_FRAME_CLOSE) { //close button clicked.
            return 0;
        }
    }
}


