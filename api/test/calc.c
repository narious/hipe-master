/* Calculator program to demonstrate an application displaying a GUI via the Hipe display server. */
/* Note: This calculator uses binary arithmetic, rather than decimal arithmetic. Therefore
   certain types of rounding errors inherant in floating point binary arithmetic are to be
   expected. */

#include <hipe.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

hipe_session session;
double currentValue;
double accumulator;
short placeValue;
hipe_loc mantissa;
short operation;

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

void updateDisplay() {
//update the onscreen representation of the current value
    char mantissaStr[16];
    snprintf(mantissaStr, 16, "%lf", currentValue);

    hipe_send(session, HIPE_OP_SET_TEXT, 0, mantissa, 1, mantissaStr); 
}

void crankTheHandle(short nextOperation) {
//run any pending operation and make the next operation pending.

    if(operation == '/')
        accumulator = accumulator / currentValue;
    else if(operation == '*')
        accumulator = accumulator * currentValue;
    else if(operation == '-')
        accumulator = accumulator - currentValue;
    else if(operation == '+')
        accumulator = accumulator + currentValue;
    else if(operation == '=')
        if(currentValue)
            accumulator = currentValue;
    
    operation = nextOperation;

    currentValue = accumulator;
    updateDisplay();

    currentValue = 0;
    placeValue = 1;
}

void appendToMantissa(short digit) {
//Precondition: digit is either a numerical digit 0 to 9, or the '.' character.
    int i; double frac;

    if(digit == '.') {
        if(placeValue == 1) placeValue = 0;
    } else if(placeValue == 1) {
        currentValue *= 10;
        currentValue += digit;
    } else {
        frac = digit;
        for(i=0; i>=placeValue; i--) {
            frac /= 10;
        }
        currentValue += frac;
        placeValue--;
    }

    //update the onscreen representation of the current value
    updateDisplay();
}


int main(int argc, char** argv)
{
    currentValue = accumulator = 0;
    placeValue = 1;
    operation = '=';

    //Request a new top-level application frame from the Hipe server
    session = hipe_open_session(argc>1 ? argv[1] : 0,0,0,"Calculator");
    if(!session) exit(1);

    //Apply initial styling rules
    hipe_send(session, HIPE_OP_ADD_STYLE_RULE, 0,0, 2, "body", "height:100%; margin:0; background-color:grey;padding:1% 1% 0 1%; overflow:hidden;");
    hipe_send(session, HIPE_OP_ADD_STYLE_RULE, 0,0, 2, "button", "height:100%; width:100%; display:block; font-size:large; color: black; background-color:dimgrey; box-shadow:-1px -1px 1px cyan; border-color:cadetblue; border-radius:3px; border-width:1px;font-size:170%;");
    hipe_send(session, HIPE_OP_ADD_STYLE_RULE, 0,0, 2, "button:active", "background-color:grey");
    hipe_send(session, HIPE_OP_ADD_STYLE_RULE, 0,0, 2, "table", "position:absolute;");
    hipe_send(session, HIPE_OP_ADD_STYLE_RULE, 0,0, 2, "td", "width:25%");
    hipe_send(session, HIPE_OP_ADD_STYLE_RULE, 0,0, 2, "#mantissa", "text-align:right; border:inset 2px; border-radius:5px; background-color:aquamarine;color:black;font-size:150%;");

    //Specify initial window layout
    hipe_send(session, HIPE_OP_APPEND_TAG, 0, 0, 1, "table");
    hipe_loc tableLoc = getLastChild(0);
    hipe_send(session, HIPE_OP_SET_STYLE, 0, tableLoc, 2, "visibility", "hidden"); //hide the table until constructed.

    hipe_send(session, HIPE_OP_APPEND_TAG, 0, tableLoc, 1, "tbody");
    hipe_loc tbodyLoc = getLastChild(tableLoc);
    hipe_send(session, HIPE_OP_APPEND_TAG, 0, tbodyLoc, 1, "tr");

    //first row of table layout shows the calculator's mantissa display
    hipe_loc rowLoc = getLastChild(tbodyLoc);
    hipe_send(session, HIPE_OP_APPEND_TAG, 0, rowLoc, 2, "td", "mantissa");
    mantissa = getLoc("mantissa");
    hipe_send(session, HIPE_OP_SET_ATTRIBUTE, 0, mantissa, 2, "colspan", "4");
    hipe_send(session, HIPE_OP_APPEND_TAG, 0, tbodyLoc, 1, "tr");
    hipe_send(session, HIPE_OP_FREE_LOCATION, 0, rowLoc, 0); //free the location reference to first row as we no longer need it.

    //second row of table layout displays first row of buttons: clear, clear entry, divide, times.
    rowLoc = getLastChild(tbodyLoc); //rowLoc now points to second row.
    hipe_send(session, HIPE_OP_APPEND_TAG, 0, rowLoc, 1, "td");
    hipe_send(session, HIPE_OP_APPEND_TAG, 0, getLastChild(rowLoc), 2, "button", "oC"); //C button.
    hipe_send(session, HIPE_OP_APPEND_TAG, 0, rowLoc, 1, "td");
    hipe_send(session, HIPE_OP_APPEND_TAG, 0, getLastChild(rowLoc), 2, "button", "oE"); //CE button.
    hipe_send(session, HIPE_OP_APPEND_TAG, 0, rowLoc, 1, "td");
    hipe_send(session, HIPE_OP_APPEND_TAG, 0, getLastChild(rowLoc), 2, "button", "oD"); //&div; button.
    hipe_send(session, HIPE_OP_APPEND_TAG, 0, rowLoc, 1, "td");
    hipe_send(session, HIPE_OP_APPEND_TAG, 0, getLastChild(rowLoc), 2, "button", "oT"); //&times; button.
    hipe_send(session, HIPE_OP_APPEND_TAG, 0, tbodyLoc, 1, "tr");
    hipe_send(session, HIPE_OP_FREE_LOCATION, 0, rowLoc, 0); //free the location reference to 2nd row as we no longer need it.

    //third row of table layout displays calculator buttons: 7, 8, 9, minus.
    rowLoc = getLastChild(tbodyLoc);
    hipe_send(session, HIPE_OP_APPEND_TAG, 0, rowLoc, 1, "td");
    hipe_send(session, HIPE_OP_APPEND_TAG, 0, getLastChild(rowLoc), 2, "button", "n7"); //7 button.
    hipe_send(session, HIPE_OP_APPEND_TAG, 0, rowLoc, 1, "td");
    hipe_send(session, HIPE_OP_APPEND_TAG, 0, getLastChild(rowLoc), 2, "button", "n8"); //8 button.
    hipe_send(session, HIPE_OP_APPEND_TAG, 0, rowLoc, 1, "td");
    hipe_send(session, HIPE_OP_APPEND_TAG, 0, getLastChild(rowLoc), 2, "button", "n9"); //9 button.
    hipe_send(session, HIPE_OP_APPEND_TAG, 0, rowLoc, 1, "td");
    hipe_send(session, HIPE_OP_APPEND_TAG, 0, getLastChild(rowLoc), 2, "button", "oM"); //&minus; button.
    hipe_send(session, HIPE_OP_APPEND_TAG, 0, tbodyLoc, 1, "tr");
    hipe_send(session, HIPE_OP_FREE_LOCATION, 0, rowLoc, 0); //free the location reference to 3rd row as we no longer need it.

    //fourth row of table layout displays calculator buttons: 4, 5, 6, plus.
    rowLoc = getLastChild(tbodyLoc);
    hipe_send(session, HIPE_OP_APPEND_TAG, 0, rowLoc, 1, "td");
    hipe_send(session, HIPE_OP_APPEND_TAG, 0, getLastChild(rowLoc), 2, "button", "n4"); //4 button.
    hipe_send(session, HIPE_OP_APPEND_TAG, 0, rowLoc, 1, "td");
    hipe_send(session, HIPE_OP_APPEND_TAG, 0, getLastChild(rowLoc), 2, "button", "n5"); //5 button.
    hipe_send(session, HIPE_OP_APPEND_TAG, 0, rowLoc, 1, "td");
    hipe_send(session, HIPE_OP_APPEND_TAG, 0, getLastChild(rowLoc), 2, "button", "n6"); //6 button.
    hipe_send(session, HIPE_OP_APPEND_TAG, 0, rowLoc, 1, "td");
    hipe_send(session, HIPE_OP_APPEND_TAG, 0, getLastChild(rowLoc), 2, "button", "oA"); //+ button.
    hipe_send(session, HIPE_OP_APPEND_TAG, 0, tbodyLoc, 1, "tr");
    hipe_send(session, HIPE_OP_FREE_LOCATION, 0, rowLoc, 0);

    //fifth row of table layout displays calculator buttons: 1, 2, 3, equals (spans 2 rows)
    rowLoc = getLastChild(tbodyLoc);
    hipe_send(session, HIPE_OP_APPEND_TAG, 0, rowLoc, 1, "td");
    hipe_send(session, HIPE_OP_APPEND_TAG, 0, getLastChild(rowLoc), 2, "button", "n1"); //1 button.
    hipe_send(session, HIPE_OP_APPEND_TAG, 0, rowLoc, 1, "td");
    hipe_send(session, HIPE_OP_APPEND_TAG, 0, getLastChild(rowLoc), 2, "button", "n2"); //2 button.
    hipe_send(session, HIPE_OP_APPEND_TAG, 0, rowLoc, 1, "td");
    hipe_send(session, HIPE_OP_APPEND_TAG, 0, getLastChild(rowLoc), 2, "button", "n3"); //3 button.
    hipe_send(session, HIPE_OP_APPEND_TAG, 0, rowLoc, 1, "td");
    hipe_send(session, HIPE_OP_SET_ATTRIBUTE,0,getLastChild(rowLoc), 2, "rowspan", "2"); //make = button span 2 rows.
    hipe_send(session, HIPE_OP_APPEND_TAG, 0, getLastChild(rowLoc), 2, "button", "oR"); //= button.
    hipe_send(session, HIPE_OP_APPEND_TAG, 0, tbodyLoc, 1, "tr");
    hipe_send(session, HIPE_OP_FREE_LOCATION, 0, rowLoc, 0);
    
    //sixth row of table layout displays calculator buttons: 0 (spans 2 cols), point.
    rowLoc = getLastChild(tbodyLoc);
    hipe_send(session, HIPE_OP_APPEND_TAG, 0, rowLoc, 1, "td");
    hipe_send(session, HIPE_OP_SET_ATTRIBUTE,0,getLastChild(rowLoc), 2, "colspan", "2"); //make 0 button span 2 rows.
    hipe_send(session, HIPE_OP_APPEND_TAG, 0, getLastChild(rowLoc), 2, "button", "n0"); //0 button.
    hipe_send(session, HIPE_OP_APPEND_TAG, 0, rowLoc, 1, "td");
    hipe_send(session, HIPE_OP_APPEND_TAG, 0, getLastChild(rowLoc), 2, "button", "oP"); //. button.
    hipe_send(session, HIPE_OP_FREE_LOCATION, 0, rowLoc, 0);

    //Get location handles for all the GUI elements so we can play with them.
    mantissa = getLoc("mantissa");
    hipe_loc bClear = getLoc("oC");
    hipe_loc bClearEntry = getLoc("oE");
    hipe_loc bDiv = getLoc("oD");
    hipe_loc bTimes = getLoc("oT");
    hipe_loc bMinus = getLoc("oM");
    hipe_loc bAdd = getLoc("oA");
    hipe_loc bPoint = getLoc("oP");
    hipe_loc bEquals = getLoc("oR");

    //label these buttons by setting content inside their <button> tags.
    hipe_send(session, HIPE_OP_SET_TEXT, 0, bClear, 1, "C");
    hipe_send(session, HIPE_OP_SET_TEXT, 0, bClearEntry, 1, "CE");
    hipe_send(session, HIPE_OP_SET_TEXT, 0, bDiv, 1, "&div;");
    hipe_send(session, HIPE_OP_SET_TEXT, 0, bTimes, 1, "&times;");
    hipe_send(session, HIPE_OP_SET_TEXT, 0, bMinus, 1, "&minus;");
    hipe_send(session, HIPE_OP_SET_TEXT, 0, bAdd, 1, "+");
    hipe_send(session, HIPE_OP_SET_TEXT, 0, bPoint, 1, ".");
    hipe_send(session, HIPE_OP_SET_TEXT, 0, bEquals, 1, "=");

    hipe_loc bNum[10]; //collect the number buttons in an array.
    int i;
    char idstr[3];
    idstr[0] = 'n'; idstr[2] = '\0';
    for(i=0; i<10; i++) { //loop through the numerical digits 0 to 9.
        idstr[1] = i+'0';
        bNum[i] = getLoc(idstr);
        //label the button with its index -- the number it represents. Use idstr[i] but skip the 0th character.
        hipe_send(session, HIPE_OP_SET_TEXT, 0, bNum[i], 1, &(idstr[1]));
    }

    //Stretch the table to fill the frame and make it visible now that the table has been fully constructed.
    hipe_send(session, HIPE_OP_SET_STYLE, 0, tableLoc, 2, "width", "98%");
    hipe_send(session, HIPE_OP_SET_STYLE, 0, tableLoc, 2, "height", "98%");
    hipe_send(session, HIPE_OP_SET_STYLE, 0, tableLoc, 2, "visibility", "visible"); //hide the table until constructed.

    //Now set up click events for all the buttons. Pass a character code mnemonic as requestor.
    hipe_send(session, HIPE_OP_EVENT_REQUEST, 'C', bClear,   2, "click", "");
    hipe_send(session, HIPE_OP_EVENT_REQUEST, 'E', bClearEntry,2,"click", "");
    hipe_send(session, HIPE_OP_EVENT_REQUEST, '/', bDiv,     2, "click", "");
    hipe_send(session, HIPE_OP_EVENT_REQUEST, '*', bTimes,   2, "click", "");
    hipe_send(session, HIPE_OP_EVENT_REQUEST, '-', bMinus,   2, "click", "");
    hipe_send(session, HIPE_OP_EVENT_REQUEST, '+', bAdd,     2, "click", "");
    hipe_send(session, HIPE_OP_EVENT_REQUEST, '.', bPoint,   2, "click", "");
    hipe_send(session, HIPE_OP_EVENT_REQUEST, '=', bEquals,  2, "click", "");
    
    //For digits, use the value of the digit as the requestor value.
    for(i=0; i<10; i++) {
        hipe_send(session, HIPE_OP_EVENT_REQUEST, i, bNum[i], 1, "click");
    }

    hipe_send(session, HIPE_OP_EVENT_REQUEST, 'K', 0, 1, "keypress");

    updateDisplay();

    hipe_instruction hi;
    hipe_instruction_init(&hi);
    while(1) { //Main event loop for calculator application.
        hipe_next_instruction(session, &hi, 1);
        if(hi.opcode == HIPE_OP_EVENT) {
            short action;
            if(hi.requestor == 'K') { //keyboard event
                char charcode = 0; //extract the character code string and turn it into a char.
                for(i=0; i<hi.arg_length[1]; i++) {
                    charcode *= 10;
                    charcode += hi.arg[1][i] - '0';
                }
                if(charcode >= '0' && charcode <= '9')
                    action = charcode - '0';
                else if(charcode == '\r' || charcode == '=') //enter or '=' pressed.
                    action = '=';
                else if(charcode == '.' || charcode == '+' || charcode == '-' 
                                        || charcode == '*' || charcode == '/')
                    action = charcode;
                else { //nonrecognised
                    continue;
                }
            } else { //mouse/other event
                action = hi.requestor;
            }

            if(action < 10 || action == '.') {
                appendToMantissa(action);
            } else if(action == 'C') {
                currentValue = accumulator = 0;
                placeValue = 1;
                operation = '=';
                updateDisplay();
            } else if(action == 'E') {
                currentValue = 0; 
                placeValue = 1;
                updateDisplay();
            } else {
                crankTheHandle(action);
            }
        } else if(hi.opcode == HIPE_OP_SERVER_DENIED) {
            return 0;
        } else if(hi.opcode == HIPE_OP_FRAME_CLOSE) { //close button clicked.
            return 0;
        }
    }
    return 0;
}
