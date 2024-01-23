#!/bin/sh

export LD_LIBRARY_PATH=/mnt/system/lib/

if [ -z "$1" ]
then
	echo "usage: $0 count"
	exit 1
fi

echo "Set ADC Volume"
echo "0
24
1
" | sample_audio 6

echo "Now record sound"
arecord -Dhw:0,0 -d $1 -r 48000 -f S16_LE -t wav /tmp/ramdisk/test.wav

echo "Now play sound"
aplay -D hw:1,0 -f S16_LE /tmp/ramdisk/test.wav
