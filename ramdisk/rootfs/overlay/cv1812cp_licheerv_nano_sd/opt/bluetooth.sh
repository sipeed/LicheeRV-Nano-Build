#!/bin/sh

insmod /mnt/system/ko/aic8800_bsp.ko
insmod /mnt/system/ko/aic8800_fdrv.ko
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
hciattach ttyS1 any 1500000
hciconfig hci0 up
