HIPE: Hypertext Pipe
====================

* For full documentation visit the project homepage at http://hipe.tech


VERSION INFORMATION
-------------------

v0.25 beta -- 08 Mar 2017

- EVENT_CANCEL instruction now sends an optional acknowledgement back to the client.


TODO:

- Fix bug: A lot of HIPE_OPCODE_GET_GEOMETRY requests in quick succession (alternated with other instructions)
  can cause the client to hang on the call to hipe_await_instruction() for a reply that doesn't eventuate.
  This scenario also causes the CPU to go to 100%, suggesting the blocking syscalls aren't running optimally.
- Add security: sanitise canvas operations to prevent arbitrary code causing performance instability.



GETTING THE LATEST VERSION
--------------------------

The latest version is always available via git. Major revisions are available on sourceforge, but these may not reflect the latest bug fixes.

Run the following command and a directory named hipe containing the Hipe distribution will be created in your present working directory:


  git clone https://gitlab.com/danielkos/hipe.git


