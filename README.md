HIPE: Hypertext Pipe
====================

* For full documentation visit the project homepage at http://hipe.tech


VERSION INFORMATION
-------------------

v0.19 beta -- 13 Oct 2016


- The hipetec API interface (for C++ users) has been restructured to be more useable. The limitation of only allowing a single session has been removed from hipetec.
- Fixed bug: canvases didn't work unless they were created with ids.

TODO:

- Add security: sanitise canvas operations to prevent arbitrary code causing performance instability.



GETTING THE LATEST VERSION
--------------------------

The latest version is always available via git. Major revisions are available on sourceforge, but these may not reflect the latest bug fixes.

Run the following command and a directory named hipe containing the Hipe distribution will be created in your present working directory:


  git clone https://gitlab.com/danielkos/hipe.git


