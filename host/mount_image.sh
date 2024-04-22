#!/bin/sh

THISDIR=$(dirname $(realpath $0))

if [ -z $2 ]
then
	echo "usage: $0 image dir"
fi

if [ ! -e $2 ]
then
	mkdir -pv $2
fi

set -eux

PART=2

PART_OFFSET=$(partx -s $1 | head -n 3 | tail -n 1 | awk '{print $2}')
PART_OFFSET=$((PART_OFFSET * 512)) # sector size is 512

echo "PART: $PART"
echo "PART OFFSET: $PART_OFFSET"

# some old version fuse2fs not support offset
$THISDIR/fuse2fs -o fakeroot -o offset=$PART_OFFSET $1 $2
