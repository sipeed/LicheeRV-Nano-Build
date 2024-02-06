#!/bin/sh

if [ -z $1 ]
then
	echo "usage: $0 count"
	exit 1
fi

cd $(dirname $(realpath $0))

seq 1 $1 | while read line
do
	./nn_runner ./mobilenet_v2_rgb_224_int8.mud ./cat_224.jpg 1000
done

