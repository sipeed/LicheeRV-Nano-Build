################################################################################
#
# sg2002-codec-firmware
#
################################################################################

SG2002_CODEC_FIRMWARE_VERSION = 1e339b782642ce1b2c8aa81f9fdef212912a6a83
SG2002_CODEC_FIRMWARE_SITE = $(call github,0x754C,sg2002_codec_fw,$(SG2002_CODEC_FIRMWARE_VERSION))

define SG2002_CODEC_FIRMWARE_INSTALL_TARGET_CMDS
	mkdir -pv $(TARGET_DIR)/usr/share/fw_vcodec/
	rsync -r --verbose --copy-dirlinks --copy-links --hard-links ${@D}/* $(TARGET_DIR)/usr/share/fw_vcodec/
endef

$(eval $(generic-package))
