/* Copyright (c) 2015-2018 Daniel Kos, General Development Systems

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

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Hipe commons visible to library users.
 **/

#ifndef HIPECOM_H
#define HIPECOM_H

#include <sys/types.h>
#include <stdint.h>

#define HIPE_OP_CLEAR              1
/*clear the contents of the tag given by location (removing all child elements),
 *or clear body if location==0.*/

#define HIPE_OP_SET_TEXT           2
/* set a tag's contents to a string of plain text given in arg[0], overwriting
 * previous contents.
 * if location==0 this applies to the entire body tag.
 */

#define HIPE_OP_APPEND_TEXT        3
/* append plain text inside the tag given by location, or inside body if location==0.
 * arg[0] is the text content to append. HTML-style special character entities may be used.
 */

#define HIPE_OP_APPEND_TAG         4
/* append a tag element inside the tag given by location, or inside body if location==0.
 * arg[0] is the tag type,
 * arg[1] is an optional identifier for the tag
 * arg[2] is an optional argument. If "1" is passed here, the server will send back a
 * HIPE_OP_LOCATION_RETURN instruction with the location of the newly created tag. */

#define HIPE_OP_ATTRIBUTE_RETURN   5
/* arg[0] is the name of the attribute and arg[1] is the retrieved value */

#define HIPE_OP_CONTAINER_GRANT    6
/* Server response to container request. Arg1 is "0" if the container request
 * was denied, or "1" if a new container has been granted. */

#define HIPE_OP_EVENT              7
/* Sent by the server to the client whenever a requested event occurs.
 * 'arg[0]' is the type of event, and 'location' is the tag for which events of 'arg[0]' were requested.
 * 'requestor' returns the same client-supplied value as was used for HIPE_OP_EVENT_REQUEST.
 * 'arg[1]' provides event-specific detail about the event -- e.g. for "click" events it returns the
 * number of clicks registered.
 * */

#define HIPE_OP_EVENT_CANCEL       8
/* Sent by the client to cancel notification of further events of type 'arg[0]' on location 'location'.
 * if arg[1]=="1", the server will reply with the same instruction to acknowledge that the event has been
 * cancelled. This may be useful to a client that needs to know when it is safe to clean up event listeners.
 * */

#define HIPE_OP_EVENT_REQUEST      9
/*Sent by the client to request notification of event 'arg[0]' on location 'location'.
 * */

#define HIPE_OP_FREE_LOCATION      10
/* Sent by the client to the server to de-allocate a location index that is no longer required.*/

#define HIPE_OP_GET_ATTRIBUTE      11
/* arg[0] is the name of the attribute*/

#define HIPE_OP_GET_BY_ID          12
/* Request a location index for the HTML tag with id attribute == arg[0]*/

#define HIPE_OP_GET_FIRST_CHILD    13

#define HIPE_OP_GET_GEOMETRY       14
/* Request a HIPE_OP_GEOMETRY reply with the x,y,width,height coordinates of location.*/

#define HIPE_OP_GET_LAST_CHILD     15
#define HIPE_OP_GET_NEXT_SIBLING   16
#define HIPE_OP_GET_PREV_SIBLING   17
/* Sent by client to request a location index relative to the supplied location*/

#define HIPE_OP_LOCATION_RETURN    18
/* Sent from server to client in immediate response to a 'get child' request.
 * Args are undefined (0), requestor is copied from the request, location is the requested location.*/

#define HIPE_OP_GEOMETRY_RETURN    19
/* arg[0] is x position and arg[1] is y position relative to containing frame.
 * arg[2] is the width and arg[3] is the height of the element itself. */

#define HIPE_OP_REQUEST_CONTAINER  20
/* this must be the first instruction received. arg[0] is the key and arg[1] is a short client name.*/

#define HIPE_OP_SERVER_DENIED      21
/*Sent by the server when a request is received but cannot be acted on due to
 *some critical violation. Usually this means that the client has not followed
 *protocol, e.g. is trying to send instructions even though a container request
 *was previously denied. Or, the session has been terminated at the server end.
 */

#define HIPE_OP_SET_ATTRIBUTE      22
/* arg[0] is the property and arg[1] is the new value */

#define HIPE_OP_SET_STYLE          23
/* arg[0] is the property and arg[1] is the new value */

#define HIPE_OP_ADD_STYLE_RULE     24 //to replace SET_STYLESHEET
/* Used to apply CSS style rules before other content is displayed.
 * arg[0] is the CSS descriptor of element(s) that the rule will apply to, and
 * arg[1] is the styling to apply, e.g. "border:0; width:100%;"
 */

#define HIPE_OP_SET_SRC            25
/*
 * arg[0] is the binary contents of the media file to be applied to the element.
 * arg[1] is the mime type of the source file to be applied to the element, e.g. "image/jpeg"
 */

#define HIPE_OP_SET_TITLE          26
/* arg[0] overwrites the container's title with a new one.*/

#define HIPE_OP_GET_FRAME_KEY      28
/* Sent by client to request the hostkey for connecting to a particular iframe
 * location is the handle corresponding to the particular iframe.
 */

#define HIPE_OP_KEY_RETURN         29
/* Sent by the server in response to a GET_FRAME_KEY request.
 * arg[0] is the returned host key and location is the frame on which the request was made.
 */

#define HIPE_OP_FRAME_EVENT        30
/* Sent by the server to indicate an event affecting a direct sub-frame of this one,
 * such as a client connecting to the sub-frame, or the title having been changed by
 * the client.
 * Frame events are sent automatically once HIPE_OP_GET_FRAME_KEY has been requested for that frame.
 * The framing client doesn't specify which events should be received.
 * requestor is the requestor value that was originally passed last time GET_FRAME_KEY was called.
 * location is the iframe element.
 * arg[0] is the event type
 * arg[1] is event detail (if applicable).
 */

#define HIPE_OP_FRAME_CLOSE        31
/* Sent by a client that manages a subframe to indicate a request for the client occupying that
 * frame to terminate.
 * Location: the frame element
 * arg[0]: non-null string - convey a user's request to close that client (e.g. user clicks close button).
 *       null - tells hipe server to terminate client connection forcibly.
 */

#define HIPE_OP_TOGGLE_CLASS       32
/* Applies or removes a CSS class to/from an element.
 * arg[0]: the class name to apply/remove.
 */

#define HIPE_OP_SET_FOCUS          33
/* Keyboard-focuses the element at location */

#define HIPE_OP_SET_BACKGROUND_SRC 34
/* Applies a background image to the element at location.
 * arg[0] is the binary contents of the media file.
 * arg[1] is the mime type of the binary data.
 */

#define HIPE_OP_TAKE_SNAPSHOT      35
/* Takes a screenshot of the client frame.
 * arg[0] is the file format. ("pdf" for vector screenshots, or "png" for raster screenshots.)
 */

#define HIPE_OP_FILE_RETURN        36
/* Sent to the client when the client requests a file (e.g. a snapshot of the frame contents).
 * requestor carries the value of the instruction that requested the file.
 */

#define HIPE_OP_ADD_STYLE_RULE_SRC 37
/* Sets background image data for a particular CSS style rule.
 * arg[0] is the CSS designator and arg[1] is the image file data, which should be PNG format.
 */

#define HIPE_OP_USE_CANVAS         38
/* Sets a canvas object at location to be the active canvas for drawing.
 * arg[0] is the canvas drawing context to be used (e.g. "2d").
 */

#define HIPE_OP_CANVAS_ACTION      39
/* Carries out a drawing method on the canvas object selected with HIPE_OP_USE_CANVAS.
 * arg[0] is the method to use (e.g. "fillRect") and arg[1] is a string of comma-separated
 * parameters (e.g "0,0,150,75").
 */

#define HIPE_OP_CANVAS_SET_PROPERTY 40
/* Sets a property on the current canvas context.
 * arg[0] is the property, arg[1] is the desired value.
 */

#define HIPE_OP_SET_ICON           41
/* Sets the client application's icon, which will be passed to the client's
 * parent environment for the purpose of helping the user identify this application.
 * arg[0] is the image file in PNG format. arg[1] is unused.
 */

#define HIPE_OP_REMOVE_ATTRIBUTE   42
/* Removes (unsets) the attribute specified in arg[0]. arg[1] is unused.
 */

#define HIPE_OP_MESSAGE            43
/* Sends an arbitrary message to another Hipe client with a direct parent/child relationship.
 * Or receives a message from another client.
 * Location: * 0 (or body element) means the message is being passed to/from the direct parent
 *               (which manages the client frame)
 *           * a frame element means message is being passed to/from that child frame's client.
 * arg[0], arg[1], requestor: passed through to other client unmodified. User-defined message data can be passed through here.
 */

#define HIPE_OP_GET_SCROLL_GEOMETRY 44
/*Requests the scroll geometry of the specified element. No arguments. Return
 *instruction is a HIPE_OP_GEOMETRY_RETURN instruction, where arg[0] is the x
 *scroll position at the left hand side, arg[1] is the y scroll position at the
 *visible top of the element, arg[2] is the total scrollable width of the
 *element content, and arg[3] is the total scrollable height.
 */

#define HIPE_OP_SCROLL_TO          45
#define HIPE_OP_SCROLL_BY          46
/*Scrolls the contents of the element TO an absolute position or BY a relative
 *(_BY) amount. arg[0] is the horizontal pixel offset to scroll to (or by).
 *arg[1] is the vertical pixel offset to scroll to (or by) To scroll in one
 *direction only, leave the unwanted argument unspecified.
 *arg[2] can be specified as "%" in which case a percentage of the scroll track
 *is used. Otherwise scrolling is in pixels.
 */

#define HIPE_OP_GET_CONTENT       47
/*Requests inner content of location; either flattened to unformatted plain
 *text or formatted to HTML (if arg[0] == "1" or "2"). Response is sent back from the
 *server as a HIPE_OP_CONTENT_RETURN instruction.
 * arg[0] == 0 -- unformatted plain text (default)
 * arg[0] == 1 -- contents formatted to HTML
 * arg[0] == 2 -- contents returned as formatted plain text (line breaks etc.)
 */

#define HIPE_OP_CONTENT_RETURN    48
/*Sent to client in response to a HIPE_OP_GET_CONTENT request. arg[0] contains
 *the requested content. requestor contains whatever was passed to the initial
 *request. Await this instruction after making a HIPE_OP_GET_CONTENT request.
 */

#define HIPE_OP_DELETE 49
/*Request that the location itself be removed from the document.
 */

#define HIPE_OP_CARAT_POSITION 50
/*Sets the position of the text entry carat in an input element such as a
 *<textarea> tag.
 * arg[0] == character index of cursor within element
 * arg[1] == character index of the end of the selection, or same as arg[0]
 * if text is not selected.
 * Note: additional arguments may be added in future if deemed necessary.
 */

#define HIPE_OP_GET_CARAT_POSITION 51
/*Requests the sever to send the client a HIPE_OP_CARAT_POSITION instruction
 *containing the *current* state of the input element.
 */

#define HIPE_OP_FIND_TEXT 52
/*Searches for the next instance of a text string in the entire client-frame
 *hierarchy, in the style of a web browser's 'find on page' feature.
 *Matches are selected for the user.
 *For security reasons, only functional for the top-level frame.
 */

#define HIPE_OP_ADD_FONT 53
/*Adds a @fontface style rule to the document
 *arg[0] == the name to be used to specify the font face in subsequent styling
 *arg[1] == the mime type for the font data (e.g. "application/x-font-woff")
 *arg[2] == the raw font file data itself.
 */

#define HIPE_OP_AUDIOVIDEO_STATE 54
/*If sent from the client to the server, modifies the state of an <audio> or
 *<video> tag. If received, this is in response to a ..._GET_AUDIOVIDEO_STATE
 *request and reflects the current status of the media element
 *arg[0] == the current playback position (in seconds) and the total duration (in seconds)
 *          separated by a ','. When setting the current playback position, it is
            sufficient to specify the desired position without the total duration.
 *arg[1] == the playback speed (1 == normal speed)
 *arg[2] == the playing/paused status. ("1" or "0")
 *arg[3] == the playback volume (fraction of 1.0)
 *--nonrequired arguments can be left empty.
 */

#define HIPE_OP_GET_AUDIOVIDEO_STATE 55
/*Requests a HIPE_OP_AUDIOVIDEO_STATE instruction containing the current state
 *of an <audio> or <video> tag.
 */

/*--------------*/

#define HIPE_FRAME_EVENT_CLIENT_CONNECTED    1 //arg[1] is client name
#define HIPE_FRAME_EVENT_CLIENT_DISCONNECTED 2
#define HIPE_FRAME_EVENT_TITLE_CHANGED       3 //arg[1] is new title
#define HIPE_FRAME_EVENT_ICON_CHANGED        4 //arg[1] is binary PNG image data.
#define HIPE_FRAME_EVENT_COLOR_CHANGED       5 //arg[1] is the new foreground color
#define HIPE_FRAME_EVENT_BACKGROUND_CHANGED  6 //arg[1] is the new background color

#define HIPE_NARGS 4 //fixed maximum number of arguments per hipe instruction.

typedef uint64_t hipe_loc;

struct _hipe_instruction { /*for storing the (decoded) values of an instruction*/
    char opcode;
    uint64_t requestor;
    hipe_loc location;
    char* arg[HIPE_NARGS]; //up to 4 arguments may be transmitted per instruction.
    uint64_t arg_length[HIPE_NARGS]; //each argument's length is transmitted as a 64 bit value.

    struct _hipe_instruction* next; /*for implementing a linked list.*/
};
typedef struct _hipe_instruction hipe_instruction;

void hipe_instruction_init(hipe_instruction* obj);
/*Must be called on any new hipe_instruction instance (excepting shallow copies
 *of an existing instance). The same applies if a struct previously modified by
 *the user (e.g. assigning arg strings to it) is going to be reused for
 *collecting new input from hipe. This function doesn't allocate or free memory,
 *but it initialises the struct's values into a consistent state so that other
 *hipe functions can do so safely.
 */

void hipe_instruction_alloc(hipe_instruction* obj);
/*Allocates memory to store each argument in obj, according to arg_length[]
 *values already assigned to obj. Arguments allocated in this way should later
 *be freed with hipe_instruction_clear().
 */

void hipe_instruction_clear(hipe_instruction* obj);
/*Frees memory previously allocated within the instruction, and leaves the
 *struct in a clean state ready for reuse. Do not use this function to clear a
 *hipe_instruction for which you have allocated values yourself. Instead,
 *de-allocate the argument strings in a way consistent with how you allocated
 *them, then call hipe_instruction_init() to re-initialise the values into a
 *clean state.
 */

void hipe_instruction_copy(hipe_instruction* dest, hipe_instruction* src);
/*Deeply copies src values into dest, allocating memory for args.
 *Does not do any initialisation or deallocation of any previous values that may
 *have been present in dest previously.
 *When done with the instruction in dest, free the memory of its internal
 *variables with hipe_instruction_clear().
 */

void hipe_instruction_move(hipe_instruction* dest, hipe_instruction* src);
/*Moves allocations from src to dest. Similar to hipe_instruction_copy except no
 *new memory is allocated; the memory allocated to src is reassigned to dest,
 *and src is then re-initialised. The memory in dest should later be freed in a
 *manner consistent with how src was previously allocated. */

#endif
#ifdef __cplusplus
}
#endif
