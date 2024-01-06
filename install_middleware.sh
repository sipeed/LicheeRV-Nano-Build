#!/bin/sh

cp -v middleware/v2/sample/*/sample_* ramdisk/rootfs/overlay/cv1812cp_licheerv_nano_sd/bin/
rm -v ramdisk/rootfs/overlay/cv1812cp_licheerv_nano_sd/bin/*.c
rm -v ramdisk/rootfs/overlay/cv1812cp_licheerv_nano_sd/bin/*.o
rm -v ramdisk/rootfs/overlay/cv1812cp_licheerv_nano_sd/bin/*.d
rm -v ramdisk/rootfs/overlay/cv1812cp_licheerv_nano_sd/bin/*.h
