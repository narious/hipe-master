HIPE: Hypertext Pipe
====================

* For full documentation visit the project homepage at http://hipe.tech
* See INSTALL.txt for further installation instructions which may supplement the information on the website.

VERSION INFORMATION
-------------------

1.10 beta -- 28 Feb 2020

Attempted to commit some fixes that were not in the tree.
Replaced the old HIPE_OP_SET_BACKGROUND_SRC operation with a more general HIPE_OP_SET_STYLE_SRC
operation (can be used with the "background-image" style attribute for the same functionality.
Since the old instruction has been removed, existing code that used this operation should be modified.

1.09 beta -- 9 Jul 2019

Numerous improvements. An instruction framework for modal dialog box implementation is now provided.

1.08 beta -- 14 Apr 2019

Numerous bug fixes. Added carat manipulation instructions for accessing and manipulating selections in text input elements.

1.07  beta -- 24 Jan 2019

Multiple new instructions, sanitisation of canvas instructions to prevent arbitrary
code execution.

v1.06 beta -- 21 July 2018

Important bug fix release.
Test programs now conveniently installable to help verify a working Hipe environment.

v1.05 beta -- 4 July 2018

New instructions and instruction arguments added.

v1.04 beta -- 15 May 2018

Documentation cleanup and various bug-fixes.

v1.03 beta -- 24 Apr 2018

Sending instructions to the server is now an atomic operation.

v1.02 beta -- 19 Apr 2018

Fixed a longstanding bug in await_instruction that caused the calling client
to hang at random.

v1.01 beta -- 7 Jan 2018

- HIPE_OP_GET_SCROLL_GEOMETRY instruction has been added.
- HIPE_OP_GET_GEOMETRY instruction has changed. Server now replies with a HIPE_OP_GEOMETRY_RETURN
  instruction, and the old two-step reply instructions have been removed.
- Socket handling in the hiped server process is now handled with multithreading. Instructions are
  collected and decoded in a separate thread.

From 1.0:
- The hipe_send() function now supports a variadic syntax. Existing programs will require modification.
- Hipe now uses a slightly different message-passing format between client and server, enabling between
  zero and four arguments per instruction. (Existing programs require recompilation.)
- From 0.28: The argument ordering for the following  instructions has been altered: HIPE_OP_SET_ICON,
  HIPE_OP_SET_BACKGROUND_SRC, HIPE_OP_SET_SRC. Applications that make use of these will need minor modification to
  work with the new version. This change will ease the transition to a variadic instruction set.
- C++ <hipe> library extension now has a new argument format for the hipe::send() function. Arguments should now
  be enclosed in initialiser list braces { }, and a variable number of arguments may be provided.

- From 0.27: A workaround has been added to allow HIPE_OP_ADD_STYLE_RULE instructions to be respected after initial
  body content has been added to the screen. Not all style rules added in this scenario appear to be respected
  by the webkit backend, however.


TODO:

- More robust memory management
- A transparent PING instruction to allow the hipe display server to identify nonresponsive applications based on when the application last checked for new instructions. Such information could then be passed to (or managed entirely by) the framing manager.
- More flexible DOM manipulation operations, e.g. compliment append with prepend operations.


GETTING THE LATEST VERSION
--------------------------

The latest version is always available via git. Major revisions are available on sourceforge, but these may not reflect the latest bug fixes.

Run the following command and a directory named hipe containing the Hipe distribution will be created in your present working directory:


  git clone https://gitlab.com/danielkos/hipe.git


