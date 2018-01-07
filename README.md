HIPE: Hypertext Pipe
====================

* For full documentation visit the project homepage at http://hipe.tech


VERSION INFORMATION
-------------------

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

- Reduce reliance on Qt to enable future porting to other toolkits. Use POSIX socket functions rather than
  those provided by Qt.
- Fix bug: A lot of HIPE_OP_GET_GEOMETRY requests in quick succession (alternated with other instructions)
  can cause the client to hang on the call to hipe_await_instruction() for a reply that doesn't eventuate.
  This scenario also causes the CPU to go to 100%, suggesting the blocking syscalls aren't running optimally.
- Add security: sanitise canvas operations to prevent arbitrary code causing performance instability.



GETTING THE LATEST VERSION
--------------------------

The latest version is always available via git. Major revisions are available on sourceforge, but these may not reflect the latest bug fixes.

Run the following command and a directory named hipe containing the Hipe distribution will be created in your present working directory:


  git clone https://gitlab.com/danielkos/hipe.git


