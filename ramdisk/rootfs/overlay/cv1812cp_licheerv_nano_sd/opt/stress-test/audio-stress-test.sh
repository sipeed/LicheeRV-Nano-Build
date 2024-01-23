#!/bin/sh

while true
do
	# ntp will broken test
	killall crond
	/opt/audio.sh 1
	sleep 1
	if [ $? -ne 0 ]
	then
		echo "audio test failed"
	else
		echo "audio test running"
	fi
done
