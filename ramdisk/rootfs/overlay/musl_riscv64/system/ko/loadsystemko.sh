#!/bin/sh
${CVI_SHOPTS}
#
# Start to insert kernel modules
#
insmod /mnt/system/ko/soph_sys.ko
insmod /mnt/system/ko/soph_base.ko
insmod /mnt/system/ko/soph_rtos_cmdqu.ko
insmod /mnt/system/ko/soph_fast_image.ko
insmod /mnt/system/ko/soph_mipi_rx.ko
insmod /mnt/system/ko/soph_snsr_i2c.ko
insmod /mnt/system/ko/soph_vi.ko
insmod /mnt/system/ko/soph_vpss.ko
insmod /mnt/system/ko/soph_dwa.ko
insmod /mnt/system/ko/soph_vo.ko
insmod /mnt/system/ko/soph_mipi_tx.ko
insmod /mnt/system/ko/soph_rgn.ko

#insmod /mnt/system/ko/soph_wdt.ko
insmod /mnt/system/ko/soph_clock_cooling.ko

insmod /mnt/system/ko/soph_tpu.ko
insmod /mnt/system/ko/soph_vcodec.ko
insmod /mnt/system/ko/soph_jpeg.ko
insmod /mnt/system/ko/soph_vc_driver.ko MaxVencChnNum=9 MaxVdecChnNum=9
#insmod /mnt/system/ko/soph_rtc.ko
insmod /mnt/system/ko/soph_ive.ko

echo 3 > /proc/sys/vm/drop_caches
dmesg -n 4

#usb hub control
#/etc/uhubon.sh host

exit $?
