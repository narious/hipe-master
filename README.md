HIPE: Hypertext Pipe
====================

* For full documentation visit the project homepage at http://hipe.tech


VERSION INFORMATION
-------------------

v0.22 beta -- 25 Oct 2016


- Partial rewrite and code cleanup of API functions
- Major bug fix in API. Instruction queue processing has been fixed. Existing hipe applications should be recompiled to the new API.
- Display server now has the ability to communicate changes to a client process's colour scheme to the framing manager. This allows
  framing managers to complement the colours of client applications.


TODO:

- Add security: sanitise canvas operations to prevent arbitrary code causing performance instability.



GETTING THE LATEST VERSION
--------------------------

The latest version is always available via git. Major revisions are available on sourceforge, but these may not reflect the latest bug fixes.

Run the following command and a directory named hipe containing the Hipe distribution will be created in your present working directory:


  git clone https://gitlab.com/danielkos/hipe.git


