# 快速开始

## 获取SDK
```
git clone https://github.com/sipeed/LicheeRvNano-Build cvi_mmf_sdk
```

## 准备编译环境

```
# 获取工具链
cd /path/to/cvi_mmf_sdk
wget https://sophon-file.sophon.cn/sophon-prod-s3/drive/23/03/07/16/host-tools.tar.gz
tar xvf host-tools.tar.gz
cd /path/to/cvi_mmf_sdk/docker
# 构建docker环境用于开发
docker build -t cvi_mmf_sdk .
```

## 编译

- 以 `cv1812cp_licheerv_nano_sd`为例

```
# 进入Docker环境
docker run --rm --name cvi_mmf_sdk -v /path/to/cvi_mmf_sdk:/path/to/cvi_mmf_sdk -it cvi_mmf_sdk /bin/bash
```

```
# 以下命令在docker环境里面执行
cd /path/to/cvi_mmf_sdk
# 设置环境
source build/cvisetup.sh
# 加载配置
defconfig cv1812cp_licheerv_nano_sd
# 构建软件
build_all
# 生成SD卡镜像
./build/tools/common/sd_tools/sd_gen_burn_image.sh install/soc_cv1812cp_licheerv_nano_sd/

```
- 编译成功后可以在`install`目录下看到生成的image

## SD卡烧录

```
# 在Docker环境之外
# 将 DATE TIME sdX 替换为实际的字符串
if=install/soc_cv1812cp_licheerv_nano_sd/images/licheervnano-DATE-TIME.img of=/dev/sdX status=progress
```

## middleware

```
# 安装位置
/mnt/system/usr/bin
```

## 摄像头DEMO

将摄像头内容显示到屏幕上:

```
/mnt/system/usr/bin/sample_vio 6
```

## wifi连接

```
wpa_passphrase ssid password >> /etc/wpa_supplicant.conf
wpa_supplicant -i wlan0 -c /etc/wpa_supplicant.conf -B
udhcpc -i wlan0
```

## 音频播放

```
volume play 15 # 设置播放音量
aplay 1.wav  # 播放指定wav文件
```

## 录制音频

```
volume record 20 # 设置录制音量
arecord -Dhw:0,0 -f S16_LE 1.wav
```

## 更换屏幕

mipi默认使用7inch zct2133v1屏幕

如果需要使用其他屏幕，请在menuconfig里面自行勾选

屏幕初始化序列发送和时序设置在uboot内进行，通常编译完成后只需要替换sd卡boot分区的fip.bin即可切换屏幕。

## 屏幕示例程序

```
/opt/lcd/fbpattern # 屏幕时序测试工具
/opt/lcd/fbbar     # 在屏幕上打印一行字符串
```

可以使用lvgl, qtlinuxfb, sdl1.2进行开发

## 运行Linux发行版

### 非systemd发行

1. 先在SD卡上烧录好cvimmfsdk的镜像(此repo生成的)
2. 然后挂载sd卡的第二个分区，将mnt拷贝出来

```
mount /dev/sdX2 /mnt/root
cp -arv /mnt/root/mnt /tmp/mnt
```

3. 格式化SD卡的第二分区并挂载，将刚才的mnt放进去

```
mkfs.ext4 /dev/sdX2
mount /dev/sdX2 /mnt/root
cp -arv /tmp/mnt /mnt/root/
```

4. 下载发行提供的rootfs，然后解压到第二分区

```
tar xpvf xxx-rootfs.tar.xx -C /mnt/root
```

5. 编辑刚解压的rootfs里面的etc/inittab，加上串口的getty，注意波特率

```
s0:12345:respawn:/sbin/agetty -L 115200 ttyS0 vt100
gs0:12345:respawn:/sbin/agetty -L 115200 ttyGS0 vt100
```

6. 更改系统的root密码，用于第一次登录

```
cp /usr/bin/qemu-riscv64 /mnt/root/usr/bin/
echo 'root:root' | chroot /mnt/root /bin/chpasswd
```

7. (可选) 编辑启动脚本，用于加载mnt下面的内核模块
8. (可选) 启用SSH服务
9. 卸载内存卡，安装到开发板

```
sync
umount /dev/sdX2
eject /dev/sdX
```
10. 连接串口或SSH，进行系统配置

#### Alpine rootfs

1. 准备镜像，将编译出的cvi_mmf_sdk镜像拷贝:

```
cp ./install/soc_cv1812cp_licheerv_nano_sd/images/licheervnano-DATE-TIME.img licheervnano-alpine-DATE-TIME.img
```

2. 扩展镜像

```
dd if=/dev/zero bs=1M count=512 >> licheervnano-alpine-DATE-TIME.img
```

然后使用parted工具修改分区表:

```
parted licheervnano-alpine-DATE-TIME.img
# 进入parted环境
resizepart 2 -1 # 将剩余空间追加到第二分区
```

2. 使用parted工具确定第二个分区的offset


```
parted licheervnano-alpine-DATE-TIME.img
# 进入parted环境
unit b
print
# 然后记下第二分区的Start Offset，示例:

Number  Start      End         Size        Type     File system  Flags
 1      512B       16777727B   16777216B   primary  fat16        boot, lba
 2      16777728B  254853631B  238075904B  primary  ext4
```


3. 使用losetup和mount挂载镜像的第二个分区

```
# 这里的offset填写第二分区的Start
losetup -o OFFSET /dev/loop15 licheervnano-alpine-DATE-TIME.img
```

4. 扩展分区大小

```
e2fsck -f /dev/loop15
resize2fs /dev/loop15
```

5. 挂载分区


```
mkdir /mnt/chroot
mount /dev/loop15 /mnt/chroot
```

6. 将mnt和lib/firmware以及开机脚本拷贝出来

```
cd /mnt/chroot
cp -r ./lib/firmware ../
cp -r ./mnt ../
```

7. 删除分区内的旧rootfs

```
rm -rfv /mnt/chroot/*
```

8. 下载alpine riscv64 rootfs

下载地址:

https://mirrors.ustc.edu.cn/alpine/edge/releases/riscv64/

```
cd /mnt/chroot
wget https://mirrors.tuna.tsinghua.edu.cn/alpine/edge/releases/riscv64/alpine-minirootfs-20231219-riscv64.tar.gz -O alpine.tar.gz
```

9. 解压alpine riscv64 rootfs到第二分区

```
cd /mnt/chroot
tar xvpf alpine.tar.gz -C ./
```

10. 拷贝firmware,mnt和开机脚本

```
cd /mnt/chroot
cp -arv ../mnt/* ./mnt/
mkdir -pv ./lib/firmware/
cp -arv ../firmware/* ./lib/firmware/
```

11. 修改配置文件

在 ./etc/inittab 加上:

```
ttyS0::respawn:/sbin/getty -L ttyS0 115200 vt100 # UART0
ttyGS0::respawn:/sbin/getty -L ttyGS0 115200 vt100 # CDC ACM
```

创建 ./etc/resolv.conf :

```
nameserver 8.8.8.8
```

创建 ./etc/network/interfaces:

```
auto lo
iface lo inet loopback

auto eth0
iface eth0 inet dhcp

auto wlan0
iface wlan0 inet dhcp
```

创建 ./etc/local.d/aaa_boot_setup.start (需要chmod +x)

```
#!/bin/sh

set -x
 
rm -vf /mnt/etc/inittab
cp -arv /mnt/etc/* /etc/
/etc/init.d/S01modload start # 加载内核模块
/etc/init.d/S04usbdev start  # 启用usb rndis
```

12. 进入chroot环境

```
cd /mnt/chroot
cp -fv /usr/bin/qemu-riscv64 ./usr/bin/
mount -t devtmpfs devtmpfs ./dev
mount -t proc proc ./proc
mount -t sysfs sysfs ./sys
chroot /mnt/chroot /bin/ash
```

13. 设置root账户密码

```
passwd root
```

14. 安装软件包

```
apk update
apk add alpine-base dropbear haveged tmux ckermit neofetch wpa_supplicant
```

15. 设置开机服务

```
rc-update add bootmisc boot
rc-update add devfs sysinit
rc-update add dmesg sysinit
rc-update add hostname boot
rc-update add swclock boot
rc-update add killprocs shutdown
rc-update add mdev sysinit
rc-update add mount-ro shutdown
rc-update add savecache shutdown
rc-update add syslog boot
rc-update add dropbear default
rc-update add dropbear nonetwork
rc-update add ntpd default
rc-update add networking default
rc-update add wpa_supplicant boot
rc-update add local boot
rc-update add haveged sysinit
```

16. 退出chroot环境，卸载分区

```
exit
umount /mnt/chroot/*
umount /mnt/chroot/
losetup -d /dev/loop15
```

18. 修复镜像

分区未对齐会导致文件系统挂载失败，镜像末尾需要预留一些空白空间用于调整

```
dd if=/dev/zero bs=10M >> licheervnano-alpine-DATE-TIME.img
cfdisk licheervnano-alpine-DATE-TIME.img
# 将第二个分区扩展
```

17. 烧写镜像

```
dd if=licheervnano-alpine-DATE-TIME.img of=/dev/sdX
```

#### Gentoo rootfs

安装方法类似alpine linux

https://mirrors.ustc.edu.cn/gentoo/releases/riscv/autobuilds/current-stage3-rv64_lp64_musl/

https://mirrors.ustc.edu.cn/gentoo/releases/riscv/autobuilds/current-stage3-rv64_lp64d-openrc/

### Systemd发行

由于vendor提供的BSP内核的多媒体驱动和ns,cgroup等内核特性冲突，如果想要用基于systemd的发行，则需要放弃多媒体驱动。

需要在内核里面勾选:

```

```
