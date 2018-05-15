Source code for Hipe server process.

DEPENDENCIES:

You will need to have installed the development files for Qt and Qt-webkit.
Building Hipe has been tested with Qt versions 4 and 5 on Intel / AMD64 systems,
and with Qt version 4 on Raspberry Pi.
Qt must be built with STL support included.

BUILD INSTRUCTIONS:

****
****PLEASE REFER TO THE DOCUMENTATION SECTION AT http://hipe.tech
****FOR MORE UP-TO-DATE INSTALLATION INSTRUCTIONS.
****(The website also explains how to set up environment variables for non-default settings.)


You can either build the Qt project using Qt Creator, or alternatively from the terminal as follows:

Run the following commands from this directory.

$ qmake -makefile ./src/hiped.pro
$ make

Then clean up the object files with:

$ make clean

Now the hiped executable is ready to run.

INSTALLING

Once the build is complete, simply copy the hiped executable file into /usr/local/bin/, or an alternative directory 
of your choice.


RUNNING THE SERVER:

The default build requires that hiped be run within an X environment.

You can either run ./hiped directly from within your favourite desktop environment 
(rootless mode), or you can deploy the Hipe display server on its own, or within a bare-bones X session.

If running on top of an existing desktop environment (e.g. Ubuntu) it is recommended to
set up hiped to run in the background when the user logs into a graphical desktop session,
by adding the following command to your configuration:

hiped &

The command should be executed within the environment of the user's desktop session, so
that hiped can connect to the display server.



For a dedicated Hipe interface (e.g. embedded devices)...

One example for a dedicated Hipe desktop on top of X is given in a rudimentary script 
provided: run ./start-hipe-standalone 
*from the current directory* while in a text-only console virtual terminal (this script 
assumes that the second virtual graphics terminal (often the one accessible with Ctrl+Alt+F8) 
is free for use).


When the hiped server process starts, there is no graphical interface shown on the screen,
since no Hipe applications are running within the display server yet.

The user can use a separate terminal instance to start a hipe application, which will then
connect to the server process and be displayed.

Run `hiped --help` for more command-line options.
