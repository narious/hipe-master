//Collection of instruction handler functions.
//The purpose of this code module is to reduce the size of the Container::receiveInstruction()
//function which currently contains all instruction handling in a monolithic way.

//two types of function signatures: some functions take an array of std::string args, 
//other handlers do not; either because they require the raw char array within the hipe_instruction
//or because they have trivial functionality.

//having common function signatures means we can replace a long if...else if block in Container:: with
//a fast map of function pointers down the track.

#ifndef _INSTRUCTIONHANDLER_H
#define _INSTRUCTIONHANDLER_H

#include "container.h"
#include "containertoplevel.h"



void initInstructionMap();
//populates a lookup data structure with mappings of instruction codes to their corresponding
//handler functions.
//This function should be called once only: when hiped starts up.
//Note: the mappings themselves are hard-coded into this function and should be updated
//when new functions are added.

void invoke_handler(Container* c, hipe_instruction* instruction, bool locationSpecified, QWebElement location);
//invokes the appropriate handler for the instruction.



void handle_CLEAR           (Container*, hipe_instruction*, bool locationSpecified, QWebElement location);
void handle_DELETE          (Container*, hipe_instruction*, bool locationSpecified, QWebElement location);
void handle_FREE_LOCATION   (Container*, hipe_instruction*, bool locationSpecified, QWebElement location);
void handle_GET_FIRST_CHILD (Container*, hipe_instruction*, bool locationSpecified, QWebElement location);
void handle_GET_LAST_CHILD  (Container*, hipe_instruction*, bool locationSpecified, QWebElement location);
void handle_GET_NEXT_SIBLING(Container*, hipe_instruction*, bool locationSpecified, QWebElement location);
void handle_GET_PREV_SIBLING(Container*, hipe_instruction*, bool locationSpecified, QWebElement location);
void handle_SET_FOCUS       (Container*, hipe_instruction*, bool locationSpecified, QWebElement location);
void handle_GET_GEOMETRY    (Container*, hipe_instruction*, bool locationSpecified, QWebElement location);
void handle_GET_SCROLL_GEOMETRY (Container*, hipe_instruction*, bool locationSpecified, QWebElement location);
void handle_GET_FRAME_KEY   (Container*, hipe_instruction*, bool locationSpecified, QWebElement location);
void handle_SET_ICON        (Container*, hipe_instruction*, bool locationSpecified, QWebElement location);
void handle_SET_SRC         (Container*, hipe_instruction*, bool locationSpecified, QWebElement location);
void handle_SET_STYLE_SRC   (Container*, hipe_instruction*, bool locationSpecified, QWebElement location);
void handle_ADD_STYLE_RULE_SRC(Container*, hipe_instruction*, bool locationSpecified, QWebElement location);
void handle_GET_CARAT_POSITION(Container*, hipe_instruction*, bool locationSpecified, QWebElement location);
void handle_GET_AUDIOVIDEO_STATE(Container*, hipe_instruction*, bool locationSpecified, QWebElement location);



//the following functions request C++ string conversions for neater string handling for a certain number of args.
//the remaining args can be taken out of the hipe_instruction struct regardless.
void handle_APPEND_TAG      (Container*, hipe_instruction*, bool locationSpecified, QWebElement location, std::string arg[]);
void handle_SET_TEXT        (Container*, hipe_instruction*, bool locationSpecified, QWebElement location, std::string arg[]);
void handle_APPEND_TEXT     (Container*, hipe_instruction*, bool locationSpecified, QWebElement location, std::string arg[]);
void handle_GET_BY_ID       (Container*, hipe_instruction*, bool locationSpecified, QWebElement location, std::string arg[]);
void handle_ADD_STYLE_RULE  (Container*, hipe_instruction*, bool locationSpecified, QWebElement location, std::string arg[]);
void handle_ADD_FONT        (Container*, hipe_instruction*, bool locationSpecified, QWebElement location, std::string arg[]);
void handle_SET_TITLE       (Container*, hipe_instruction*, bool locationSpecified, QWebElement location, std::string arg[]);
void handle_SET_ATTRIBUTE   (Container*, hipe_instruction*, bool locationSpecified, QWebElement location, std::string arg[]);
void handle_SET_STYLE       (Container*, hipe_instruction*, bool locationSpecified, QWebElement location, std::string arg[]);
void handle_EVENT_REQUEST   (Container*, hipe_instruction*, bool locationSpecified, QWebElement location, std::string arg[]);
void handle_EVENT_CANCEL    (Container*, hipe_instruction*, bool locationSpecified, QWebElement location, std::string arg[]);
void handle_SCROLL_BY       (Container*, hipe_instruction*, bool locationSpecified, QWebElement location, std::string arg[]);
void handle_SCROLL_TO       (Container*, hipe_instruction*, bool locationSpecified, QWebElement location, std::string arg[]);
void handle_GET_ATTRIBUTE   (Container*, hipe_instruction*, bool locationSpecified, QWebElement location, std::string arg[]);
void handle_FRAME_CLOSE     (Container*, hipe_instruction*, bool locationSpecified, QWebElement location, std::string arg[]);
void handle_TAKE_SNAPSHOT   (Container*, hipe_instruction*, bool locationSpecified, QWebElement location, std::string arg[]);
void handle_USE_CANVAS      (Container*, hipe_instruction*, bool locationSpecified, QWebElement location, std::string arg[]);
void handle_CANVAS_ACTION   (Container*, hipe_instruction*, bool locationSpecified, QWebElement location, std::string arg[]);
void handle_CANVAS_SET_PROPERTY(Container*, hipe_instruction*, bool locationSpecified, QWebElement location, std::string arg[]);
void handle_REMOVE_ATTRIBUTE(Container*, hipe_instruction*, bool locationSpecified, QWebElement location, std::string arg[]);
void handle_GET_CONTENT     (Container*, hipe_instruction*, bool locationSpecified, QWebElement location, std::string arg[]);
void handle_CARAT_POSITION(Container*, hipe_instruction*, bool locationSpecified, QWebElement location, std::string arg[]);
void handle_FIND_TEXT       (Container*, hipe_instruction*, bool locationSpecified, QWebElement location, std::string arg[]);
void handle_AUDIOVIDEO_STATE(Container*, hipe_instruction*, bool locationSpecified, QWebElement location, std::string arg[]);
void handle_DIALOG          (Container*, hipe_instruction*, bool locationSpecified, QWebElement location, std::string arg[]);
void handle_DIALOG_RETURN   (Container*, hipe_instruction*, bool locationSpecified, QWebElement location, std::string arg[]);
void handle_GET_SELECTION   (Container*, hipe_instruction*, bool locationSpecified, QWebElement location, std::string arg[]);
void handle_EDIT_ACTION     (Container*, hipe_instruction*, bool locationSpecified, QWebElement location, std::string arg[]);
void handle_EDIT_STATUS     (Container*, hipe_instruction*, bool locationSpecified, QWebElement location, std::string arg[]);
void handle_MESSAGE         (Container*, hipe_instruction*, bool locationSpecified, QWebElement location, std::string arg[]);



#endif