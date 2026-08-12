# 构建 NanoKVM 镜像

完整的 NanoKVM 镜像由两部分组成：

1. `LicheeRV-Nano` 的 Linux 系统镜像。
2. `NanoKVM APP`，也就是设备启动后运行的 KVM 应用、Web 前端、后端服务和硬件辅助组件。

整体流程是：先编译 `LicheeRV-Nano Linux` 镜像，再编译 `NanoKVM APP`，最后把 `kvmapp` 合并进系统镜像。

## 0. 路径约定

为避免后续构建、复制和挂载命令中的路径混乱，本文固定使用下面的目录结构。所有源码都放在用户目录下的 `LicheeRV_NanoKVM` 文件夹中：

```text
~/LicheeRV_NanoKVM/
├── LicheeRV-Nano-Build/  # SDK 和系统镜像构建目录
└── NanoKVM/              # NanoKVM APP 源码目录
```

先在终端里定义这几个固定路径变量，后续命令都基于它们执行：

```shell
export WORKDIR="$HOME/LicheeRV_NanoKVM"
export SDK_DIR="$WORKDIR/LicheeRV-Nano-Build"
export APP_DIR="$WORKDIR/NanoKVM"
```

首次构建前创建工作目录：

```shell
mkdir -p "$WORKDIR"
```

不要把 `LicheeRV-Nano-Build` 和 `NanoKVM` 分散到其他目录，否则后文的复制、合并镜像命令需要同步调整。新开终端后需要重新执行上面的变量定义。

## 1. 构建 LicheeRV-Nano Linux 镜像

### 1.1 源码和分支

- SDK 仓库：<https://github.com/sipeed/LicheeRV-Nano-Build>
- SDK 分支：`NanoKVM`
- 工具链仓库：<https://github.com/sophgo/host-tools>

`host-tools` 中包含 `riscv64-unknown-linux-musl-gcc`，后续编译 `NanoKVM APP` 的后端服务也会用到同一套工具链。

### 1.2 拉取源码

```shell
cd "$WORKDIR"

git clone -b NanoKVM https://github.com/sipeed/LicheeRV-Nano-Build --depth=1
cd "$SDK_DIR"
git clone https://github.com/sophgo/host-tools --depth=1
```

如果源码已经存在，可以跳过 clone，直接进入目录：

```shell
cd "$SDK_DIR"
```

### 1.3 编译系统镜像

进入 SDK 目录并加载编译环境：

```shell
cd "$SDK_DIR"
source build/cvisetup.sh
```

执行后会注册 `defconfig`、`build_all`、`clean_all` 等命令，并打印当前 SDK 支持的板型配置说明。

选择 `LicheeRV Nano` 的板级配置：

```shell
defconfig sg2002_licheervnano_sd
```

如果要编译 A53 版本，可以改用：

```shell
defconfig sg2002_licheea53nano_sd
```

开始完整编译：

```shell
build_all
```

`build_all` 会依次编译 `U-Boot`、`Kernel`、`ramdisk/rootfs`、驱动、第三方库、中间件，并打包最终镜像。

如果需要从干净状态重新完整编译，可以执行：

```shell
cd "$SDK_DIR"
source build/cvisetup.sh
defconfig sg2002_licheervnano_sd
clean_all
build_all
```

不要在另一个 `build_all` 还在运行时执行 `clean_all`。

### 1.4 WSL 下的 PATH 问题

如果在 WSL 中编译，可能会遇到类似错误：

```text
Your PATH contains spaces, TABs, and/or newline (\n) characters.
This doesn't work. Fix you PATH.
make: *** [support/dependencies/dependencies.mk:27: dependencies] Error 1
```

这是因为 `PATH` 里混入了带空格的 Windows 路径，Buildroot 不允许 `PATH` 包含空格、Tab 或换行。可以在当前终端临时过滤掉这些路径后再重新编译：

```shell
export PATH="$(printf '%s' "$PATH" | tr ':' '\n' | grep -v '[[:space:]]' | paste -sd: -)"
```

### 1.5 编译产物

编译成功后，产物通常位于：

```shell
"$SDK_DIR/install/soc_sg2002_licheervnano_sd/"
```

镜像文件通常在：

```shell
"$SDK_DIR/install/soc_sg2002_licheervnano_sd/images/"
```

常见产物包括：

```text
fip.bin
boot.sd
rootfs.sd
system.sd
upgrade.zip
*.img
```

查看最新生成的整盘镜像：

```shell
ls -lh "$SDK_DIR"/install/soc_sg2002_licheervnano_sd/images/*.img
```

### 1.6 编译 minimal 镜像

如果需要更小的 rootfs，可以选择 minimal 板型：

```shell
cd "$SDK_DIR"
source build/cvisetup.sh
defconfig sg2002_licheervnano_sd_minimal
clean_all
build_all
```

minimal 产物位于：

```shell
"$SDK_DIR/install/soc_sg2002_licheervnano_sd_minimal/"
```

minimal 镜像不预装 Python3、FRP 和 Tailscale，但保留 NanoKVM、SSH、mDNS、Bash 及基础网络功能。合并 APP 时需要显式指定 minimal 镜像：

```shell
./kvm/merge_nanokvm_app.sh \
    -i install/soc_sg2002_licheervnano_sd_minimal/images/your-image-name.img
```

如果编译过程中停在下载进度位置，例如 `Buildroot` 正在下载 `Pillow`、`qt5base`、`qt5svg` 等源码包，通常是网络较慢，不一定是编译卡死。可以观察对应的临时下载文件大小是否仍在增长。

如果首次构建在 `qt5svg` 或 `qt5base` 附近失败，可以先重新执行：

```shell
build_all
```

## 2. 构建 NanoKVM APP

### 2.1 拉取源码

```shell
cd "$WORKDIR"
git clone https://github.com/sipeed/NanoKVM.git --depth=1
cd "$APP_DIR"
```

这里的 `kvmapp` 是 NanoKVM 设备上的应用目录，不是完整烧录镜像。完整系统镜像还需要依赖前面编译出的 `LicheeRV-Nano Linux` 镜像。

`NanoKVM` 仓库的主要目录如下：

```text
NanoKVM/
├── server/          # Go 后端服务
├── web/             # 前端页面
├── support/sg2002/  # kvm_system 等硬件辅助组件
├── kvmapp/          # 本地应用包目录
└── Makefile         # Docker builder 编译入口
```

最终 `kvmapp` 中至少需要这些关键内容：

```text
kvmapp/server/NanoKVM-Server
kvmapp/server/dl_lib/
kvmapp/server/web/
kvmapp/kvm_system/kvm_system
kvmapp/kvm_system/kvm_stream
kvmapp/system/init.d/S95nanokvm
```

需要编译或更新的组件主要是：

- `server/`：Go 后端，生成 `NanoKVM-Server`。
- `web/`：前端页面，生成 `dist/` 后放入 `kvmapp/server/web/`。
- `support/sg2002/kvm_system`：硬件辅助进程，生成 `kvm_system`。

通常不需要单独编译的内容：

- `kvmapp/system/init.d/`：启动脚本，直接复制。
- `kvmapp/picoclaw/`：运行时配置和技能文件，`picoclaw` 二进制由设备侧安装或下载逻辑处理。
- `kvmapp/jpg_stream/`：旧版本升级兼容文件，保留即可。
- `tools/logo_generator/`：Python 辅助脚本，不参与 `kvmapp` 主流程编译。

按需编译的内容：

- `support/sg2002/kvm_vision`：只有修改图像采集或编码动态库时才需要重新编译。
- `tools/nanokvm_update_edid`：EDID 更新工具。如果后续从 SDK 镜像挂载目录里的 `mnt/system/tool/` 补齐，可以不重新编译。

### 2.2 准备 Go 环境

`server/` 是 Go 后端项目，编译 `NanoKVM-Server` 需要本机安装 Go。项目 `server/go.mod` 使用：

```go
go 1.24.0
```

先检查当前电脑是否已经有 Go：

```shell
which go
go version
```

期望 Go 版本为 `1.24.0` 或更高，例如：

```text
/usr/local/go/bin/go
go version go1.24.0 linux/amd64
```

如果提示 `go: command not found`，或者版本低于 `1.24.0`，可以安装官方 Go：

```shell
cd /tmp
wget https://go.dev/dl/go1.24.0.linux-amd64.tar.gz
sudo rm -rf /usr/local/go
sudo tar -C /usr/local -xzf go1.24.0.linux-amd64.tar.gz

echo 'export PATH=/usr/local/go/bin:$PATH' >> ~/.bashrc
source ~/.bashrc
hash -r

which go
go version
```

如果 `which go` 指向 `/usr/bin/go`，或者执行 `go mod tidy` 时出现：

```text
invalid go version '1.24.0': must match format 1.23
```

说明当前 Go 太旧，或者用到了 Ubuntu 自带的 `gccgo`。确认 `/usr/local/go/bin` 已经排在 `PATH` 前面，然后重新执行：

```shell
hash -r
go version
```

如果 Go 依赖下载很慢，可以设置代理：

```shell
go env -w GOPROXY=https://goproxy.cn,direct
go env -w GOSUMDB=sum.golang.google.cn
```

### 2.3 准备 server 编译依赖

除了 Go，`server/build.sh` 还需要：

- `patchelf`
- `riscv64-unknown-linux-musl-gcc`

检查依赖：

```shell
which patchelf
which riscv64-unknown-linux-musl-gcc
```

安装 `patchelf`：

```shell
sudo apt update
sudo apt install -y patchelf
```

如果 RISC-V 工具链没有加入 `PATH`，可以使用 SDK 里的 `host-tools`：

```shell
export PATH="$SDK_DIR/host-tools/gcc/riscv64-linux-musl-x86_64/bin:$PATH"

which riscv64-unknown-linux-musl-gcc
riscv64-unknown-linux-musl-gcc -v
```

### 2.4 编译后端 server

推荐使用仓库里的脚本，因为它会自动设置 `RPATH`：

```shell
cd "$APP_DIR/server"
go mod tidy
./build.sh
```

成功后生成：

```text
server/NanoKVM-Server
```

确认架构和 `RPATH`：

```shell
file NanoKVM-Server
patchelf --print-rpath NanoKVM-Server
```

期望输出包含：

```text
ELF 64-bit ... RISC-V ...
$ORIGIN/dl_lib
```

如果看到类似警告：

```text
warning: libopencv_video.so.409, needed by .../libkvm.so, not found
```

但同时看到：

```text
[SUCCESS] Binary 'NanoKVM-Server' created successfully.
[DONE] Build script completed successfully!
```

说明 `server` 已经编译成功。这个警告表示本机交叉链接时没有找到某个运行时库，后续需要确认设备或 `kvmapp/server/dl_lib` 中有对应库。

把后端放入本地 `kvmapp`：

```shell
cd "$APP_DIR"
mkdir -p kvmapp/server
cp server/NanoKVM-Server kvmapp/server/NanoKVM-Server
cp -a server/dl_lib kvmapp/server/
```

### 2.5 编译前端 web

进入前端目录：

```shell
cd "$APP_DIR/web"
pnpm install
pnpm build
```

如果执行 `pnpm install` 时报错：

```text
ERROR packages field missing or empty
```

说明当前 `pnpm` 版本识别到了 `web/pnpm-workspace.yaml`，但这个文件里缺少 `packages` 字段。可以给 `pnpm-workspace.yaml` 补上当前目录作为 workspace 包：

```yaml
packages:
  - "."

allowBuilds:
  bufferutil: true
  es5-ext: true
  esbuild: true
  msw: true
  utf-8-validate: true
```

修改后重新执行：

```shell
pnpm install
pnpm build
```

成功后生成：

```text
web/dist
```

放入本地 `kvmapp`：

```shell
cd "$APP_DIR/web"
rm -rf "$APP_DIR/kvmapp/server/web"
mkdir -p "$APP_DIR/kvmapp/server"
cp -a dist "$APP_DIR/kvmapp/server/web"
```

检查前端文件：

```shell
ls "$APP_DIR/kvmapp/server/web"
ls "$APP_DIR/kvmapp/server/web/assets"
```

应包含：

```text
index.html
assets/
sipeed.ico
```

### 2.6 编译 support 组件

仓库顶层 `Makefile` 使用 Docker builder 编译 support 组件，需要 Docker 已安装并运行。

检查 Docker：

```shell
docker --version
```

构建 builder 镜像：

```shell
cd "$APP_DIR"
make builder-image
```

编译 support：

```shell
make support
```

`make support` 会在 Docker 中使用 `MaixCDK` 编译 `support/sg2002/kvm_system`，并执行：

```text
./build kvm_system
./build kvm_system add_to_kvmapp
```

生成或更新：

```text
kvmapp/kvm_system/kvm_system
```

如果修改过图像采集或编码相关动态库，可以按需编译 `kvm_vision`：

```shell
cd "$APP_DIR/support/sg2002"
./build kvm_vision
./build kvm_vision add_to_kvmapp
```

这会把 `kvm_vision_test/dist/kvm_vision_test_release/dl_lib/` 更新到 `kvmapp/server/dl_lib/`。没有修改这部分时，通常直接使用仓库或镜像中已有的 `dl_lib/` 即可。

注意：顶层 `make all` 等于：

```text
make app
make support
```

但是 `make all` 不会编译前端 `web`，并且 `make app` 没有执行 `patchelf --add-rpath '$ORIGIN/dl_lib'`。因此 `server` 更推荐使用 `server/build.sh` 编译。

### 2.7 可选：编译 EDID 更新工具

如果不从 SDK 镜像挂载目录里的 `mnt/system/tool/` 复制 EDID 工具，可以手动编译：

```shell
cd "$APP_DIR/tools/nanokvm_update_edid"
make

cd "$APP_DIR"
mkdir -p kvmapp/system/tool
cp tools/nanokvm_update_edid/nanokvm_update_edid kvmapp/system/tool/
cp tools/nanokvm_update_edid/E21_NanoKVM.bin kvmapp/system/tool/
```

如果板子烧录的是当前 SDK 编译出的镜像，更推荐后续从 `mountpoint` 补齐，避免工具版本和镜像不一致。

### 2.8 补充 version 文件

在 `kvmapp` 目录中写入应用版本号：

```shell
cd "$APP_DIR/kvmapp"
echo 2.4.3 > version
```

### 2.9 检查 kvmapp 文件

构建完后的 `kvmapp` 目录应该包含以下关键文件。不同版本的文件数量可能略有差异，但这些核心路径应当存在：

```text
kvmapp
├── jpg_stream
│   ├── S95nanokvm
│   └── jpg_stream
├── kvm_new_app
├── kvm_system
│   ├── kvm_stream
│   └── kvm_system
├── picoclaw
│   ├── AGENT.md
│   ├── AGENT_KVM.md
│   └── skills
│       └── kvm-control
├── server
│   ├── NanoKVM-Server
│   ├── dl_lib
│   └── web
│       ├── assets
│       ├── index.html
│       ├── mockServiceWorker.js
│       └── sipeed.ico
├── system
│   ├── init.d
│   │   ├── S00kmod
│   │   ├── S01fs
│   │   ├── S03usbdev
│   │   ├── S03usbhid
│   │   ├── S15kvmhwd
│   │   ├── S30eth
│   │   ├── S30wifi
│   │   ├── S50avahi-daemon
│   │   ├── S50ssdpd
│   │   ├── S50sshd
│   │   ├── S80dnsmasq
│   │   ├── S95nanokvm
│   │   ├── S96picoclaw
│   │   └── S98tailscaled
│   ├── ko
│   │   └── soph_mipi_rx.ko
│   └── update-nanokvm.py
└── version
```

可以用下面的命令快速检查关键文件：

```shell
test -x "$APP_DIR/kvmapp/server/NanoKVM-Server"
test -d "$APP_DIR/kvmapp/server/dl_lib"
test -f "$APP_DIR/kvmapp/server/web/index.html"
test -x "$APP_DIR/kvmapp/kvm_system/kvm_system"
test -f "$APP_DIR/kvmapp/system/init.d/S95nanokvm"
test -f "$APP_DIR/kvmapp/version"
```

## 3. 合并系统镜像和 NanoKVM APP

SDK 的 `kvm/merge_nanokvm_app.sh` 脚本可以自动完成合并流程，包括：

- 从 `NanoKVM/kvmapp` 复制应用文件到 SDK 的 `kvm/kvmapp`。
- 自动选择最新生成的原始 `.img`，或者使用你手动指定的镜像。
- 默认复制出一个新的 `*-nanokvm.img`，只修改这个输出镜像，不原地修改原始镜像。
- 挂载镜像的 ext4 分区。
- 写入 `kvmapp`、内核模块、启动脚本、默认数据和默认 KVM 配置；普通镜像还会写入 `frpc`、`tailscale` 和 `tailscaled`。
- 每个关键步骤后检查挂载状态或目标文件，并校验第二分区之前的 boot 区域没有变化；失败时打印具体失败步骤并终止。
- 写入完成后自动卸载镜像。

### 3.1 默认用法

确认目录结构符合本文约定：

```text
~/LicheeRV_NanoKVM/
├── LicheeRV-Nano-Build/
└── NanoKVM/
```

然后进入 SDK 目录执行：

```shell
cd "$SDK_DIR"
./kvm/merge_nanokvm_app.sh
```

默认情况下，脚本会：

- 从 `"$APP_DIR/kvmapp"` 复制应用文件。
- 使用 `"$SDK_DIR/install/soc_sg2002_licheervnano_sd/images/"` 中最新的原始 `.img`。
- 生成同目录下的 `*-nanokvm.img` 作为输出镜像。
- 使用 `"$SDK_DIR/mountpoint"` 作为临时挂载目录。

执行完成后，终端会打印已经写入 NanoKVM APP 的镜像路径，例如：

```text
NanoKVM APP has been merged into:
/home/xxx/LicheeRV_NanoKVM/LicheeRV-Nano-Build/install/soc_sg2002_licheervnano_sd/images/xxx-nanokvm.img
```

把这个 `.img` 烧录到 SD 卡即可。

### 3.2 指定镜像文件

如果你不想使用最新镜像，可以用 `-i` 指定某一个 `.img`：

```shell
cd "$SDK_DIR"
./kvm/merge_nanokvm_app.sh -i install/soc_sg2002_licheervnano_sd/images/your-image-name.img
```

脚本默认会生成 `your-image-name-nanokvm.img`。如果想指定输出路径，可以使用 `-o`：

```shell
./kvm/merge_nanokvm_app.sh \
    -i install/soc_sg2002_licheervnano_sd/images/your-image-name.img \
    -o install/soc_sg2002_licheervnano_sd/images/your-output-name.img
```

如果输出镜像已经存在，需要加 `-f` 覆盖：

```shell
./kvm/merge_nanokvm_app.sh -i your-image-name.img -o your-output-name.img -f
```

也可以使用绝对路径：

```shell
./kvm/merge_nanokvm_app.sh -i "$SDK_DIR/install/soc_sg2002_licheervnano_sd/images/your-image-name.img"
```

### 3.3 指定 NanoKVM 源码目录

如果 `NanoKVM` 源码没有放在 `~/LicheeRV_NanoKVM/NanoKVM`，可以用 `-a` 指定：

```shell
cd "$SDK_DIR"
./kvm/merge_nanokvm_app.sh -a /path/to/NanoKVM
```

不过为了减少路径错误，仍然推荐按本文约定放置源码。

### 3.4 使用已有的 SDK kvm/kvmapp

如果你已经手动准备好了 `"$SDK_DIR/kvm/kvmapp"`，不想让脚本重新从 `NanoKVM/kvmapp` 覆盖它，可以加 `--skip-copy-kvmapp`：

```shell
cd "$SDK_DIR"
./kvm/merge_nanokvm_app.sh --skip-copy-kvmapp
```

这种方式会直接使用现有的 `kvm/kvmapp` 写入镜像。

### 3.5 查看脚本帮助

脚本支持的参数可以通过 `--help` 查看：

```shell
cd "$SDK_DIR"
./kvm/merge_nanokvm_app.sh --help
```

### 3.6 注意事项

- 执行脚本前，先确保第 1 节的系统镜像已经编译完成。
- 执行脚本前，先确保第 2 节的 `NanoKVM/kvmapp` 已经准备完整。
- 脚本会覆盖 SDK 目录下的 `kvm/kvmapp`，除非使用 `--skip-copy-kvmapp`。
- 脚本不会原地修改输入 `.img`，而是生成新的输出镜像；烧录时请使用脚本最后打印的 `*-nanokvm.img`。
- 如果脚本提示 `mountpoint` 目录已经挂载，先执行 `umount "$SDK_DIR/mountpoint"` 后再重试。
