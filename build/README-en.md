[简体中文](./README.md) | English

# Table of Contents

- [Table of Contents](#table-of-contents)
- [Project Introduction](#project-introduction)
  - [Hardware Information](#hardware-information)
  - [SOC SPEC](#soc-spec)
- [SDK Directory Structure](#sdk-directory-structure)
- [Quick Start](#quick-start)
  - [Prepare the compilation environment](#prepare-the-compilation-environment)
  - [Get SDK source code](#get-sdk-source-code)
  - [Prepare cross-compilation tools](#prepare-cross-compilation-tools)
  - [Compile](#compile)
  - [SD card burning](#sd-card-burning)
    - [Using Image](#using-image)
    - [Using upgrade zip](#using-upgrade-zip)
- [FAQ](#faq)
  - [Scripts clone failed](#scripts-clone-failed)
- [About SOPHGO](#about-sophgo)
  - [Related Efforts](#related-efforts)
- [Forum](#forum)

# Project Introduction
- This is a release SDK repository for the [SOPHGO](https://www.sophgo.com/) `sg200x`, `cv181x` and `cv180x` series of Edge chips.
- Mainly applicable to official EVB and Duo borad.

## Hardware Information
- [*CV180xB EVB Borad Hardware Guide*](https://sophon-file.sophon.cn/sophon-prod-s3/drive/23/03/14/14/CV180xB_EVB%E6%9D%BF%E7%A1%AC%E4%BB%B6%E6%8C%87%E5%8D%97_V1.0.pdf)
- [*CV180xC EVB Borad Hardware Guide*](https://sophon-file.sophon.cn/sophon-prod-s3/drive/23/03/18/18/CV180xC_EVB%E6%9D%BF%E7%A1%AC%E4%BB%B6%E6%8C%87%E5%8D%97_V1.0.pdf)
- [*CV181xC EVB Borad Hardware Guide*](https://sophon-file.sophon.cn/sophon-prod-s3/drive/23/03/15/14/CV181xC_QFN_EVB%E6%9D%BF%E7%A1%AC%E4%BB%B6%E6%8C%87%E5%8D%97_V1.0.pdf)
- [*CV181xH EVB Borad Hardware Guide*](https://sophon-file.sophon.cn/sophon-prod-s3/drive/23/03/15/15/CV181xH_EVB%E6%9D%BF%E7%A1%AC%E4%BB%B6%E6%8C%87%E5%8D%97_V1.0.pdf)

## SOC SPEC
- [SOC Product Brief](https://www.sophgo.com/product/index.html)

# SDK Directory Structure
```
.
├── build               // compilation scripts and board configs
├── freertos            // freertos system
├── fsbl                // fsbl boot firmware
├── install             // images stored here
├── isp_tuning          // camera related parameters
├── linux_5.10          // Linux kernel
├── middleware          // multimedia infrastructure，including .so & .ko
├── opensbi             // opensbi library
├── osdrv               // drivers
├── oss                 // third-party libraries
├── ramdisk             // prebuilt ramdisk
└── u-boot-2021.10      // uboot source code
```

# Quick Start

## Prepare the compilation environment
1. Install a virtual machine, or use a local ubuntu system, recommend `Ubuntu 22.04 LTS`
2. Install the serial port tool: `MobaXterm`, `Xshell` or other tools.
3. Install the compile dependencies.
```
sudo apt install pkg-config build-essential ninja-build automake autoconf libtool wget curl git gcc libssl-dev bc slib squashfs-tools android-sdk-libsparse-utils android-sdk-ext4-utils jq cmake python3-distutils tclsh scons parallel ssh-client tree python3-dev python3-pip device-tree-compiler libssl-dev ssh cpio squashfs-tools fakeroot libncurses5 flex bison
```
- Note: The minimum version of `cmake` requires `3.16.5`, if you are using `Ubuntu 20.04 LTS`, the apt will install `cmake` version `3.16.3`, which is lower than the minimal requirement, so you have to install the lastest version manually:
```
wget https://github.com/Kitware/CMake/releases/download/v3.27.6/cmake-3.27.6-linux-x86_64.sh
chmod +x cmake-3.27.6-linux-x86_64.sh
sudo sh cmake-3.27.6-linux-x86_64.sh --skip-license --prefix=/usr/local/
```
- You can found it under `/usr/bin/local`, to check it's version, using `cmake --version`:
```
cmake version 3.27.6
```

## Get SDK source code
- This repository is using scripts for sub-repositories, to get the SDK, using commands below:
```
git clone https://github.com/sophgo/sophpi.git -b sg200x-evb
cd sophpi
./scripts/repo_clone.sh --gitclone scripts/subtree.xml
```
- If the way above does not working for you, or you are under the `build` repository, please clone these repos one by one, you could check the [#FAQ](#faq) section for more information.

## Prepare cross-compilation tools
If you did not encountered any issues in the previous step, you can just skip this step.
- Get the cross-compilation toolchain
```
wget https://sophon-file.sophon.cn/sophon-prod-s3/drive/23/03/07/16/host-tools.tar.gz
```
- Unpack the toolchain and link to the SDK directory
```
tar xvf host-tools.tar.gz
cd sophpi/
ln -s ../host-tools ./
```

## Compile
- Take `sg2000_duo_sd` as an example
```
cd sophpi/
```
- Enable the `envsetup_soc.sh` script.
```
source build/envsetup_soc.sh
```
- The script will print it's uasge.
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
- Following the uasge, Enter `defconfig sg200x` for supported boards.
```
* sg200x * the avaliable cvitek EVB boards
  sg2000 - sg2000_duo_sd [C906B + EMMC 8192MB + DDR 512MB]
  sg2002 - sg2002_duo_sd [C906B + EMMC 8192MB + DDR 256MB]
```
- According to your borads version, enter `clean_all && build_all` to start compiling.
```
defconfig sg2000_duo_sd
clean_all && build_all
pack_burn_image
```
- You could find final images under `install` directory.

## SD card burning
### Using Image
> Note: Writing image to the SD card will erase all existing data on the card. Don't forget to back up important data before burning!
- For Windows Users, you can use tools like `balenaEtcher`, `Rufus`, or `Win32 Disk Imager`.
- For Linux Users, you can use `dd` command like this:
```
sudo dd if=sophpi-duo-XXX.img of=/dev/sdX
```
### Using upgrade zip
- Connect the serial cable of the EVB board.
- Format the SD card into FAT32 format.
- Unzip the compressed file `upgrade.zip` in the `install` directory and put it into the root directory of the SD card. Contents of the compressed file may be different depend on different borads, for example:
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
- Insert the SD card into the SD card slot.
- Power on the platform again, and it will automatically start the burning process when it is turned on.
- After burning successfully finished, unplug the SD card, power on the board again, and enter the system.

# FAQ
## Scripts clone failed
If you are unable to clone the repository using scripts, you could clone these repos one by one.
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
Then, following the original building process and you will be well.

If you still have any questions, please contact us by following emails:
- [kenneth.liu@sophgo.com](kenneth.liu@sophgo.com)
- [sijie.wang@sophgo.com](sijie.wang@sophgo.com)
- [runze.lin@sophgo.com](runze.lin@sophgo.com)

# About SOPHGO
**SOPHGO is committed to becoming the world's leading general computing power provider.<br>
SOPHGO focuses on the development and promotion of AI, RISC-V CPU and other computing products. With the self-developed chips as the core, SOPHGO has created a matrix of computing power products, which covers the whole scene of "cloud, edge and terminal" and provides computing power products and overall solutions for urban brains, intelligent computing centers, intelligent security, intelligent transportation, safety production, industrial quality inspection, intelligent terminals and others. SOPHGO has set up R&D centers in more than 10 cities and countries, including Beijing, Shanghai, Shenzhen, Qingdao, Xiamen, the United States and Singapore.**
- [Official Website](https://www.sophgo.com/)

## Related Efforts
- [sophpi-huashan](https://github.com/sophgo/sophpi-huashan)
- [sophpi-duo](https://github.com/sophgo/sophpi-duo)

# Forum
- [Discussions - Open Hardware sophpi](https://developer.sophgo.com/forum/index/25/51.html)
