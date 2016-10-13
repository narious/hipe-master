/* Copyright (c) 2015 Daniel Kos, General Development Systems

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

#define HIPE_OPCODE_CLEAR              1
/*clear the contents of the tag given by location (removing all child elements), or clear body if location==0.*/

#define HIPE_OPCODE_SET_TEXT           2
/* set a tag's contents to a string of plain text given in arg1, overwriting previous contents.
 * if location==0 this applies to the entire body tag.
 */

#define HIPE_OPCODE_APPEND_TEXT        3
/* append plain text inside the tag given by location, or inside body if location==0.
 * arg1 is the text content to append. HTML-style special character entities may be used.
 */

#define HIPE_OPCODE_APPEND_TAG         4
/* append a tag element inside the tag given by location, or inside body if location==0.
 * arg1 is the tag type, arg2 is an optional identifier for the tag */

#define HIPE_OPCODE_ATTRIBUTE_RETURN   5
/* arg1 is the name of the attribute and arg2 is the retrieved value */

#define HIPE_OPCODE_CONTAINER_GRANT    6
/* Server response to container request. Arg1 is "0" if the container request was denied, or "1" if
 * a new container has been granted. */

#define HIPE_OPCODE_EVENT              7
/* Sent by the server to the client whenever a requested event occurs.
 * 'arg1' is the type of event, and 'location' is the tag for which events of 'arg1' were requested.
 * 'requestor' returns the same client-supplied value as was used for HIPE_OPCODE_EVENT_REQUEST.
 * 'arg2' provides event-specific detail about the event -- e.g. for "click" events it returns the
 * number of clicks registered.
 * */

#define HIPE_OPCODE_EVENT_CANCEL       8
/* Sent by the client to cancel notification of further events of type 'arg1' on location 'location'.*/

#define HIPE_OPCODE_EVENT_REQUEST      9
/*Sent by the client to request notification of event 'arg1' on location 'location'.
 * */

#define HIPE_OPCODE_FREE_LOCATION      10
/* Sent by the client to the server to de-allocate a location index that is no longer required.*/

#define HIPE_OPCODE_GET_ATTRIBUTE      11
/* arg1 is the name of the attribute*/

#define HIPE_OPCODE_GET_BY_ID          12
/* Request a location index for the HTML tag with id attribute == arg1*/

#define HIPE_OPCODE_GET_FIRST_CHILD    13

#define HIPE_OPCODE_GET_GEOMETRY       14
/* arg1 == 0 requests x,y position, arg1 == 1 requests size. */

#define HIPE_OPCODE_GET_LAST_CHILD     15
#define HIPE_OPCODE_GET_NEXT_SIBLING   16
#define HIPE_OPCODE_GET_PREV_SIBLING   17
/* Sent by client to request a location index relative to the supplied location*/

#define HIPE_OPCODE_LOCATION_RETURN    18
/* Sent from server to client in immediate response to a 'get child' request.
 * Args are undefined (0), requestor is copied from the request, location is the requested location.*/

#define HIPE_OPCODE_POSITION_RETURN    19
/* arg1 is x position and arg2 is y position relative to containing frame. */

#define HIPE_OPCODE_REQUEST_CONTAINER  20
/* this must be the first instruction received. arg1 is the key and arg2 is a short client name.*/

#define HIPE_OPCODE_SERVER_DENIED      21
/* Sent by the server when a request is received but cannot be acted on due to some critical violation.
 * Usually this means that the client has not followed protocol, e.g. is trying to send instructions
 * even though a container request was previously denied. Or, the session has been terminated at the
 * server end. */

#define HIPE_OPCODE_SET_ATTRIBUTE      22
/* arg1 is the property and arg2 is the new value */

#define HIPE_OPCODE_SET_STYLE          23
/* arg1 is the property and arg2 is the new value */

#define HIPE_OPCODE_ADD_STYLE_RULE     24 //to replace SET_STYLESHEET
/* Used to apply CSS style rules before other content is displayed.
 * arg1 is the CSS descriptor of element(s) that the rule will apply to, and
 * arg2 is the styling to apply, e.g. "border:0; width:100%;"
 */

#define HIPE_OPCODE_SET_SRC            25
/* arg1 is the mime type of the source file to be applied to the element, e.g. "image/jpeg"
 * arg2 is the binary contents of the media file itself.
 */

#define HIPE_OPCODE_SET_TITLE          26
/* arg1 overwrites the container's title with a new one.*/

#define HIPE_OPCODE_SIZE_RETURN        27
/* arg1 is width and arg2 is height. */

#define HIPE_OPCODE_GET_FRAME_KEY      28
/* Sent by client to request the hostkey for connecting to a particular iframe
 * location is the handle corresponding to the particular iframe.
 */

#define HIPE_OPCODE_KEY_RETURN         29
/* Sent by the server in response to a GET_FRAME_KEY request.
 * arg1 is the returned host key and location is the frame on which the request was made.
 */

#define HIPE_OPCODE_FRAME_EVENT        30
/* Sent by the server to indicate an event affecting a direct sub-frame of this one,
 * such as a client connecting to the sub-frame, or the title having been changed by
 * the client.
 * Frame events are sent automatically once HIPE_OPCODE_GET_FRAME_KEY has been requested for that frame.
 * The framing client doesn't specify which events should be received.
 * requestor is the requestor value that was originally passed last time GET_FRAME_KEY was called.
 * location is the iframe element.
 * arg1 is the event type
 * arg2 is event detail (if applicable).
 */

#define HIPE_OPCODE_FRAME_CLOSE        31
/* Sent by a client that manages a subframe to indicate a request for the client occupying that
 * frame to terminate.
 * Location: the frame element
 * arg1: null string - convey a user's request to close that client (e.g. user clicks close button).
 *       non-null - tells hipe server to terminate client connection forcibly.
 */

#define HIPE_OPCODE_TOGGLE_CLASS       32
/* Applies or removes a CSS class to/from an element.
 * arg1: the class name to apply/remove.
 */

#define HIPE_OPCODE_SET_FOCUS          33
/* Keyboard-focuses the element at location */

#define HIPE_OPCODE_SET_BACKGROUND_SRC 34
/* Applies a background image to the element at location.
 * arg1 is the mime type of the background image,
 * arg2 is the binary contents of the media file itself.
 */

#define HIPE_OPCODE_TAKE_SNAPSHOT      35
/* Takes a screenshot of the client frame.
 * arg1 is the file format. ("pdf" for vector screenshots, or "png" for raster screenshots.)
 */

#define HIPE_OPCODE_FILE_RETURN        36
/* Sent to the client when the client requests a file (e.g. a snapshot of the frame contents).
 * requestor carries the value of the instruction that requested the file.
 */

#define HIPE_OPCODE_ADD_STYLE_RULE_SRC 37
/* Sets background image data for a particular CSS style rule.
 * arg1 is the CSS designator and arg2 is the image file data, which should be PNG format.
 */

#define HIPE_OPCODE_USE_CANVAS         38
/* Sets a canvas object at location to be the active canvas for drawing.
 * arg1 is the canvas drawing context to be used (e.g. "2d").
 */

#define HIPE_OPCODE_CANVAS_ACTION      39
/* Carries out a drawing method on the canvas object selected with HIPE_OPCODE_USE_CANVAS.
 * arg1 is the method to use (e.g. "fillRect") and arg2 is a string of comma-separated
 * parameters (e.g "0,0,150,75").
 */

#define HIPE_OPCODE_CANVAS_SET_PROPERTY 40
/* Sets a property on the current canvas context.
 * arg1 is the property, arg2 is the desired value.
 */

#define HIPE_OPCODE_SET_ICON           41
/* Sets the client application's icon, which will be passed to the client's
 * parent environment for the purpose of helping the user identify this application.
 * arg2 is the image file in PNG format. arg1 is unused/reserved for future use.
 */

#define HIPE_OPCODE_REMOVE_ATTRIBUTE   42
/* Removes (unsets) the attribute specified in arg1. arg2 is unused.
 */

#define HIPE_OPCODE_MESSAGE            43
/* Sends an arbitrary message to another Hipe client with a direct parent/child relationship.
 * Or receives a message from another client.
 * Location: * 0 (or body element) means the message is being passed to/from the direct parent (which manages the client frame)
 *           * a frame element means message is being passed to/from that child frame's client.
 * arg1, arg2, requestor: passed through to other client unmodified. User-defined message data can be passed through here.
 */

/*--------------*/

#define HIPE_FRAME_EVENT_CLIENT_CONNECTED    1 //arg2 is client name
#define HIPE_FRAME_EVENT_CLIENT_DISCONNECTED 2
#define HIPE_FRAME_EVENT_TITLE_CHANGED       3 //arg2 is new title
#define HIPE_FRAME_EVENT_ICON_CHANGED        4 //arg2 is binary PNG image data.


typedef uint64_t hipe_loc;

struct _hipe_instruction { /*for storing the (decoded) values of an instruction*/
    char opcode;
    uint64_t requestor;
    hipe_loc location;
    uint32_t arg1Length, arg2Length;
    char* arg1;
    char* arg2;

    struct _hipe_instruction* next; /*for implementing a linked list.*/
};
typedef struct _hipe_instruction hipe_instruction;

void hipe_instruction_init(hipe_instruction* obj);
/*Must be called on any new hipe_instruction instance (excepting shallow copies of an existing instance).
 *The same applies if a struct previously modified by the user (e.g. assigning arg strings to it) is
 *going to be reused for collecting new input from hipe. This function doesn't allocate or free memory,
 *but it initialises the struct's values into a consistent state so that other hipe functions can do so
 *safely.
 */

void hipe_instruction_clear(hipe_instruction* obj);
/*Frees memory previously allocated within the instruction, and leaves the struct in a clean state ready
 *for reuse. Do not use this function to clear a hipe_instruction for which you have allocated values
 *yourself. Instead, de-allocate the argument strings in a way consistent with how you allocated them,
 *then call hipe_instruction_init() to re-initialise the values into a clean state.
 */

void hipe_instruction_copy(hipe_instruction* dest, hipe_instruction* src);
/*Deeply copies src values into dest, allocating memory for args.
 *Does not do any initialisation or deallocation of any previous values that may have been present
 *in dest previously.
 *When done with the instruction in dest, free the memory of its internal variables
 *with hipe_instruction_clear().
 */

#endif
#ifdef __cplusplus
}
#endif
