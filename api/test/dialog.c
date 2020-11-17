/* Test program to demonstrate dialog and related functionality
- Asking the framing manager (or Hipe if running at top level) to display a modal dialog on screen with multiple discrete options
- Requesting text input with list of suggested choices
- Selecting a FIFO resource to open.

*/


#include <hipe.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#define DIALOG_EVENT 1
#define TEXT_EVENT 2
#define OPEN_EVENT 3

hipe_session session;

hipe_loc getLoc(char* id) {
    hipe_send(session, HIPE_OP_GET_BY_ID, 0, 0, 1, id); 
    hipe_instruction instruction;
    hipe_instruction_init(&instruction);
    hipe_await_instruction(session, &instruction, HIPE_OP_LOCATION_RETURN);
    return instruction.location;
}

void showDialog() {
    hipe_send(session, HIPE_OP_DIALOG, 0,0, 3, "Dialog title", "Dialog prompt text", "Choice A\nChoice B\n\nChoice 1\nChoice 2\nChoice 3");
}

void showTextDialog() {
    hipe_send(session, HIPE_OP_DIALOG_INPUT, 0,0, 3, "Dialog title", "Dialog prompt text", "Default item\nrecent item 1\nrecent item 2\nrecent item 3");
}

void showFifoOpen() {
    hipe_send(session, HIPE_OP_FIFO_GET_PEER, 0,0, 3, "untitled.txt", "r", "Open file\n*:any file\njpg;jpeg;gif:graphic\n/:an entire directory (may not be supported)");
}

int main(int argc, char** argv)
{

    //Request a new top-level application frame from the Hipe server
    session = hipe_open_session(argc>1 ? argv[1] : 0,0,0,"Dialog demo");
    if(!session) exit(1);

    //Create and place buttons
    hipe_send(session, HIPE_OP_APPEND_TAG, 0,0, 2, "button", "dialogButton");
    hipe_send(session, HIPE_OP_APPEND_TAG, 0,0, 2, "button", "textDialogButton");
    hipe_send(session, HIPE_OP_APPEND_TAG, 0,0, 2, "button", "fifoOpenButton");

    hipe_send(session, HIPE_OP_APPEND_TAG, 0,0, 1, "hr"); //horizontal line
    
    hipe_send(session, HIPE_OP_APPEND_TEXT, 0,0, 2, "Dialog Demo\n"
        "This program allows you to test three kinds of interaction with the broader "
        "environment: Displaying a dialog box with multiple items, displaying a dialog "
        "prompt with free-form input, and requesting a FIFO resource, such as a file on disk. "
        "\n\nNote: the behaviour will differ depending on the environment this program runs "
        "in. If the application runs at the top level, Hipe itself will service the requests "
        "with system-standard prompts. If the application runs within a framing manager that "
        "supports these actions, the framing manager will manage the requests instead. Nothing "
        "happens if the framing manager does not implement these functions.",
        "1"
    );

    hipe_loc dialogButton = getLoc("dialogButton");
    hipe_loc textDialogButton = getLoc("textDialogButton");
    hipe_loc fifoOpenButton = getLoc("fifoOpenButton");

    //add text to the buttons
    hipe_send(session, HIPE_OP_APPEND_TEXT, 0,dialogButton, 1, "Show dialog");
    hipe_send(session, HIPE_OP_APPEND_TEXT, 0,textDialogButton, 1, "Show text input dialog");
    hipe_send(session, HIPE_OP_APPEND_TEXT, 0,fifoOpenButton, 1, "Open FIFO resource");

    //requests events for these buttons (we designate requestor codes 1,2,3 to identify these quickly)
    hipe_send(session, HIPE_OP_EVENT_REQUEST, DIALOG_EVENT, dialogButton,   1, "click");
    hipe_send(session, HIPE_OP_EVENT_REQUEST, TEXT_EVENT, textDialogButton,   1, "click");
    hipe_send(session, HIPE_OP_EVENT_REQUEST, OPEN_EVENT, fifoOpenButton,   1, "click");

    hipe_instruction event;
    hipe_instruction_init(&event);

    do {
        hipe_next_instruction(session, &event, 1);

        switch(event.requestor) {
            case DIALOG_EVENT: showDialog(); break;
            case TEXT_EVENT:   showTextDialog(); break;
            case OPEN_EVENT:   showFifoOpen(); break;
        }
    } while(event.opcode != HIPE_OP_FRAME_CLOSE); //repeat until window closed.


    return 0;
}
