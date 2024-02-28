#!/usr/bin/env sh

set -eux

cd linux_5.10
# add aic8800 sdio wifi config into kernel
git am ../patches/linux_5.10/0001-drivers-net-wireless-add-config-aic8800.patch

# try /jump for init process
git am ../patches/linux_5.10/0002-init-try-jump-for-init-process.patch

# default dac/adc gain is too low, so we need set it high
git am ../patches/linux_5.10/0003-sound-soc-cvitek-cv181x-adc-dac-default-volume-is-to.patch
cd ..

cd osdrv
# add aic8800 sdio wifi driver
git am ../patches/osdrv/0001-extdrv-wireless-add-aic8800-sdio.patch
git am ../patches/osdrv/0002-extdrv-wireless-aic8800-fix-firmware-load-path.patch

# framebuffer default use double buffer
git am ../patches/osdrv/0003-interdrv-v2-fb-default-enable-double-buffer.patch

# fix pixel format on licheervnano
git am ../patches/osdrv/0004-interdrv-v2-fb-fix-pixel-format-for-licheervnao-add-.patch

# change touch size, for zct2133v1 800x1280
git am ../patches/osdrv/0005-extdrv-tp-ts_gt9xx-gt9xx.h-change-touch-size-for-zct.patch

git am ../patches/osdrv/0006-extdrv-add-rtl8733bs-driver.patch
cd ..

cd middleware
# install sample_vio sensor_test sample_audio into rootfs
git am ../patches/middleware/0001-v2-Makefile-install-vio-sensor_test-audio-demo-into-.patch

# disable some binary copy, because source file is missing
git am ../patches/middleware/0002-v2-Makefile-dont-copy-some-file-because-source-file-.patch

# add mipi panel st7701_hd228001c31
git am ../patches/middleware/0003-v2-component-panel-sg200x-add-dsi_st7701_hd228001c31.patch
git am ../patches/middleware/0004-v2-sample-mipi_tx-sample_dsi_panel.h-add-dsi_st7701_.patch

# add gc4653 support for licheervnano
git am ../patches/middleware/0005-v2-component-isp-sensor-sg200x-gcore_gc4653-add-supp.patch

# add zct2133v1 7inch 800x1280 mipi lcd support
git am ../patches/middleware/0006-v2-component-panel-sg200x-dsi_zct2133v1.h-add-zct213.patch

# add zct2133v1 into sample_dsi
git am ../patches/middleware/0007-v2-sample-mipi_tx-sample_dsi.c-add-zct2133v1-into-sa.patch

# fix image color (fix sample_vio 6)
git am ../patches/middleware/0008-v2-sample-common-sample_common_sensor.c-fix-sensor-p.patch

# fix sample_vio crash, thank lxowalle
git am ../patches/middleware/0009-v2-sample-vio-sample_vio.c-fix-crash-thank-lxowalle.patch

# fix mipi screen hd22801c31 initial
git am ../patches/middleware/0010-v2-component-panel-sg200x-dsi_st7701_hd228001c31.h-r.patch

# add alt hd22801c31 timing(not working)
git am ../patches/middleware/0011-v2-component-panel-sg200x-dsi_st7701_hd228001c31_alt.patch

# add mipi panel st7701_d300fpc9307a support
git am ../patches/middleware/0012-v2-sample-mipi_tx-sample_dsi-add-st7701_d300fpc9307a.patch

# fix CVI_U8 redefine
git am ../patches/middleware/0013-v2-component-panel-sg200x-fix-CVI_U8-redefine.patch

git am ../patches/middleware/0014-panel-add-st7701_dxq5d0019b480854.patch

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
git am ../patches/build/0009-boards-sg200x-sg2002_licheervnano_sd-fix-missing-wif.patch

# fix lcd reset in devicetree
git am ../patches/build/0010-boards-sg200x-sg2002_licheervnano_sd-fix-lcd-reset-i.patch

# enable lcd support in uboot
git am ../patches/build/0011-boards-sg200x-sg2002_licheervnano_sd-enable-lcd-supp.patch

# add dsi_st7701_hd228001c31 into Kconfig
git am ../patches/build/0012-panels-panel_list.json-add-dsi_st7701_hd228001c31.patch

# select default mipi panel for uboot
git am ../patches/build/0013-boards-sg200x-sg2002_licheervnano_sd-select-MIPI_PAN.patch

# default enable uboot lcd init
git am ../patches/build/0014-cvisetup.sh-enable-uboot-lcd-init-by-default.patch

# disable rndis script, enable acm script 
git am ../patches/build/0015-boards-sg200x-sg2002_licheervnano_sd-sg2002_licheerv.patch

# change framebuffer max size, for 32bpp mode
git am ../patches/build/0016-boards-sg200x-sg2002_licheervnano_sd-memmap.py-chang.patch

# add zct2133v1 into panel list
git am ../patches/build/0017-panels-panel_list.json-add-zct2133v1.patch

# licheervnano default use zct2133v1 panel (7inch)
git am ../patches/build/0018-boards-sg200x-sg2002_licheervnano_sd-sg2002_licheerv.patch

# licheervnano enable package lcd, initial lcd on bootup
git am ../patches/build/0019-boards-sg200x-sg2002_licheervnano_sd-sg2002_licheerv.patch

# licheervnano enable a lot of package, for debug usage
git am ../patches/build/0020-boards-sg200x-sg2002_licheervnano_sd-sg2002_licheerv.patch

# change partition max size, 16MiB is too small, change to 32MiB
git am ../patches/build/0021-tools-common-sd_tools-genimage_rootless.cfg-change-m.patch

# licheervnano enable tpu demo for tpu test
git am ../patches/build/0022-boards-sg200x-sg2002_licheervnano_sd-sg2002_licheerv.patch

# licheervnano linux kernel enable some feature for systemd based distro
git am ../patches/build/0023-boards-sg200x-sg2002_licheervnano_sd-linux-sg2002_li.patch

# add panel hd22801c31_alt0 into panel list
git am ../patches/build/0024-panels-panel_list.json-add-st7701_hd228001c31_alt0.patch

# change rootfs partition max size, 32MiB is too small, change to 40MiB
git am ../patches/build/0025-tools-common-sd_tools-genimage_rootless.cfg-resize-r.patch

# enable haveged unifont qt5 qt5demo
git am ../patches/build/0026-sg2002_licheervnano_sd_defconfig-enable-haveged-unif.patch

# add st7701_d300fpc9307a into panel list
git am ../patches/build/0027-panels-panel_list.json-add-st7701_d300fpc9307a.patch

# licheervnano: enable libdaemon expat dbus avahi
git am ../patches/build/0028-sg2002_licheervnano_defconfig-enable-libdaemon-expat.patch

git am ../patches/build/0029-genimage_rootless.cfg-fix-resize2fs-too-slow.patch
git am ../patches/build/0030-licheervnano_sd-clean_rootfs.sh-not-used.patch
git am ../patches/build/0031-sg2002_licheervnano_sd_defconfig-clean-buggy-package.patch
git am ../patches/build/0032-linux-sg2002_licheervnano_sd_defconfig-add-gpio-key.patch
git am ../patches/build/0033-sg2002_licheervnano_sd_defconfig-add-input-event-dae.patch
git am ../patches/build/0034-panels-add-st7701_dxq5d0019b480854-into-panel-list.patch
git am ../patches/build/0035-licheervnano_sd-enable-cstxxx-touch-screen-driver.patch


cd ..


cd u-boot-2021.10

# allow gpio not found in devicetree (some device haven't it)
git am ../patches/u-boot-2021.10/0001-drivers-video-cvitek-cvi_mipi.c-allow-gpio-not-found.patch

# disable cvi_jpeg build, because source file is missing
git am ../patches/u-boot-2021.10/0002-skip-cvitek_jpeg_dec-build-because-file-is-missing.patch

# add mipi panel st7701_hd22801c31
git am ../patches/u-boot-2021.10/0003-add-mipi-panel-st7701_hd22801c31.patch

# add mipi panel zct2133v1
git am ../patches/u-boot-2021.10/0004-include-cvitek-cvi_panels-dsi_zct2133v1.h-add-zct213.patch

# disable startvl on bootup, because it cause lcd show green background on blank
git am ../patches/u-boot-2021.10/0005-include-cvitek-cvi_panels-cvi_panel_diffs.h-disable-.patch

# add mipi panel st7701_hd228001c31_alt0
git am ../patches/u-boot-2021.10/0006-include-cvitek-cvi_panels-add-dsi_st7701_hd228001c31.patch

# add mipi panel st7701_d300fpc9307a
git am ../patches/u-boot-2021.10/0007-include-cvitek-cvi_panels-add-dsi_st7701_d300fpc9307.patch

git am ../patches/u-boot-2021.10/0008-panels-add-st7701_dxq5d0019b480854.patch

cd ..


exit 0
cd ramdisk
git am ../patches/ramdisk/0001-rootfs-public-add-a-lot-of-package-for-licheervnano.patch
git am ../patches/ramdisk/0002-src-add-qt5.patch
git am ../patches/ramdisk/0003-src-add-avahi-dbus-expat-libdaemon-doc.patch
git am ../patches/ramdisk/0004-overlay-add-sg2002_licheervnano_sd.patch
git am ../patches/ramdisk/0005-src-add-openssh-openssl.patch
git am ../patches/ramdisk/0006-common_musl_riscv64-busybox-upgrade.patch
git am ../patches/ramdisk/0007-common_musl_riscv64-S40network-allow-load-config-fro.patch
git am ../patches/ramdisk/0008-20240228.patch
cd ..


