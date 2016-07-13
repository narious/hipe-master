Source code for Hipe server process.

DEPENDENCIES:

You will need to have installed the development files for Qt and Qt-webkit.
Building Hipe has been tested with Qt versions 4 and 5.
Qt must be built with STL support included.

BUILD INSTRUCTIONS:

You can either build the Qt project using Qt Creator, or alternatively from the terminal as follows:

Run the following commands from this directory.

$ qmake -makefile ./src/hiped.pro
$ make

Then clean up the object files with:

$ make clean


Now the hiped executable is ready to run.



RUNNING THE SERVER:

The server must be run in an X environment. You can either run ./hiped directly in your
favourite desktop environment (rootless mode), or you can use the script ./start-hipe-standalone
*from the current directory* while in a text-only console virtual terminal (this script assumes that the second virtual
graphics terminal (often the one accessible with Ctrl+Alt+F8) is free for use)


When the hiped server process starts, there is no graphical interface shown on the screen,
since no Hipe applications are running within the display server yet.

The user can use a separate terminal instance to start a hipe application, which will then
connect to the server process and be displayed.

