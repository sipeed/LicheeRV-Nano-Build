#!/bin/sh

if [ -z "$1" ]
then
	echo "usage: $0 count"
	exit 1
fi

seq 1 $1 | while read line
do
	echo "Now record sound"
	arecord -Dhw:0,0 -d 5 -r 48000 -f S16_LE -t wav /tmp/ramdisk/test.wav

	echo "Now play sound"
	aplay -D hw:1,0 -f S16_LE /tmp/ramdisk/test.wav
done
