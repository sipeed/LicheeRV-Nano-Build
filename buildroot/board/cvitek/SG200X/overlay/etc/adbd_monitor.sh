#!/bin/bash

UDC_PATH=/sys/kernel/config/usb_gadget/g0/UDC
if [ -f /boot/usb.adbd ] && [ -f /boot/usb.dev ];
then
    while true; do
        if [ -z "$(cat $UDC_PATH | tr -d '[:space:]')" ];
        then
            echo "try restart usb device"
            /etc/init.d/S03usbdev stop
            /etc/init.d/S03usbdev start
        fi
        sleep 5
    done
fi
