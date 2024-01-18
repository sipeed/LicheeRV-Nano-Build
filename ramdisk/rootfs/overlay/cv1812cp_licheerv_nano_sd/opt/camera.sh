#!/bin/sh

THISDIR=$(dirname $(realpath $0))

if [ ! -e /mnt/data/sensor_cfg.ini ]
then
    cp -fv $THISDIR/sensor_cfg.ini /mnt/data
fi
echo "snsr_r 0 0" > /proc/mipi-rx
devmem 0x03002814 32 0x00308101
echo "snsr_on 0 1 4" > /proc/mipi-rx

SENSOR_TEST=/opt/sensor_test_gc4653

cd /tmp/

if [ -z "$1" ]
then
    echo "usage: $0 count"
    exit 1
fi

seq 1 $1 | while read line
do
    # test camera
    echo "
    1
    0
    1
    255" | $SENSOR_TEST
    rm -rf /tmp/*.raw
    rm -rf /tmp/*.yuv
done
