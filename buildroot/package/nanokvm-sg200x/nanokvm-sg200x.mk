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

NANOKVM_SG200X_EXT_MIDDLEWARE = ../../../../middleware
NANOKVM_SG200X_EXT_MAIXCAM_LIB = v2/sample/test_mmf/maixcam_lib/release.linux/libmaixcam_lib.so
NANOKVM_SG200X_EXT_OVERLAY = $(BR2_ROOTFS_OVERLAY)

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
	echo -n 720 > $(TARGET_DIR)/kvmapp/kvm/res
	echo 30 > $(TARGET_DIR)/kvmapp/kvm/fps
	if [ -e ${@D}/$(NANOKVM_SG200X_EXT_MIDDLEWARE)/$(NANOKVM_SG200X_EXT_MAIXCAM_LIB) ]; then \
		rsync -r --verbose --copy-dirlinks --copy-links --hard-links ${@D}/$(NANOKVM_SG200X_EXT_MIDDLEWARE)/$(NANOKVM_SG200X_EXT_MAIXCAM_LIB) $(TARGET_DIR)/kvmapp/kvm_system/dl_lib/ ; \
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
