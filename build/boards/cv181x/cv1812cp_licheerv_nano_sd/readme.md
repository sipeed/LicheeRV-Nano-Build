# how to use sdk

video example:

```
asciinema play build/boards/cv181x/cv1812cp_licheerv_nano_sd/asciinema/how-to-use-sdk
```

# manual create bootable sdcard

```
sudo cfdisk /dev/sdX
# then create boot partition:
# size is 32MiB
# type is W95 FAT32 (LBA)
# boot is Bootable
# sdX1 is sdcard's boot partition, use fat32 format
# then create root partition:
# size is 1024MiB (set by your rootfs.sd size)
# type is Linux
# boot is not Bootable
# sdX2 is sdcard's root partition
# then create overlay partition:
# size is max size (set by your sdcard size)
# type is Linux
# boot is not Bootable
# sdX3 is sdcard's overlay partition
sudo mkfs.vfat -n boot /dev/sdX1
mkdir -pv /mnt/boot/
sudo mount /dev/sdX1 /mnt/boot
# install bootloader
cp -vf install/soc_cv1812cp_licheerv_nano_sd/*.bin /mnt/boot/
# install kernel
cp -vf install/soc_cv1812cp_licheerv_nano_sd/rawimages/boot.sd /mnt/boot/
# install rootfs(squashfs)
sudo dd if=install/soc_cv1812cp_licheerv_nano_sd/rawimages/roots.sd of=/dev/sdX2
# formate overlay partition
mkfs.ext4 /dev/sdX3 # if you have formarted, you can skip this step
```

video sample:

```
asciinema play build/boards/cv181x/cv1812cp_licheerv_nano_sd/asciinema/how-to-connect-board
```

# connect to board by serial port

licheerv nano's usb is device mode when bootup, it have:

```
usb cdc acm
usb rndis net
```

connect your board to PC, use usb cable.

access your licheerv nano:

```
picocom -b 115200 /dev/ttyACMX # X is your device, use dmesg look it
```

if you need trans file on serial port, you can use command on licheerv nano:

```
ek -r
```

then you can start kermit send program on your PC

# connect to board by network

user: root

pass: cvitek

```
# connect device's usb port to your PC
# then PC will set new usb rndis interface IP address by ipv4 link local or dhcp
# board is enable mdns by default, you can use mdns found it:
# avahi-browse -art | grep licheervnano
# XXXX is machine id, if you have multi licheervnano in your network, you can use machine id to select it.
ssh root@licheervnano-XXXX.local
# note: some system's mdns is not working, my maybe need use avahi utils to get real ip address instead use domain name:
# avahi-resolve-host-name licheervnano-XXXX.local
```

# compile program use vendor's toolchain

```
wget https://sophon-file.sophon.cn/sophon-prod-s3/drive/23/03/07/16/host-tools.tar.gz
tar xf host-tools.tar.gz
cd host-tools/gcc/riscv64-linux-musl-x86_64/bin
export PATH=$(pwd):${PATH}

cd /path/to/your/project
riscv64-unknown-linux-musl-gcc -mcpu=c906fdv -march=rv64imafdcv0p7xthead -mcmodel=medany -mabi=lp64d hellworld.c -o helloworld
# then upload helloworld into board
scp helloworld root@licheervnano-XXXX.local:/tmp/helloworld
# then connect board , execute it
ssh root@licheervnano-XXXX.local -- 'chmod +x /tmp/helloworld; /tmp/helloworld'
```

video sample:

# wifi

if you want use wifi on licheervnano:

1. write wifi ssid & password into config file:

```
/etc/wpa_supplicant.conf
```

add:


```
network={
        ssid="ssid"
        psk="password"
}
```

2. execute command to load wifi drvier:

```
/opt/wifi.sh
```

3. if your wifi is connected please:

```
echo 'sh /opt/wifi.sh' >> /etc/rc.local
```

if not working, please check step 1, then execute step 2

# bluetooth

if you want use bluetooth on licheervnano:

1. execute command to load bluetooth drvier:

```
/opt/wifi.sh
```

2. scan

```
```

3. if your bluetooth is working please:

```
echo 'sh /opt/bluetooth.sh' >> /etc/rc.local
```

if not working, please check step 1, then execute step 2

# middleware

```
# after middleware source code edit, you need rebuild it & copy new program into rootfs
build_all
# example: copy mipi_tx
cp middleware/v2/sample/mipi_tx/sample_dsi ramdisk/rootfs/overlay/cv1812cp_licheerv_nano_sd/bin/sample_dsi
# example: copy all
sh install_middleware.sh
```
# screen

if you want use mipi screen on licheervnano:

1. edit init script:

```
/opt/fb_load.sh
```

2. uncomment your screen initial command

3. execute init script:

```
sh /opt/fb_load.sh
```

4. run your graphic program

5. if your screen is working please:  

```
echo 'sh /opt/fb_load.sh' >> /etc/rc.local
```

if not working, please check step 2, then execute step 3

# tpu

```
/opt/tpu-test.sh 10
/opt/nn_runner_test/run.sh 10
```

# camera

```
```

# your custom config file

```
cp xxx.conf ramdisk/rootfs/overlay/cv1812cp_licheerv_nano_sd/path/to/xxx.conf
```

# add your custom package

create initial file & directory

```
source build/cvisetup.sh
mkdir -pv ramdisk/rootfs/public/hello
touch ramdisk/rootfs/public/hello/Kconfig
touch ramdisk/rootfs/public/hello/target.mk
mkdir -pv ramdisk/rootfs/public/hello/musl_riscv64/bin
```

edit ramdisk/rootfs/public/hello/Kconfig

write:

```
config TARGET_PACKAGE_HELLO
	bool "Target package hello"
	default n
	help
	  target hello
```

edit ramdisk/rootfs/public/dropbear/target.mk

write:

```
ifeq ($(CONFIG_TARGET_PACKAGE_HELLO),y)
TARGET_PACKAGES += hello
endif
```


build your program:

```
# riscv64-unknown-linux-musl-gcc must be vendor's toolchain
riscv64-unknown-linux-musl-gcc -mcpu=c906fdv -march=rv64imafdcv0p7xthead -mcmodel=medany -mabi=lp64d hello.c -o hello
```

copy your program:

```
cp -fv hello ramdisk/rootfs/public/hello/musl_riscv64/bin/hello
```

select your program

```
echo 'CONFIG_TARGET_PACKAGE_HELLO=y' >> build/boards/cv181x/cv1812cp_licheerv_nano_sd/cv1812cp_licheerv_nano_sd_defconfig
```

then reload config & build it into rootfs:

```
defconfig cv1812cp_licheerv_nano_sd
build_all
```

video sample:
