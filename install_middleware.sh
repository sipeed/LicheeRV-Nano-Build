#!/bin/sh

cp -v middleware/v2/sample/*/sample_* ramdisk/rootfs/overlay/cv1812cp_licheerv_nano_sd/bin/
cp -rvf middleware/v2/sample/ive/data ramdisk/rootfs/overlay/cv1812cp_licheerv_nano_sd/opt/ive_data
cp -rvf middleware/v2/sample/scene_auto/param ramdisk/rootfs/overlay/cv1812cp_licheerv_nano_sd/opt/scene_auto_param
rm -v ramdisk/rootfs/overlay/cv1812cp_licheerv_nano_sd/bin/*.c
rm -v ramdisk/rootfs/overlay/cv1812cp_licheerv_nano_sd/bin/*.o
rm -v ramdisk/rootfs/overlay/cv1812cp_licheerv_nano_sd/bin/*.d
rm -v ramdisk/rootfs/overlay/cv1812cp_licheerv_nano_sd/bin/*.h
