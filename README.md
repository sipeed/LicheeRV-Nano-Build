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

#### Gentoo rootfs

https://mirrors.ustc.edu.cn/gentoo/releases/riscv/autobuilds/current-stage3-rv64_lp64_musl/

https://mirrors.ustc.edu.cn/gentoo/releases/riscv/autobuilds/current-stage3-rv64_lp64d-openrc/

#### Alpine rootfs

https://mirrors.ustc.edu.cn/alpine/edge/releases/riscv64/

### Systemd发行

由于vendor提供的BSP内核的多媒体驱动和ns,cgroup等内核特性冲突，如果想要用基于systemd的发行，则需要放弃多媒体驱动。

需要在内核里面勾选:

```

```
