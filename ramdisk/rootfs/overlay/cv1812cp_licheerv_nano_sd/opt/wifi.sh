#!/bin/sh

insmod /mnt/system/ko/3rd/aic8800_bsp.ko
insmod /mnt/system/ko/3rd/aic8800_fdrv.ko
ifconfig wlan0 up
touch /etc/wpa_supplicant.conf
wpa_supplicant -c /etc/wpa_supplicant.conf -i wlan0 -B -g/tmp/ramdisk/wpa
udhcpc -i wlan0 -b &
