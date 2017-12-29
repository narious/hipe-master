HIPE: Hypertext Pipe
====================

* For full documentation visit the project homepage at http://hipe.tech


VERSION INFORMATION
-------------------

v0.28 beta -- 29 Dec 2017

- From 0.28: The argument ordering for the following  instructions has been altered: HIPE_OP_SET_ICON,
  HIPE_OP_SET_BACKGROUND_SRC, HIPE_OP_SET_SRC. Applications that make use of these will need minor modification to
  work with the new version. This change will ease the transition to a variadic instruction set.
- C++ <hipe> library extension now has a new argument format for the hipe::send() function. Arguments should now
  be enclosed in initialiser list braces { }, and a variable number of arguments may be provided.

- From 0.27: A workaround has been added to allow HIPE_OP_ADD_STYLE_RULE instructions to be respected after initial
  body content has been added to the screen. Not all style rules added in this scenario appear to be respected
  by the webkit backend, however.


TODO:

- Hipe is moving from a fixed number of arguments per instruction to a variable number of arguments (from 0 to 4 arguments
  with argument length limited to the memory size magnitude (size_t) of the target machine).
  This means functions need to be revised to allow variadic arguments, and the struct format of hipe_instruction
  will change, as will the communication protocol. This will break some existing applications but it's now or never
  as far as Hipe is concerned.

- Fix bug: A lot of HIPE_OP_GET_GEOMETRY requests in quick succession (alternated with other instructions)
  can cause the client to hang on the call to hipe_await_instruction() for a reply that doesn't eventuate.
  This scenario also causes the CPU to go to 100%, suggesting the blocking syscalls aren't running optimally.
- Add security: sanitise canvas operations to prevent arbitrary code causing performance instability.



GETTING THE LATEST VERSION
--------------------------

The latest version is always available via git. Major revisions are available on sourceforge, but these may not reflect the latest bug fixes.

Run the following command and a directory named hipe containing the Hipe distribution will be created in your present working directory:


  git clone https://gitlab.com/danielkos/hipe.git


