#!/bin/sh

# 10inch
#sample_dsi --panel=ZCT1429V1
# 7inch
sample_dsi --panel=ZCT2133V1
# 5inch
#sample_dsi --panel=ST7701_DXQ5D0019B480854
# 2inch
#sample_dsi --panel=ST7701_HD228001C31

# backlight on
devmem 0x030010EC 32 0x3
echo 448 > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio448/direction
echo 1 > /sys/class/gpio/gpio448/value

insmod /mnt/system/ko/cfbcopyarea.ko
insmod /mnt/system/ko/cfbfillrect.ko
insmod /mnt/system/ko/cfbimgblt.ko

# 32bit RGB888
insmod /opt/ko/huashanpai/cvi_fb.ko option=1
# 16bit RGB1555
#insmod /opt/ko/v4.1.0_181x_fb_ko_debug/cvi_fb.ko
