################################################################################
#
# maix-cdk
#
################################################################################

MAIX_CDK_VERSION = be3115270a2cb369a6a16b560cb088777add2a77
MAIX_CDK_SITE = $(call github,sipeed,MaixCDK,$(MAIX_CDK_VERSION))

MAIX_CDK_EXT_HOST_TOOLS = ../../../../host-tools
MAIX_CDK_EXT_MIDDLEWARE = ../../../../middleware
MAIX_CDK_EXT_OSDRV = ../../../../osdrv

MAIX_CDK_MIDDLEWARE = components/3rd_party/sophgo-middleware/sophgo-middleware

define MAIX_CDK_POST_EXTRACT_FIXUP
	mv $(@D)/$(MAIX_CDK_MIDDLEWARE)/v2 $(@D)/$(MAIX_CDK_MIDDLEWARE)/v2-cdk
	mkdir $(@D)/$(MAIX_CDK_MIDDLEWARE)/v2
	rsync -r --verbose --copy-dirlinks --copy-links --hard-links $(@D)/$(MAIX_CDK_EXT_MIDDLEWARE)/v2/ $(@D)/$(MAIX_CDK_MIDDLEWARE)/v2/
	mkdir $(@D)/$(MAIX_CDK_MIDDLEWARE)/v2/uapi
	rsync -r --verbose --copy-dirlinks --copy-links --hard-links $(@D)/$(MAIX_CDK_EXT_OSDRV)/interdrv/v2/include/common/uapi/ $(@D)/$(MAIX_CDK_MIDDLEWARE)/v2/uapi/
	rsync -r --verbose --copy-dirlinks --copy-links --hard-links $(@D)/$(MAIX_CDK_EXT_OSDRV)/interdrv/v2/include/chip/mars/uapi/ $(@D)/$(MAIX_CDK_MIDDLEWARE)/v2/uapi/
	rsync -r --verbose --copy-dirlinks --copy-links --hard-links $(@D)/$(MAIX_CDK_MIDDLEWARE)/v2-cdk/sample/vio/ $(@D)/$(MAIX_CDK_MIDDLEWARE)/v2/sample/vio/
	sed -i s/'^    url: .*'/'    url:'/g $(@D)/platforms/maixcam.yaml
	sed -i s/'^    sha256sum: .*'/'    sha256sum:'/g $(@D)/platforms/maixcam.yaml
	sed -i s/'^    filename: .*'/'    filename:'/g $(@D)/platforms/maixcam.yaml
	sed -i s/'^    path: .*'/'    path:'/g $(@D)/platforms/maixcam.yaml
	sed -i 's|^    bin_path: .*|    bin_path: '$(@D)/$(MAIX_CDK_EXT_HOST_TOOLS)'/gcc/riscv64-linux-musl-x86_64/bin|g' $(@D)/platforms/maixcam.yaml
endef
MAIX_CDK_POST_EXTRACT_HOOKS += MAIX_CDK_POST_EXTRACT_FIXUP

# todo: add real build

define MAIX_CDK_INSTALL_TARGET_CMDS
	mkdir -pv $(TARGET_DIR)/kvmapp/kvm_system/dl_lib/
	rsync -r --verbose --copy-dirlinks --copy-links --hard-links ${@D}/components/maixcam_lib/lib/libmaixcam_lib.so $(TARGET_DIR)/kvmapp/kvm_system/dl_lib/
	#rsync -r --verbose --copy-dirlinks --copy-links --hard-links $(MAIX_CDK_PKGDIR)/overlay/ $(TARGET_DIR)/
endef

$(eval $(generic-package))
