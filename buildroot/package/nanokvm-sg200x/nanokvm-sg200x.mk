################################################################################
#
# nanokvm-sg200x
#
################################################################################

NANOKVM_SG200X_VERSION = latest
NANOKVM_SG200X_BASE = $(NANOKVM_SG200X_VERSION)
NANOKVM_SG200X_SOURCE = $(NANOKVM_SG200X_BASE).zip
NANOKVM_SG200X_SITE = https://cdn.sipeed.com/nanokvm

NANOKVM_SG200X_DEPENDENCIES += nanokvm-server

NANOKVM_SG200X_EXT_MIDDLEWARE = ../../../../middleware/v2
NANOKVM_SG200X_EXT_KVM_SYSTEM = sample/kvm_system/kvm_system
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

NANOKVM_SG200X_UNUSED_LIBS = \
	libcli.so \
	libjson-c.so.5 \
	libcvi_ispd2.so \
	libaaccomm2.so \
	libaacdec2.so \
	libaacenc2.so \
	libaacsbrdec2.so \
	libaacsbrenc2.so \
	libcvi_RES1.so \
	libcvi_VoiceEngine.so \
	libcvi_audio.so \
	libcvi_ssp.so \
	libcvi_vqe.so \
	libdnvqe.so \
	libtinyalsa.so

define NANOKVM_SG200X_EXTRACT_CMDS
	$(UNZIP) -d $(@D) \
		$(NANOKVM_SG200X_DL_DIR)/$(NANOKVM_SG200X_SOURCE)
	mv $(@D)/$(NANOKVM_SG200X_BASE) $(@D)/kvmapp
	rm -f ${@D}/kvmapp/server/NanoKVM-Server
	rm -rf ${@D}//kvmapp/server/web/
endef

define NANOKVM_SG200X_INSTALL_TARGET_CMDS
	rsync -r --verbose --copy-dirlinks --copy-links --hard-links $(NANOKVM_SG200X_PKGDIR)/overlay/ $(TARGET_DIR)/
	mkdir -pv $(TARGET_DIR)/kvmapp/
	rsync -r --verbose --copy-dirlinks --copy-links --hard-links ${@D}/kvmapp/ $(TARGET_DIR)/kvmapp/
	echo 0 > $(TARGET_DIR)/kvmapp/kvm/now_fps
	echo 30 > $(TARGET_DIR)/kvmapp/kvm/fps
	echo -n 80 > $(TARGET_DIR)/kvmapp/kvm/qlty
	echo -n 720 > $(TARGET_DIR)/kvmapp/kvm/res
	echo mjpeg > $(TARGET_DIR)/kvmapp/kvm/type
	echo 0 > $(TARGET_DIR)/kvmapp/kvm/state
	if [ -e ${@D}/$(NANOKVM_SG200X_EXT_MIDDLEWARE)/$(NANOKVM_SG200X_EXT_KVM_SYSTEM) ]; then \
		rsync -r --verbose --copy-dirlinks --copy-links --hard-links ${@D}/$(NANOKVM_SG200X_EXT_MIDDLEWARE)/$(NANOKVM_SG200X_EXT_KVM_SYSTEM) $(TARGET_DIR)/kvmapp/kvm_system/ ; \
	fi
	if [ -e ${@D}/$(NANOKVM_SG200X_EXT_MIDDLEWARE)/$(NANOKVM_SG200X_EXT_MAIXCAM_LIB) ]; then \
		rsync -r --verbose --copy-dirlinks --copy-links --hard-links ${@D}/$(NANOKVM_SG200X_EXT_MIDDLEWARE)/$(NANOKVM_SG200X_EXT_MAIXCAM_LIB) $(TARGET_DIR)/kvmapp/kvm_system/dl_lib/ ; \
	fi
	for l in $(NANOKVM_SG200X_REQUIRED_LIBS) ; do \
		rsync -r --verbose --copy-dirlinks --copy-links --hard-links ${@D}/$(NANOKVM_SG200X_EXT_MIDDLEWARE)/lib/$$l $(TARGET_DIR)/kvmapp/kvm_system/dl_lib/ ; \
	done
	for l in $(NANOKVM_SG200X_VPU_LIBS) ; do \
		if [ -e ${@D}/$(NANOKVM_SG200X_EXT_MIDDLEWARE)/lib/$$l ]; then \
			rsync -r --verbose --copy-dirlinks --copy-links --hard-links ${@D}/$(NANOKVM_SG200X_EXT_MIDDLEWARE)/lib/$$l $(TARGET_DIR)/kvmapp/kvm_system/dl_lib/ ; \
		fi ; \
	done
	for l in $(NANOKVM_SG200X_UNUSED_LIBS) ; do \
		rm -f $(TARGET_DIR)/kvmapp/kvm_system/dl_lib/$$l ; \
		ln -s libmisc.so $(TARGET_DIR)/kvmapp/kvm_system/dl_lib/$$l ; \
	done
	if [ -e $(TARGET_DIR)/kvmapp/kvm_system/dl_lib/libmaixcam_lib.so -a \
	     -e $(TARGET_DIR)/kvmapp/kvm_system/kvm_stream -a \
	   ! -e $(TARGET_DIR)/kvmapp/kvm_stream ]; then \
		rm -rf $(TARGET_DIR)/kvmapp/jpg_stream/ ; \
		mkdir -pv $(TARGET_DIR)/kvmapp/kvm_stream/ ; \
		mv $(TARGET_DIR)/kvmapp/kvm_system/kvm_stream  $(TARGET_DIR)/kvmapp/kvm_stream/ ; \
		mkdir -pv $(TARGET_DIR)/kvmapp/kvm_stream/dl_lib/ ; \
		rsync -avpPxH $(TARGET_DIR)/kvmapp/kvm_system/dl_lib/ $(TARGET_DIR)/kvmapp/kvm_stream/dl_lib/ ; \
	fi
	rm -f $(TARGET_DIR)/kvmapp/system/ko/*.ko
	if [ "X$(BR2_PACKAGE_TAILSCALE_RISCV64)" != "Xy" ]; then \
		rm -f $(TARGET_DIR)/kvmapp/system/init.d/S??tailscaled ; \
	fi
	mkdir -pv $(TARGET_DIR)/etc/init.d/
	rsync -r --verbose --copy-dirlinks --copy-links --hard-links $(TARGET_DIR)/kvmapp/system/init.d/ $(TARGET_DIR)/etc/init.d/
	rm -f $(TARGET_DIR)/kvmapp/version
	if [ -e $(NANOKVM_SG200X_EXT_OVERLAY)/etc/init.d ]; then \
		rsync -r --verbose --copy-dirlinks --copy-links --hard-links $(TARGET_DIR)/kvmapp/system/init.d/ $(NANOKVM_SG200X_EXT_OVERLAY)/etc/init.d/ ; \
	fi
endef

$(eval $(generic-package))
