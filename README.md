v0.18 beta -- 22 Sep 2016

- Added hipetec.hh -- a C++ wrapper to provide OO location objects to C++ developers.
- Added partial optimisation of user input sanitisation.
- display of new top-level containers is now postponed until initial styling
  has been specified and the first content has been appended to the body.
  This avoids 'restyling flicker' when the container is first displayed.

TODO:

- Restructure hipetec internally to avoid use of static private variables, so compiler doesn't see multiple initialisations.

- Fix bug: canvases don't work unless they are created with ids.
- Add security: sanitise canvas operations to prevent arbitrary code causing performance instability.
