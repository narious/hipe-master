#! /bin/bash
# Start stress testing the hipe server by running
# a bunch of client applications. Terminate these
# applications with script 'stress-test-end.sh'.
#
# Takes a single argument number of clients to run as
# background processes.
for ((i=0; i < $1; ++i)); do
	../../test/hipe-calc &
	sleep 0.1
done
