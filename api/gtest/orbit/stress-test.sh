#! /bin/sh
# Stress test hipe by running a bunch of client
# applications.
NTESTS=200

./stress-test-start.sh $NTESTS
napps=$(pgrep hipe-calc | wc --lines)
printf "successfully ran %d/%d client apps\n" $napps $NTESTS
sleep 1
./stress-test-end.sh
