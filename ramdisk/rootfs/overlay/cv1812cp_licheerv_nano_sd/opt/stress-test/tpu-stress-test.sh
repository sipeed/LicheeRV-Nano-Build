#!/bin/sh

while true
do
	# ntp will borken test
	killall crond
	killall nn_runner
	cd /opt/nn_runner_test/
	nn_runner_test_time=$(/opt/nn_runner_test/run.sh 1 | grep 'forward image success' | awk -F':' '{print $2}' | awk -F'us' '{print $1}')
	echo "nn runner test time (us): $nn_runner_test_time" | tee -a /tmp/ramdisk/vendor_test.log
done
