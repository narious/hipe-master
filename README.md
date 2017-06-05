HIPE: Hypertext Pipe
====================

* For full documentation visit the project homepage at http://hipe.tech


VERSION INFORMATION
-------------------

v0.26 beta -- 05 Jun 2017

- HIPE_OPCODE_ prefixed operation codes have been replaced with shorter HIPE_OP_ macro constants. Behaviour
  is identical to how it was before; the macro definitions have simply been shortened to make them easier to
  type when writing source code.


TODO:

- Fix bug: A lot of HIPE_OP_GET_GEOMETRY requests in quick succession (alternated with other instructions)
  can cause the client to hang on the call to hipe_await_instruction() for a reply that doesn't eventuate.
  This scenario also causes the CPU to go to 100%, suggesting the blocking syscalls aren't running optimally.
- Add security: sanitise canvas operations to prevent arbitrary code causing performance instability.



GETTING THE LATEST VERSION
--------------------------

The latest version is always available via git. Major revisions are available on sourceforge, but these may not reflect the latest bug fixes.

Run the following command and a directory named hipe containing the Hipe distribution will be created in your present working directory:


  git clone https://gitlab.com/danielkos/hipe.git


