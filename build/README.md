简体中文 | [English](./README-en.md)

# 目录

- [目录](#目录)
- [项目简介](#项目简介)
  - [硬件资料](#硬件资料)
  - [芯片规格](#芯片规格)
- [SDK目录结构](#SDK目录结构)
- [快速开始](#快速开始)
  - [准备编译环境](#准备编译环境)
  - [获取SDK](#获取SDK)
  - [准备编译工具](#准备编译工具)
  - [编译](#编译)
  - [SD卡烧录](#SD卡烧录)
    - [使用镜像](#使用镜像)
    - [使用upgrade zip](#使用upgrade-zip)
- [常见问题解答](#常见问题解答)
  - [脚本拉取失败](#脚本拉取失败)
- [相关项目](#相关项目)
- [关于算能](#关于算能)
- [技术论坛](#技术论坛)

# 项目简介
- 本仓库提供[算能科技](https://www.sophgo.com/)端侧芯片 `SG200X`，`CV181x` 和 `CV180x` 三个系列芯片的软件开发包（SDK）。
- 主要适用于官方 EVB 和 Duo 开发板。

## 硬件资料
- [《CV180xB EVB板硬件指南》](https://sophon-file.sophon.cn/sophon-prod-s3/drive/23/03/14/14/CV180xB_EVB%E6%9D%BF%E7%A1%AC%E4%BB%B6%E6%8C%87%E5%8D%97_V1.0.pdf)
- [《CV180xC EVB板硬件指南》](https://sophon-file.sophon.cn/sophon-prod-s3/drive/23/03/18/18/CV180xC_EVB%E6%9D%BF%E7%A1%AC%E4%BB%B6%E6%8C%87%E5%8D%97_V1.0.pdf)
- [《CV181xC EVB板硬件指南》](https://sophon-file.sophon.cn/sophon-prod-s3/drive/23/03/15/14/CV181xC_QFN_EVB%E6%9D%BF%E7%A1%AC%E4%BB%B6%E6%8C%87%E5%8D%97_V1.0.pdf)
- [《CV181xH EVB板硬件指南》](https://sophon-file.sophon.cn/sophon-prod-s3/drive/23/03/15/15/CV181xH_EVB%E6%9D%BF%E7%A1%AC%E4%BB%B6%E6%8C%87%E5%8D%97_V1.0.pdf)

## 芯片规格
- [芯片产品简介](https://www.sophgo.com/product/index.html)

# SDK目录结构
```
.
├── build               // 编译目录，存放编译脚本和板级配置
├── freertos            // freertos 系统
├── fsbl                // fsbl 启动固件
├── install             // 执行一次完整编译后，镜像的存放路径
├── isp_tuning          // 图像效果调试参数存放路径
├── linux_5.10          // 开源 Linux 内核
├── middleware          // 自研多媒体框架，包含 so 与 ko
├── opensbi             // 开源 opensbi 库
├── osdrv               // 驱动程序
├── oss                 // 第三方库
├── ramdisk             // 存放最小文件系统的预编译目录
└── u-boot-2021.10      // 开源 uboot 代码
```

# 快速开始

## 准备编译环境
- 在本地或虚拟机中安装 Ubuntu 系统，推荐 `Ubuntu 22.04 LTS`。
- 安装串口工具：例如 `MobaXterm` 或 `Xshell` 等，可根据自身喜好选择。

- 安装编译依赖:
```
sudo apt install pkg-config build-essential ninja-build automake autoconf libtool wget curl git gcc libssl-dev bc slib squashfs-tools android-sdk-libsparse-utils android-sdk-ext4-utils jq cmake python3-distutils tclsh scons parallel ssh-client tree python 3-dev python3-pip device-tree-compiler libssl-dev ssh cpio squashfs-tools fakeroot libncurses5 flex bison
```
- 注意：cmake 最低版本要求为 `3.16.5`，而在 `Ubuntu 20.04` 中使用 apt 安装的 cmake 版本号为 `3.16.3`，不满足 SDK 的最低版本要求，因此需要手动安装最新版本。
```
wget https://github.com/Kitware/CMake/releases/download/v3.27.6/cmake-3.27.6-linux-x86_64.sh
chmod +x cmake-3.27.6-linux-x86_64.sh
sudo sh cmake-3.27.6-linux-x86_64.sh --skip-license --prefix=/usr/local/
```
- 手动安装的 cmake 在 `/usr/local/bin` 目录下，使用 `cmake --version` 命令可查看其版本号：
```
cmake version 3.27.6
```

## 获取SDK
- 本 SDK 仓库采用自动化脚本的方式管理子仓库，在拉取 SDK 时请使用如下的命令：
```
git clone https://github.com/sophgo/sophpi.git -b sg200x-evb
cd sophpi
./scripts/repo_clone.sh --gitclone scripts/subtree.xml
```
- 若以上的方式拉取失败，或者您当前是在 build 项目中查看本文档，您可以单独拉取每个子仓库，详见 [#常见问题解答](#常见问题解答)
## 准备编译工具
如果您在上一步中没有碰到任何问题，您可以跳过这一步。
- 获取工具链
```
wget https://sophon-file.sophon.cn/sophon-prod-s3/drive/23/03/07/16/host-tools.tar.gz
```
- 解压工具链并链接到 SDK 目录
```
tar xvf host-tools.tar.gz
cd sophpi/
ln -s ../host-tools ./
```

## 编译
- 以 `sg2000_duo_sd`为例，首先切换到 SDK 的根目录：
```
cd sophpi/
```
- 使用以下路径启用环境设置脚本：
```
source build/envsetup_soc.sh
```
- 会出现以下的提示：
```
-------------------------------------------------------------------------------------------------------
  Usage:
  (1) menuconfig - Use menu to configure your board.
      ex: $ menuconfig

  (2) defconfig $CHIP_ARCH - List EVB boards($BOARD) by CHIP_ARCH.
      ** sg200x ** -> ['sg2000', 'sg2002']
      ** cv181x ** -> ['cv1812cp', 'cv1812h', 'cv1813h']
      ** cv180x ** -> ['cv1800b']
      ex: $ defconfig cv181x

  (3) defconfig $BOARD - Choose EVB board settings.
      ex: $ defconfig cv1813h_wevb_0007a_spinor
      ex: $ defconfig cv1812cp_wevb_0006a_spinor
-------------------------------------------------------------------------------------------------------
```
- 根据以上的提示，输入 `defconfig sg200x`，就会列出当前支持的开发板列表：
```
* sg200x * the avaliable cvitek EVB boards
  sg2000 - sg2000_duo_sd [C906B + EMMC 8192MB + DDR 512MB]
  sg2002 - sg2002_duo_sd [C906B + EMMC 8192MB + DDR 256MB]
```
- 选择对应的开发板型号，输入 `clean_all && build_all` 开始编译：
```
defconfig sg2000_duo_sd
clean_all && build_all
pack_burn_image
```
- 编译成功后即可在 `install` 目录下看到生成的镜像

## SD卡烧录
### 使用镜像
> 注意：烧录过程中会清除 SD 卡中的全部内容，请务必提前做好备份！
- 准备一张 SD 卡
- 在 Windows 下，可以使用 `balenaEtcher`，`Rufus` 或 `Win32 Disk Imager` 等工具将生成的镜像写入 SD 卡中。
- 在 Linux 下，可以使用 dd 命令将镜像写入 SD 卡中:
```
sudo dd if=sophpi-duo-XXX.img of=/dev/sdX
```

### 使用upgrade zip
- 接好 EVB 板的串口线
- 将 SD 卡格式化为 FAT32 格式
- 将 `install` 目录下的 `upgrade.zip` 压缩包解压缩并放入 SD 卡根目录，取决于具体开发板型号，压缩包中的文件可能不同：
```
soc_cv1812h_wevb_0007a_emmc
.
├── boot.emmc
├── cfg.emmc
├── fip.bin
├── META/
├── partition_emmc.xml
├── rootfs.emmc
├── system.emmc
└── utils/

soc_cv1800b_sophpi_duo_sd
.
├── boot.sd
├── META/
├── partition_sd.xml
├── rootfs.sd
└── utils/
```
- 将 SD 卡插入卡槽中
- 给开发板上电，开机就会自动进入烧录。
- 等待烧录成功后，拔掉 SD 卡，重新给开发板上电，即可进入系统。

# 常见问题解答
## 脚本拉取失败
如果您无法使用自动化脚本的方式拉取代码，也可以使用如下命令分别拉取各个子仓库：
```
mkdir sophpi -p && cd sophpi
git clone https://github.com/sophgo/build -b sg200x-dev
git clone https://github.com/sophgo/freertos -b sg200x-dev
git clone https://github.com/sophgo/FreeRTOS-Kernel -b sg200x-dev freertos/Source
git clone https://github.com/sophgo/Lab-Project-FreeRTOS-POSIX -b sg200x-dev freertos/Source/FreeRTOS-Plus-POSIX
git clone https://github.com/sophgo/fsbl -b sg200x-dev
git clone https://github.com/sophgo/isp_tuning -b sg200x-dev
git clone https://github.com/sophgo/linux_5.10 -b sg200x-dev
git clone https://github.com/sophgo/middleware -b sg200x-dev
git clone https://github.com/sophgo/opensbi -b sg200x-dev
git clone https://github.com/sophgo/osdrv -b sg200x-dev
git clone https://github.com/sophgo/oss
git clone https://github.com/sophgo/ramdisk -b sg200x-dev
git clone https://github.com/sophgo/u-boot-2021.10 -b sg200x-dev
```
之后按照即可按照正常程序获取 SDK 并编译。

如果您还有疑问，欢迎通过邮箱联系本仓库的维护者：
- [kenneth.liu@sophgo.com](kenneth.liu@sophgo.com)
- [sijie.wang@sophgo.com](sijie.wang@sophgo.com)
- [runze.lin@sophgo.com](runze.lin@sophgo.com)

# 相关项目
- [sophpi-huashan](https://github.com/sophgo/sophpi-huashan): 一款基于CV1812H的开源硬件
- [sophpi-duo](https://github.com/sophgo/sophpi-duo)：一款基于CV1800B的开源硬件

# 关于算能
**算能致力于成为全球领先的通用算力提供商。<br>
算能专注于AI、RISC-V CPU等算力产品的研发和推广应用，以自研产品为核心打造了覆盖“云、边、端”的全场景应用矩阵 ，为城市大脑、智算中心、智慧安防、智慧交通、安全生产、工业质检、智能终端等应用提供算力产品及整体解决方案 。公司在北京、上海、深圳、青岛、厦门等国内 10 多个城市及美国、新加坡等国家设有研发中心。**
- [官方网站](https://www.sophgo.com/)

# 技术论坛
- [技术讨论 - 开源硬件sophpi](https://developer.sophgo.com/forum/index/25/51.html)
