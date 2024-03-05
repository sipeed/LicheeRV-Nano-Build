# OSS

collection of open source softwares for cvitek SDK build.

## regression

assuming host-tools and ramdisk is present

```bash
$ export PATH=$PWD/../host-tools/gcc/gcc-linaro-6.3.1-2017.05-x86_64_aarch64-linux-gnu/bin:$PATH
$ SDK_VER=64bit \
    SYSROOT_PATH=$PWD/../ramdisk/sysroot/sysroot-glibc-linaro-2.23-2017.05-aarch64-linux-gnu \
    ./test_build_all.sh

$ export PATH=$PWD/../host-tools/gcc/gcc-linaro-6.3.1-2017.05-x86_64_arm-linux-gnueabihf/bin:$PATH
$ SDK_VER=32bit \
    SYSROOT_PATH=$PWD/../ramdisk/sysroot/sysroot-glibc-linaro-2.23-2017.05-arm-linux-gnueabihf \
    ./test_build_all.sh
```

## zlib

download from `https://www.zlib.net/`, version 1.2.11.

As the source code is not maintained with git, we created a git project `zlib` by our own.

## glog

clone from `https://github.com/google/glog`, tag `v0.4.0`.

## flatbuffers

## opencv
