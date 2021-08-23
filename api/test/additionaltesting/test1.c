/* Calculator program to demonstrate an application displaying a GUI via the Hipe display server. */
/* Note: This calculator uses binary arithmetic, rather than decimal arithmetic. Therefore
   certain types of rounding errors inherant in floating point binary arithmetic are to be
   expected. */

#include <hipe.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

hipe_session session;