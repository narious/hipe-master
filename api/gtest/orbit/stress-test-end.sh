#! /bin/sh
# Terminate all hipe client applications started
# with script 'stress-test-start.sh'.
for pid in $(pgrep hipe-calc); do
	kill -s 9 $pid
done
