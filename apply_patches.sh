#!/usr/bin/env sh

set -eux

cd linux_5.10
# add aic8800 sdio wifi config into kernel
git am ../patches/linux_5.10/0001-drivers-net-wireless-add-config-aic8800.patch

# try /jump for init process
git am ../patches/linux_5.10/0002-init-try-jump-for-init-process.patch
cd ..

cd osdrv
# add aic8800 sdio wifi driver
git am ../patches/osdrv/0001-extdrv-wireless-add-aic8800-sdio.patch
git am ../patches/osdrv/0002-extdrv-wireless-aic8800-fix-firmware-load-path.patch
cd ..

cd ramdisk
# add aic8800 sdio wifi firmware
git am ../patches/ramdisk/0001-rootfs-public-wifi-musl_riscv64-add-aic8800-sdio-wif.patch

# add script for wifi module load on bootup
git am ../patches/ramdisk/0002-rootfs-public-wifi-musl_riscv64-etc-add-script-for-w.patch

# add S99skel and sensor_cfg.ini (default camera config file)
git am ../patches/ramdisk/0003-rootfs-overlay-sg2002_licheervnano_sd-add-S99skel-se.patch

# add jump script for alt rootfs
git am ../patches/ramdisk/0004-rootfs-overlay-sg2002_licheervnano_sd-add-jump-scrip.patch

cd ..

cd middleware
# install sample_vio sensor_test sample_audio into rootfs
git am ../patches/middleware/0001-v2-Makefile-install-vio-sensor_test-audio-demo-into-.patch

# disable some binary copy, because source file is missing
git am ../patches/middleware/0002-v2-Makefile-dont-copy-some-file-because-source-file-.patch

# add mipi panel st7701_hd228001c31
../patches/middleware/0003-v2-component-panel-sg200x-add-dsi_st7701_hd228001c31.patch
../patches/middleware/0004-v2-sample-mipi_tx-sample_dsi_panel.h-add-dsi_st7701_.patch

# add gc4653 support for licheervnano
git am ../patches/middleware/0005-v2-component-isp-sensor-sg200x-gcore_gc4653-add-supp.patch

cd ..


cd build
# allow vo & cvifb node ref by other dts file
git am ../patches/build/0001-boards-default-dts-sg200x-soph_base.dtsi-allow-vo-an.patch

# add rootless sdcard image generate script
git am ../patches/build/0002-tools-common-sd_tools-add-sd_gen_burn_image_rootless.patch

# add licheervnano basic support
git am ../patches/build/0003-boards-sg200x-add-sg2002_licheervnano_sd.patch

# enable aic8800 sdio wifi on licheervnano
git am ../patches/build/0004-boards-sg200-sg2002_licheervnano_sd-enable-aic8800-s.patch

# add user partition for image generate
git am ../patches/build/0005-tools-common-sd_tools-genimage_rootless.cfg-add-user.patch

# enable rndis script on boot for licheervnano
git am ../patches/build/0006-boards-sg200x-sg2002_licheervnano_sd-enable-rndis-sc.patch

# don't clean middleware demo
git am ../patches/build/0007-boards-sg200x-sg2002_licheervnano_sd-dont-clean-midd.patch

# fix out of tree wireless driver install config depend
git am ../patches/build/0008-Kconfig-why-wireless-driver-install-depend-FLASH_SIZ.patch

# enable out of tree wireless driver install config
git am ../patches/build/0009-boards-sg200x-sg2002_licheervnano_sd-fix-missing-wif.patc

# fix lcd reset in devicetree
git am ../patches/build/0010-boards-sg200x-sg2002_licheervnano_sd-fix-lcd-reset-i.patch

# enable lcd support in uboot
git am ../patches/build/0011-boards-sg200x-sg2002_licheervnano_sd-enable-lcd-supp.patch

# add dsi_st7701_hd228001c31 into Kconfig
git am ../patches/build/0012-panels-panel_list.json-add-dsi_st7701_hd228001c31.patch

# select default mipi panel for uboot
git am ../patches/build/0013-boards-sg200x-sg2002_licheervnano_sd-select-MIPI_PAN.patch

cd ..


cd u-boot-2021.10

# allow gpio not found in devicetree (some device haven't it)
git am ../patches/u-boot-2021.10/0001-drivers-video-cvitek-cvi_mipi.c-allow-gpio-not-found.patch

# disable cvi_jpeg build, because source file is missing
git am ../patches/u-boot-2021.10/0002-skip-cvitek_jpeg_dec-build-because-file-is-missing.patch

# add mipi panel st7701_hd22801c31
git am ../patches/u-boot-2021.10/0003-add-mipi-panel-st7701_hd22801c31.patch

cd ..
