#!/bin/sh
module="jpu"
device="jpu"

# invoke rmmod with all	arguments we got
rmmod /lib/modules/$module $* || exit 1

# Remove stale nodes
rm -f /dev/${device}
