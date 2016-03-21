v0.16 beta -- 22 Mar 2016

- First beta release!
- Hipe is now relatively feature-complete, but needs additional work to make it more robust and efficient.

TODO:

- Fix bug: can't call HIPE_OPCODE_SET_STYLE on body element until other elements have been added.
- Fix bug: canvases don't work unless they are created with ids.
- Add security: sanitise canvas operations to prevent arbitrary code causing performance instability.
