#!/bin/sh
module="vcodec"
device="vcodec"

# invoke rmmod with all	arguments we got
rmmod /lib/modules/$module $* || exit 1

# Remove stale nodes
rm -f /dev/${device}
