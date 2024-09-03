################################################################################
#
# nanokvm-sg200x
#
################################################################################

NANOKVM_SG200X_VERSION = latest
NANOKVM_SG200X_BASE = $(NANOKVM_SG200X_VERSION)
NANOKVM_SG200X_SOURCE = $(NANOKVM_SG200X_BASE).zip
NANOKVM_SG200X_SITE = https://cdn.sipeed.com/nanokvm

NANOKVM_SG200X_DEPENDENCIES += maix-cdk

define NANOKVM_SG200X_EXTRACT_CMDS
	$(UNZIP) -d $(@D) \
		$(NANOKVM_SG200X_DL_DIR)/$(NANOKVM_SG200X_SOURCE)
	mv $(@D)/$(NANOKVM_SG200X_BASE) $(@D)/kvmapp
endef

define NANOKVM_SG200X_INSTALL_TARGET_CMDS
	rsync -r --verbose --copy-dirlinks --copy-links --hard-links $(NANOKVM_SG200X_PKGDIR)/overlay/ $(TARGET_DIR)/
	mkdir -pv $(TARGET_DIR)/kvmapp/
	rsync -r --verbose --copy-dirlinks --copy-links --hard-links ${@D}/kvmapp/ $(TARGET_DIR)/kvmapp/
	echo -n 720 > $(TARGET_DIR)/kvmapp/kvm/res
	echo 30 > $(TARGET_DIR)/kvmapp/kvm/fps
	rm -f $(TARGET_DIR)/kvmapp/system/init.d/S01fs
	rm -f $(TARGET_DIR)/kvmapp/system/init.d/S03usbdev
	rm -f $(TARGET_DIR)/kvmapp/system/init.d/S30eth
	rm -f $(TARGET_DIR)/kvmapp/system/init.d/S99resizefs
	rm -f $(TARGET_DIR)/kvmapp/version
endef

$(eval $(generic-package))
