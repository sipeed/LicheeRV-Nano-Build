#!/bin/sh

if [ -z $1 ]
then
	echo "usage: $0 count"
	exit 1
fi

seq 1 $1 | while read line
do
	cvi_tpu_demo2 1 2023 2023
done
