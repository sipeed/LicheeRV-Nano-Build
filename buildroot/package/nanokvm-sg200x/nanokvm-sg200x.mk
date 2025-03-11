################################################################################
#
# nanokvm-sg200x
#
################################################################################

NANOKVM_SG200X_VERSION = 2.2.2
NANOKVM_SG200X_SUBLEVEL =
NANOKVM_SG200X_BASE = nanokvm-skeleton-$(NANOKVM_SG200X_VERSION)$(NANOKVM_SG200X_SUBLEVEL)
NANOKVM_SG200X_SOURCE = v$(NANOKVM_SG200X_VERSION)$(NANOKVM_SG200X_SUBLEVEL).zip
NANOKVM_SG200X_SITE = https://github.com/scpcom/nanokvm-skeleton/archive/refs/tags

NANOKVM_SG200X_DEPENDENCIES += nanokvm-server

NANOKVM_SG200X_EXT_MIDDLEWARE = $(realpath $(TOPDIR)/../middleware/v2)
NANOKVM_SG200X_EXT_KVM_SYSTEM = sample/kvm_system/kvm_system
NANOKVM_SG200X_EXT_KVM_STREAM = sample/kvm_stream/kvm_stream
NANOKVM_SG200X_EXT_KVM_MMF = sample/test_mmf/kvm_mmf/release.linux/libkvm_mmf.so
NANOKVM_SG200X_EXT_KVM_VISION = sample/test_mmf/kvm_vision/release.linux/libkvm.so
NANOKVM_SG200X_EXT_MAIXCAM_LIB = sample/test_mmf/maixcam_lib/release.linux/libmaixcam_lib.so
NANOKVM_SG200X_EXT_OVERLAY = $(BR2_ROOTFS_OVERLAY)

NANOKVM_SG200X_REQUIRED_LIBS = \
	libae.so \
	libaf.so \
	libawb.so \
	libcvi_bin_isp.so \
	libcvi_bin.so \
	libcvi_ive.so \
	libini.so \
	libisp_algo.so \
	libisp.so \
	libmipi_tx.so \
	libmisc.so \
	libraw_dump.so \
	libsys.so \
	libvdec.so \
	libvenc.so

NANOKVM_SG200X_VPU_LIBS = \
	libosdc.so \
	libvpu.so \
	libgdc.so \
	librgn.so \
	libvi.so \
	libvo.so \
	libvpss.so

NANOKVM_SG200X_DUMMY_LIBS = \
	libae.so \
	libaf.so \
	libawb.so

define NANOKVM_SG200X_EXTRACT_CMDS
	$(UNZIP) -d $(@D) \
		$(NANOKVM_SG200X_DL_DIR)/$(NANOKVM_SG200X_SOURCE)
	mv $(@D)/$(NANOKVM_SG200X_BASE) $(@D)/kvmapp
	rm -f ${@D}/kvmapp/server/NanoKVM-Server
	rm -rf ${@D}//kvmapp/server/web/
endef

define NANOKVM_SG200X_INSTALL_TARGET_CMDS
	rsync -r --verbose --copy-dirlinks --copy-links --hard-links $(NANOKVM_SG200X_PKGDIR)/overlay/ $(TARGET_DIR)/
	if [ -e $(TARGET_DIR)/kvmapp/kvm_system/kvm_system ]; then \
		rm -f ${@D}/kvmapp/kvm_system/kvm_system ; \
		touch ${@D}/.maixcdk_kvm_system ; \
	else \
		rm -f ${@D}/.maixcdk_kvm_system ; \
	fi
	mkdir -pv $(TARGET_DIR)/kvmapp/
	rsync -r --verbose --copy-dirlinks --copy-links --hard-links ${@D}/kvmapp/ $(TARGET_DIR)/kvmapp/
	echo 0 > $(TARGET_DIR)/kvmapp/kvm/now_fps
	echo 30 > $(TARGET_DIR)/kvmapp/kvm/fps
	echo -n 80 > $(TARGET_DIR)/kvmapp/kvm/qlty
	echo -n 720 > $(TARGET_DIR)/kvmapp/kvm/res
	echo mjpeg > $(TARGET_DIR)/kvmapp/kvm/type
	echo 0 > $(TARGET_DIR)/kvmapp/kvm/state
	mkdir -pv $(TARGET_DIR)/kvmapp/kvm_system/
	if [ -e $(NANOKVM_SG200X_EXT_MIDDLEWARE)/$(NANOKVM_SG200X_EXT_KVM_SYSTEM) -a ! -e ${@D}/.maixcdk_kvm_system ]; then \
		rsync -r --verbose --copy-dirlinks --copy-links --hard-links $(NANOKVM_SG200X_EXT_MIDDLEWARE)/$(NANOKVM_SG200X_EXT_KVM_SYSTEM) $(TARGET_DIR)/kvmapp/kvm_system/ ; \
	fi
	mkdir -pv $(TARGET_DIR)/kvmapp/kvm_system/dl_lib/
	if [ -e $(TARGET_DIR)/kvmapp/server/dl_lib/libkvm_mmf.so -a -e ${@D}/.maixcdk_kvm_system ]; then \
		touch ${@D}/.maixcdk_kvm_mmf ; \
	elif [ -e $(NANOKVM_SG200X_EXT_MIDDLEWARE)/$(NANOKVM_SG200X_EXT_KVM_MMF) ]; then \
		rm -f ${@D}/.maixcdk_kvm_mmf ; \
		rsync -r --verbose --copy-dirlinks --copy-links --hard-links $(NANOKVM_SG200X_EXT_MIDDLEWARE)/$(NANOKVM_SG200X_EXT_KVM_MMF) $(TARGET_DIR)/kvmapp/kvm_system/dl_lib/ ; \
	elif [ -e $(NANOKVM_SG200X_EXT_MIDDLEWARE)/$(NANOKVM_SG200X_EXT_MAIXCAM_LIB) ]; then \
		rm -f ${@D}/.maixcdk_kvm_mmf ; \
		rsync -r --verbose --copy-dirlinks --copy-links --hard-links $(NANOKVM_SG200X_EXT_MIDDLEWARE)/$(NANOKVM_SG200X_EXT_MAIXCAM_LIB) $(TARGET_DIR)/kvmapp/kvm_system/dl_lib/ ; \
	fi
	for l in $(NANOKVM_SG200X_REQUIRED_LIBS) ; do \
		rsync -r --verbose --copy-dirlinks --copy-links --hard-links $(NANOKVM_SG200X_EXT_MIDDLEWARE)/lib/$$l $(TARGET_DIR)/kvmapp/kvm_system/dl_lib/ ; \
	done
	for l in $(NANOKVM_SG200X_VPU_LIBS) ; do \
		if [ -e $(NANOKVM_SG200X_EXT_MIDDLEWARE)/lib/$$l ]; then \
			rsync -r --verbose --copy-dirlinks --copy-links --hard-links $(NANOKVM_SG200X_EXT_MIDDLEWARE)/lib/$$l $(TARGET_DIR)/kvmapp/kvm_system/dl_lib/ ; \
		fi ; \
	done
	if [ -e $(NANOKVM_SG200X_EXT_MIDDLEWARE)/lib/libcvi_dummy.so ]; then \
		rsync -r --verbose --copy-dirlinks --copy-links --hard-links $(NANOKVM_SG200X_EXT_MIDDLEWARE)/lib/libcvi_dummy.so $(TARGET_DIR)/kvmapp/kvm_system/dl_lib/ ; \
		for l in $(NANOKVM_SG200X_DUMMY_LIBS) ; do \
			rm -f $(TARGET_DIR)/kvmapp/kvm_system/dl_lib/$$l ; \
			ln -s libcvi_dummy.so $(TARGET_DIR)/kvmapp/kvm_system/dl_lib/$$l ; \
		done ; \
	fi
	if [ -e $(NANOKVM_SG200X_EXT_MIDDLEWARE)/lib/libcvi_bin_light.so -a \
	     -e $(NANOKVM_SG200X_EXT_MIDDLEWARE)/lib/libisp_light.so ]; then \
		rsync -r --verbose --copy-dirlinks --copy-links --hard-links $(NANOKVM_SG200X_EXT_MIDDLEWARE)/lib/libcvi_bin_light.so $(TARGET_DIR)/kvmapp/kvm_system/dl_lib/ ; \
		for l in libcvi_bin.so ; do \
			rm -f $(TARGET_DIR)/kvmapp/kvm_system/dl_lib/$$l ; \
			ln -s libcvi_bin_light.so $(TARGET_DIR)/kvmapp/kvm_system/dl_lib/$$l ; \
		done ; \
		rsync -r --verbose --copy-dirlinks --copy-links --hard-links $(NANOKVM_SG200X_EXT_MIDDLEWARE)/lib/libisp_light.so $(TARGET_DIR)/kvmapp/kvm_system/dl_lib/ ; \
		for l in libcvi_bin_isp.so libisp.so ; do \
			rm -f $(TARGET_DIR)/kvmapp/kvm_system/dl_lib/$$l ; \
			ln -s libisp_light.so $(TARGET_DIR)/kvmapp/kvm_system/dl_lib/$$l ; \
		done ; \
		for l in libisp_algo.so ; do \
			rm -f $(TARGET_DIR)/kvmapp/kvm_system/dl_lib/$$l ; \
			ln -s libmisc.so $(TARGET_DIR)/kvmapp/kvm_system/dl_lib/$$l ; \
		done ; \
	fi
	if [ -e $(NANOKVM_SG200X_EXT_MIDDLEWARE)/$(NANOKVM_SG200X_EXT_KVM_STREAM) ]; then \
		rsync -r --verbose --copy-dirlinks --copy-links --hard-links $(NANOKVM_SG200X_EXT_MIDDLEWARE)/$(NANOKVM_SG200X_EXT_KVM_STREAM) $(TARGET_DIR)/kvmapp/kvm_system/ ; \
	fi
	if [ -e $(TARGET_DIR)/kvmapp/kvm_system/dl_lib/libmaixcam_lib.so -a \
	     -e $(TARGET_DIR)/kvmapp/kvm_system/kvm_stream -a \
	   ! -e $(TARGET_DIR)/kvmapp/kvm_stream ]; then \
		rm -rf $(TARGET_DIR)/kvmapp/jpg_stream/ ; \
		mkdir -pv $(TARGET_DIR)/kvmapp/kvm_stream/ ; \
		mv $(TARGET_DIR)/kvmapp/kvm_system/kvm_stream  $(TARGET_DIR)/kvmapp/kvm_stream/ ; \
		mkdir -pv $(TARGET_DIR)/kvmapp/kvm_stream/dl_lib/ ; \
		rsync -avpPxH $(TARGET_DIR)/kvmapp/kvm_system/dl_lib/ $(TARGET_DIR)/kvmapp/kvm_stream/dl_lib/ ; \
	fi
	if [ -e $(NANOKVM_SG200X_EXT_MIDDLEWARE)/$(NANOKVM_SG200X_EXT_KVM_VISION) -a \
	     -e $(TARGET_DIR)/kvmapp/server/dl_lib/libkvm.so -a \
	   ! -e ${@D}/.maixcdk_kvm_mmf ]; then \
		rsync -r --verbose --copy-dirlinks --copy-links --hard-links $(NANOKVM_SG200X_EXT_MIDDLEWARE)/$(NANOKVM_SG200X_EXT_KVM_VISION) $(TARGET_DIR)/kvmapp/server/dl_lib/ ; \
		chmod ugo+rx $(TARGET_DIR)/kvmapp/server/dl_lib/libkvm.so ; \
	fi
	if [ -e $(TARGET_DIR)/kvmapp/server/dl_lib/libkvm.so ]; then \
		rm -rf $(TARGET_DIR)/kvmapp/kvm_stream/ ; \
		rm -rf $(TARGET_DIR)/kvmapp/kvm_system/dl_lib/ ; \
		mkdir -pv $(TARGET_DIR)/kvmapp/kvm_system/dl_lib/ ; \
	fi
	if [ ! -e ${@D}/.maixcdk_kvm_system ]; then \
		sed -i 's|# cp -r /kvmapp/server|cp -r /kvmapp/server|g' $(TARGET_DIR)/kvmapp/system/init.d/S95nanokvm ; \
		sed -i 's|# /tmp/server/NanoKVM-Server|/tmp/server/NanoKVM-Server|g' $(TARGET_DIR)/kvmapp/system/init.d/S95nanokvm ; \
	fi
	rm -f $(TARGET_DIR)/kvmapp/system/ko/*.ko
	if [ "X$(BR2_PACKAGE_TAILSCALE_RISCV64)" != "Xy" ]; then \
		rm -f $(TARGET_DIR)/kvmapp/system/init.d/S??tailscaled ; \
	fi
	mkdir -pv $(TARGET_DIR)/etc/init.d/
	rsync -r --verbose --copy-dirlinks --copy-links --hard-links $(TARGET_DIR)/kvmapp/system/init.d/ $(TARGET_DIR)/etc/init.d/
	if [ -e $(NANOKVM_SG200X_EXT_OVERLAY)/etc/init.d ]; then \
		rsync -r --verbose --copy-dirlinks --copy-links --hard-links $(TARGET_DIR)/kvmapp/system/init.d/ $(NANOKVM_SG200X_EXT_OVERLAY)/etc/init.d/ ; \
	fi
endef

$(eval $(generic-package))
