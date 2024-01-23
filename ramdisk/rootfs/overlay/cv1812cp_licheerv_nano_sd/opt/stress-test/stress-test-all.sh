#!/bin/sh

# ntp will broken test
killall crond

/opt/wifi.sh

nice -n 2 /opt/stress-test/net-stress-test.sh &
nice -n 3 /opt/stress-test/audio-stress-test.sh &
nice -n 4 /opt/stress-test/tpu-tress-test.sh &

mkdir -pv /tmp/ramdisk/
mount -t tmpfs tmpfs /tmp/ramdisk
touch /tmp/ramdisk/vendor_test.log
/opt/fb_load.sh
(/opt/camera2lcd.sh &> /dev/null) &
sleep 5
/opt/touch.sh
nice -n 1 /opt/vendortest
