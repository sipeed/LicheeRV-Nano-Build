#!/bin/sh

set -eu

target_dir=$1

# Production NanoKVM uses its own server and multimedia libraries. SDK sample
# programs and development metadata are not needed at runtime.
find "$target_dir/mnt/system/usr/bin" -type f \
	\( -name 'sample_*' -o -name '*_test' -o -name 'test_*' -o -name 'ive_stress' -o -name 'sensor_test' \) \
	-delete 2>/dev/null || true

touch "$target_dir/etc/nanokvm-minimal"

# The minimal board uses AIC8800; do not ship or load the unused 8733bs driver.
rm -f "$target_dir/mnt/system/ko/3rd/8733bs.ko"
if [ -f "$target_dir/etc/init.d/S25wifimod" ]; then
	sed -i '/insmod 3rd\/8733bs\.ko/d' "$target_dir/etc/init.d/S25wifimod"
fi

# Do not retain stale hardware database files in an incremental build.
rm -rf "$target_dir/etc/udev/hwdb.bin" "$target_dir/etc/udev/hwdb.d"

# Keep only the sensor selected by the minimal board configuration.
rm -f \
	"$target_dir/mnt/system/usr/lib/libsns_gc4653.so" \
	"$target_dir/mnt/system/usr/lib/libov_ov2685.so" \
	"$target_dir/mnt/system/usr/lib/libsns_os04a10.so" \
	"$target_dir/mnt/system/usr/lib/libsns_sc035gs.so"

# LT6911 has no matching SDK ISP tuning blob; remove stale blobs from reuse builds.
find "$target_dir/mnt/cfg/param" "$target_dir/mnt/cfg/tmp_secure" \
	-type f -delete 2>/dev/null || true

find "$target_dir" -type f \( -name '*.a' -o -name '*.la' \) -delete
rm -rf \
	"$target_dir/usr/include" \
	"$target_dir/usr/share/doc" \
	"$target_dir/usr/share/info" \
	"$target_dir/usr/share/man" \
	"$target_dir/usr/share/pkgconfig" \
	"$target_dir/usr/lib/pkgconfig" \
	"$target_dir/usr/lib/cmake"
