#!/bin/sh

/opt/wifi.sh
insmod /mnt/system/ko/cmac.ko
insmod /mnt/system/ko/ecc.ko
insmod /mnt/system/ko/ecb.ko
insmod /mnt/system/ko/ecdh_generic.ko
insmod /mnt/system/ko/hmac.ko
insmod /mnt/system/ko/libaes.ko
insmod /mnt/system/ko/aes_generic.ko
insmod /mnt/system/ko/bluetooth.ko
insmod /mnt/system/ko/rfcomm.ko
insmod /mnt/system/ko/btintel.ko
insmod /mnt/system/ko/hci_uart.ko
insmod /mnt/system/ko/bnep.ko
hciconfig | grep hci0
if [ $? -ne 0 ]
then
	hciattach ttyS1 any 1500000
	hciconfig hci0 up
fi
