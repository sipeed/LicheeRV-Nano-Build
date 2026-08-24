#!/bin/sh

set -eu
set +x

usage() {
    cat <<'EOF'
Usage:
  kvm/merge_nanokvm_app.sh [options]

Options:
  -i, --image FILE      Input image. Defaults to the latest non-*-nanokvm.img in
                        install/soc_sg2002_licheervnano_sd/images/.
  -o, --output FILE     Output image. Defaults to INPUT-nanokvm.img.
  -a, --app-dir DIR     NanoKVM source directory. Defaults to ../NanoKVM next to
                        this SDK directory.
  -m, --mount-dir DIR   Temporary mount directory. Defaults to ./mountpoint.
  --skip-copy-kvmapp    Use existing kvm/kvmapp instead of copying from APP_DIR.
  -f, --force           Overwrite the output image if it already exists.
  -h, --help            Show this help.

Examples:
  ./kvm/merge_nanokvm_app.sh
  ./kvm/merge_nanokvm_app.sh -i install/soc_sg2002_licheervnano_sd/images/out.img
  ./kvm/merge_nanokvm_app.sh -i base.img -o base-nanokvm.img
  ./kvm/merge_nanokvm_app.sh -a "$HOME/LicheeRV_NanoKVM/NanoKVM"
EOF
}

die() {
    echo "error: $*" >&2
    exit 1
}

warn() {
    echo "warning: $*" >&2
}

info() {
    echo "==> $*"
}

run_step() {
    desc=$1
    shift

    info "$desc"
    if "$@"; then
        return 0
    else
        status=$?
        die "$desc failed with exit code $status"
    fi
}

need_file() {
    [ -f "$1" ] || die "missing file: $1"
}

need_exec() {
    [ -x "$1" ] || die "missing executable file: $1"
}

need_dir() {
    [ -d "$1" ] || die "missing directory: $1"
}

copy_image() {
    src=$1
    dst=$2

    cp --reflink=auto --sparse=always "$src" "$dst" 2>/dev/null || cp "$src" "$dst"
}

partition2_offset_bytes() {
    partx -s "$1" | awk '$1 == 2 { print $2 * 512; found = 1 } END { exit !found }'
}

is_mounted() {
    if command -v mountpoint >/dev/null 2>&1; then
        mountpoint -q "$MOUNT_DIR" && return 0
    fi

    if command -v findmnt >/dev/null 2>&1; then
        findmnt -rn --mountpoint "$MOUNT_DIR" >/dev/null 2>&1 && return 0
    fi

    if [ -r /proc/mounts ]; then
        awk -v dir="$MOUNT_DIR" '$2 == dir { found = 1 } END { exit !found }' /proc/mounts
        return $?
    fi

    return 1
}

write_config() {
    name=$1
    value=$2
    file="$MOUNT_DIR/kvmapp/kvm/$name"

    info "write default config: kvm/$name=$value"
    if ! printf '%s\n' "$value" > "$file"; then
        die "failed to write default config: $file"
    fi

    need_file "$file"
}

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd -P)
SDK_DIR=$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd -P)
WORKDIR=$(dirname "$SDK_DIR")

APP_DIR="$WORKDIR/NanoKVM"
IMAGE_DIR="$SDK_DIR/install/soc_sg2002_licheervnano_sd/images"
IMAGE_FILE=""
OUTPUT_IMAGE_FILE=""
MOUNT_DIR="$SDK_DIR/mountpoint"
COPY_KVMAPP=1
FORCE_OUTPUT=0

while [ "$#" -gt 0 ]; do
    case "$1" in
        -i|--image)
            [ "$#" -ge 2 ] || die "$1 requires a file path"
            IMAGE_FILE=$2
            shift 2
            ;;
        -o|--output)
            [ "$#" -ge 2 ] || die "$1 requires a file path"
            OUTPUT_IMAGE_FILE=$2
            shift 2
            ;;
        -a|--app-dir)
            [ "$#" -ge 2 ] || die "$1 requires a directory path"
            APP_DIR=$2
            shift 2
            ;;
        -m|--mount-dir)
            [ "$#" -ge 2 ] || die "$1 requires a directory path"
            MOUNT_DIR=$2
            shift 2
            ;;
        --skip-copy-kvmapp)
            COPY_KVMAPP=0
            shift
            ;;
        -f|--force)
            FORCE_OUTPUT=1
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            die "unknown argument: $1"
            ;;
    esac
done

case "$IMAGE_FILE" in
    "") ;;
    /*) ;;
    *) IMAGE_FILE="$SDK_DIR/$IMAGE_FILE" ;;
esac

case "$OUTPUT_IMAGE_FILE" in
    "") ;;
    /*) ;;
    *) OUTPUT_IMAGE_FILE="$SDK_DIR/$OUTPUT_IMAGE_FILE" ;;
esac

case "$MOUNT_DIR" in
    /*) ;;
    *) MOUNT_DIR="$SDK_DIR/$MOUNT_DIR" ;;
esac

command -v partx >/dev/null 2>&1 || die "missing command: partx"
need_exec "$SDK_DIR/host/mount_ext4.sh"

if [ -z "$IMAGE_FILE" ]; then
    need_dir "$IMAGE_DIR"
    IMAGE_FILE=$(find "$IMAGE_DIR" -maxdepth 1 -type f -name '*.img' ! -name '*-nanokvm.img' -printf '%T@ %p\n' | sort -nr | sed -n '1s/^[^ ]* //p')
    [ -n "$IMAGE_FILE" ] || die "no input *.img found in $IMAGE_DIR; build the system image first or pass -i FILE"
fi

need_file "$IMAGE_FILE"
SOURCE_IMAGE_FILE=$IMAGE_FILE

ROOTFS_OFFSET_BYTES=$(partition2_offset_bytes "$SOURCE_IMAGE_FILE") || die "failed to read partition 2 offset from $SOURCE_IMAGE_FILE"
[ "$ROOTFS_OFFSET_BYTES" -gt 0 ] || die "invalid partition 2 offset: $ROOTFS_OFFSET_BYTES"

if [ -z "$OUTPUT_IMAGE_FILE" ]; then
    case "$SOURCE_IMAGE_FILE" in
        *.img) OUTPUT_IMAGE_FILE=${SOURCE_IMAGE_FILE%.img}-nanokvm.img ;;
        *) OUTPUT_IMAGE_FILE=$SOURCE_IMAGE_FILE.nanokvm.img ;;
    esac
fi

[ "$OUTPUT_IMAGE_FILE" != "$SOURCE_IMAGE_FILE" ] || die "output image must be different from input image"

MOUNTED=0
OUTPUT_CREATED=0
STRIP_TEMP_FILE=""
cleanup() {
    status=$?
    cleanup_ok=1
    if [ "$MOUNTED" -eq 1 ]; then
        info "cleanup: unmount $MOUNT_DIR"
        if umount "$MOUNT_DIR"; then
            MOUNTED=0
        else
            cleanup_ok=0
            warn "failed to unmount $MOUNT_DIR; please run: umount \"$MOUNT_DIR\""
        fi
    fi
    if [ "$status" -ne 0 ] && [ "$OUTPUT_CREATED" -eq 1 ] && [ "$cleanup_ok" -eq 1 ]; then
        info "cleanup: remove incomplete output image $OUTPUT_IMAGE_FILE"
        rm -f "$OUTPUT_IMAGE_FILE" || warn "failed to remove incomplete output image: $OUTPUT_IMAGE_FILE"
    fi
    if [ -n "$STRIP_TEMP_FILE" ]; then
        rm -f "$STRIP_TEMP_FILE"
    fi
    exit "$status"
}
trap cleanup EXIT
trap 'exit 1' HUP INT TERM

if [ -e "$OUTPUT_IMAGE_FILE" ]; then
    if [ "$FORCE_OUTPUT" -eq 1 ]; then
        run_step "remove old output image" rm -f "$OUTPUT_IMAGE_FILE"
    else
        die "output image already exists: $OUTPUT_IMAGE_FILE; pass -f to overwrite it"
    fi
fi

OUTPUT_CREATED=1
run_step "copy input image to output image" copy_image "$SOURCE_IMAGE_FILE" "$OUTPUT_IMAGE_FILE"
IMAGE_FILE=$OUTPUT_IMAGE_FILE

if ! cmp -n "$ROOTFS_OFFSET_BYTES" "$SOURCE_IMAGE_FILE" "$IMAGE_FILE" >/dev/null; then
    die "output image differs from input before partition 2; aborting before mount"
fi

info "SDK directory: $SDK_DIR"
info "NanoKVM directory: $APP_DIR"
info "input image: $SOURCE_IMAGE_FILE"
info "output image: $IMAGE_FILE"
info "partition 2 offset: $ROOTFS_OFFSET_BYTES bytes"
info "mount directory: $MOUNT_DIR"

run_step "create mount directory" mkdir -p "$MOUNT_DIR"

if is_mounted; then
    die "$MOUNT_DIR is already mounted; run: umount \"$MOUNT_DIR\""
fi

if [ "$COPY_KVMAPP" -eq 1 ]; then
    need_dir "$APP_DIR/kvmapp"
    need_exec "$APP_DIR/kvmapp/server/NanoKVM-Server"
    need_exec "$APP_DIR/kvmapp/kvm_system/kvm_system"
    need_file "$APP_DIR/kvmapp/system/ko/soph_mipi_rx.ko"
    need_file "$APP_DIR/kvmapp/system/init.d/S95nanokvm"
    need_file "$APP_DIR/kvmapp/version"

    run_step "remove old SDK kvmapp" rm -rf "$SCRIPT_DIR/kvmapp"
    run_step "copy NanoKVM app from $APP_DIR/kvmapp" cp -a "$APP_DIR/kvmapp" "$SCRIPT_DIR/"
    run_step "create kvm_new_img marker" touch "$SCRIPT_DIR/kvmapp/kvm_new_img"
else
    info "use existing SDK kvmapp: $SCRIPT_DIR/kvmapp"
fi

need_dir "$SCRIPT_DIR/kvmapp"
run_step "make SDK kvmapp files executable" chmod -R +x "$SCRIPT_DIR/kvmapp"
need_exec "$SCRIPT_DIR/kvmapp/server/NanoKVM-Server"
need_exec "$SCRIPT_DIR/kvmapp/kvm_system/kvm_system"
need_file "$SCRIPT_DIR/kvmapp/system/ko/soph_mipi_rx.ko"
need_file "$SCRIPT_DIR/kvmapp/system/init.d/S95nanokvm"
need_file "$SCRIPT_DIR/kvmapp/version"
for script in S00kmod S01fs S03usbdev S15kvmhwd S30eth S30wifi S95nanokvm; do
    need_file "$SCRIPT_DIR/kvmapp/system/init.d/$script"
done
need_dir "$SCRIPT_DIR/data"
need_file "$SCRIPT_DIR/data/sensor_cfg.ini"

info "mount image: $IMAGE_FILE"
if "$SDK_DIR/host/mount_ext4.sh" "$IMAGE_FILE" "$MOUNT_DIR"; then
    :
else
    status=$?
    die "failed to mount image: $IMAGE_FILE; mount_ext4.sh exited with code $status"
fi
MOUNTED=1

if ! is_mounted; then
    die "mount command finished, but $MOUNT_DIR is not a mounted filesystem"
fi

need_dir "$MOUNT_DIR/etc"
need_dir "$MOUNT_DIR/usr"
need_dir "$MOUNT_DIR/mnt"
info "mount check passed"

MINIMAL_IMAGE=0
if [ -e "$MOUNT_DIR/etc/nanokvm-minimal" ]; then
    MINIMAL_IMAGE=1
else
    FRP_DIR=$(find "$SCRIPT_DIR" -maxdepth 1 -type d -name 'frp_*_linux_riscv64' | sort -V | tail -n 1)
    TAILSCALE_DIR=$(find "$SCRIPT_DIR" -maxdepth 1 -type d -name 'tailscale_*_riscv64' | sort -V | tail -n 1)
    [ -n "$FRP_DIR" ] || die "missing frp_*_linux_riscv64 directory under $SCRIPT_DIR"
    [ -n "$TAILSCALE_DIR" ] || die "missing tailscale_*_riscv64 directory under $SCRIPT_DIR"
    need_exec "$FRP_DIR/frpc"
    need_exec "$TAILSCALE_DIR/tailscale"
    need_exec "$TAILSCALE_DIR/tailscaled"
fi

SSH_AVAILABLE=0
if [ -x "$MOUNT_DIR/usr/sbin/sshd" ] && [ -x "$MOUNT_DIR/usr/bin/ssh-keygen" ]; then
    SSH_AVAILABLE=1
    need_file "$SCRIPT_DIR/kvmapp/system/init.d/S50sshd"
fi

run_step "remove old image kvmapp" rm -rf "$MOUNT_DIR/kvmapp"
run_step "copy kvmapp into image" cp -a "$SCRIPT_DIR/kvmapp" "$MOUNT_DIR/"
strip_tool="${CROSS_COMPILE_STRIP:-}"
if [ -z "$strip_tool" ]; then
    strip_tool=$(command -v riscv64-unknown-linux-musl-strip 2>/dev/null || true)
fi
if [ -z "$strip_tool" ]; then
    strip_tool="$SDK_DIR/host-tools/gcc/riscv64-linux-musl-x86_64/bin/riscv64-unknown-linux-musl-strip"
fi
case "$strip_tool" in
    */*) need_exec "$strip_tool" ;;
    *) command -v "$strip_tool" >/dev/null 2>&1 || die "missing strip tool: $strip_tool" ;;
esac
STRIP_TEMP_FILE=$(mktemp "${TMPDIR:-/tmp}/nanokvm-server.XXXXXX") || die "failed to create strip temporary file"
run_step "strip NanoKVM-Server debug information" \
    "$strip_tool" --strip-debug -o "$STRIP_TEMP_FILE" "$SCRIPT_DIR/kvmapp/server/NanoKVM-Server"
run_step "install stripped NanoKVM-Server" \
    install -m 755 "$STRIP_TEMP_FILE" "$MOUNT_DIR/kvmapp/server/NanoKVM-Server"
need_exec "$MOUNT_DIR/kvmapp/server/NanoKVM-Server"
need_file "$MOUNT_DIR/kvmapp/system/init.d/S95nanokvm"
need_file "$MOUNT_DIR/kvmapp/version"
info "kvmapp install check passed"

if [ "$MINIMAL_IMAGE" -eq 1 ]; then
    info "minimal image: omit bundled frp and tailscale"
    run_step "remove bundled frp and tailscale" rm -f \
        "$MOUNT_DIR/usr/bin/frpc" \
        "$MOUNT_DIR/usr/bin/tailscale" \
        "$MOUNT_DIR/usr/sbin/tailscaled" \
        "$MOUNT_DIR/etc/init.d/S98tailscaled"
else
    run_step "create frp and tailscale target directories" install -d "$MOUNT_DIR/usr/bin" "$MOUNT_DIR/usr/sbin"
    run_step "install frpc" install -m 755 "$FRP_DIR/frpc" "$MOUNT_DIR/usr/bin/frpc"
    run_step "install tailscale" install -m 755 "$TAILSCALE_DIR/tailscale" "$MOUNT_DIR/usr/bin/tailscale"
    run_step "install tailscaled" install -m 755 "$TAILSCALE_DIR/tailscaled" "$MOUNT_DIR/usr/sbin/tailscaled"
    need_exec "$MOUNT_DIR/usr/bin/frpc"
    need_exec "$MOUNT_DIR/usr/bin/tailscale"
    need_exec "$MOUNT_DIR/usr/sbin/tailscaled"
    info "frp and tailscale install check passed"
fi

run_step "create kernel module and init target directories" install -d "$MOUNT_DIR/mnt/system/ko" "$MOUNT_DIR/etc/init.d"
run_step "install soph_mipi_rx.ko" install -m 755 "$SCRIPT_DIR/kvmapp/system/ko/soph_mipi_rx.ko" "$MOUNT_DIR/mnt/system/ko/soph_mipi_rx.ko"
need_file "$MOUNT_DIR/mnt/system/ko/soph_mipi_rx.ko"

for script in S00kmod S01fs S03usbdev S15kvmhwd S30eth S30wifi S95nanokvm; do
    run_step "install init script: $script" install -m 755 "$SCRIPT_DIR/kvmapp/system/init.d/$script" "$MOUNT_DIR/etc/init.d/$script"
    need_exec "$MOUNT_DIR/etc/init.d/$script"
done
if [ "$SSH_AVAILABLE" -eq 1 ]; then
    run_step "install init script: S50sshd" install -m 755 "$SCRIPT_DIR/kvmapp/system/init.d/S50sshd" "$MOUNT_DIR/etc/init.d/S50sshd"
    need_exec "$MOUNT_DIR/etc/init.d/S50sshd"
else
    warn "input image has no OpenSSH server; SSH remains unavailable"
    run_step "remove unavailable SSH init script" rm -f "$MOUNT_DIR/etc/init.d/S50sshd"
fi
info "kernel module and init script install check passed"

run_step "create default data and kvm config directories" install -d "$MOUNT_DIR/mnt/data" "$MOUNT_DIR/etc/kvm" "$MOUNT_DIR/kvmapp/kvm"
for data_file in "$SCRIPT_DIR"/data/*; do
    [ -f "$data_file" ] || continue
    data_name=$(basename "$data_file")
    run_step "install default data: $data_name" install -m 644 "$data_file" "$MOUNT_DIR/mnt/data/$data_name"
    need_file "$MOUNT_DIR/mnt/data/$data_name"
done
need_file "$MOUNT_DIR/mnt/data/sensor_cfg.ini"
run_step "create ssh_stop marker" touch "$MOUNT_DIR/etc/kvm/ssh_stop"
need_file "$MOUNT_DIR/etc/kvm/ssh_stop"

write_config fps 30
write_config height 1080
write_config now_fps 0
write_config qlty 60
write_config res 0
write_config state 1
write_config type mjpeg
write_config width 1920
info "default data and kvm config install check passed"

info "unmount image"
if umount "$MOUNT_DIR"; then
    :
else
    status=$?
    die "failed to unmount $MOUNT_DIR with exit code $status; make sure no terminal is inside the mount directory, then run: umount \"$MOUNT_DIR\""
fi
MOUNTED=0

if is_mounted; then
    die "$MOUNT_DIR is still mounted after umount"
fi

if ! cmp -n "$ROOTFS_OFFSET_BYTES" "$SOURCE_IMAGE_FILE" "$IMAGE_FILE" >/dev/null; then
    die "boot area changed unexpectedly; do not burn this output image: $IMAGE_FILE"
fi
info "boot area check passed: bytes before partition 2 are unchanged"

trap - EXIT HUP INT TERM

echo
echo "NanoKVM APP has been merged into:"
echo "$IMAGE_FILE"
