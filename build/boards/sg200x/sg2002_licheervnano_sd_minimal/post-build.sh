#!/bin/sh

set -eu

target_dir=$1

# Production NanoKVM uses its own server and multimedia libraries. SDK sample
# programs and development metadata are not needed at runtime.
find "$target_dir/mnt/system/usr/bin" -type f \
	\( -name 'sample_*' -o -name '*_test' -o -name 'test_*' -o -name 'ive_stress' -o -name 'sensor_test' \) \
	-delete 2>/dev/null || true

touch "$target_dir/etc/nanokvm-minimal"

find "$target_dir" -type f \( -name '*.a' -o -name '*.la' \) -delete
rm -rf \
	"$target_dir/usr/include" \
	"$target_dir/usr/share/doc" \
	"$target_dir/usr/share/info" \
	"$target_dir/usr/share/man" \
	"$target_dir/usr/share/pkgconfig" \
	"$target_dir/usr/lib/pkgconfig" \
	"$target_dir/usr/lib/cmake"
