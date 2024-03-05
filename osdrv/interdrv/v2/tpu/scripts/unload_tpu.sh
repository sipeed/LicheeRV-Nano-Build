#!/bin/sh
module="cvi_tpu"
device="cvi-tpu0"

# invoke rmmod with all	arguments we got
/sbin/rmmod $module $* || exit 1

# Remove stale nodes
rm -f /dev/${device}

