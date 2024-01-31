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
if=install/soc_cv1812cp_licheerv_nano_sd/images/sophpi-duo-DATE-TIME.img of=/dev/sdX status=progress
```

## middleware

```
# 安装位置
/mnt/system/usr/bin
```
