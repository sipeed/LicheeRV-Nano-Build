#!/bin/bash
# a sd image generator for sophpi
# usage
if [ "$#" -ne "1" ]
then
	echo "usage: sudo ./sd_gen_burn_image.sh OUTPUT_DIR"
	echo ""
	echo "       The script is used to create a sdcard image with two partitions, "
	echo "       one is fat32 with 128MB, the other is ext4 with 256MB."
	echo "       You can modify the capacities in this script as you wish!"
	echo ""
	echo "Note:  Please backup you sdcard files before using this image!"

	exit
fi

vfat_cap=128M
vfat_label="boot"
ext4_cap=256M
ext4_label="rootfs"

output_dir=$1
# gen a empty image
image=sophpi-duo-`date +%Y%m%d-%H%M`.img

echo ${output_dir}
# rootless image generate
set -eu
THISDIR=$(dirname $(realpath $0))
mkdir -pv ${output_dir}/tmp/
mkdir -pv ${output_dir}/root/
mkdir -pv ${output_dir}/input/
mkdir -pv ${output_dir}/input/rawimages/
cp -fv ${output_dir}/fip.bin ${output_dir}/input/
cp -fv ${output_dir}/rawimages/boot.sd ${output_dir}/input/rawimages/
cp -fv ${output_dir}/rawimages/rootfs_ext4.sd ${output_dir}/input/
cp -fv ${THISDIR}/genimage_buildroot.cfg ${output_dir}/genimage.cfg
sed -i -e "s/sophpi-duo.img/${image}/g" ${output_dir}/genimage.cfg
cd ${output_dir}/
${THISDIR}/genimage
exit $?


# not used

pushd ${output_dir}
echo ${image}
dd if=/dev/zero of=./${image} bs=1M count=512

################################
# Note: do not change this flow
################################
sudo fdisk ./${image} << EOF
n
p
1

+${vfat_cap}
n
p
2

+${ext4_cap}
w
EOF
# Note end
################################

dev_name=`sudo losetup -f`
echo ${dev_name}
echo ""

sudo losetup ${dev_name} ./${image}
sudo partprobe ${dev_name}

sudo mkfs.vfat -F 32 -n ${vfat_label} ${dev_name}p1
sudo mkfs.ext4 -L ${ext4_label} ${dev_name}p2

# mount partitions
rm ./tmp1 ./tmp2 -rf
mkdir tmp1 tmp2
sudo mount -t vfat ${dev_name}p1 tmp1/
sudo mount -t ext4 ${dev_name}p2 tmp2/

# copy boot file and rootfs
sudo cp ${output_dir}/fip.bin ./tmp1/
sudo cp ${output_dir}/rawimages/boot.sd ./tmp1/
sudo cp -raf ${output_dir}/rootfs/* ./tmp2

sync

# umount
sudo umount tmp1 tmp2
sudo losetup -d ${dev_name}
rmdir tmp1 tmp2

# tar image
tar zcvf ${image}.tar.gz ${image}

echo "Gen image successful: ${image}"
echo ""

popd
