################################################################################
#
# aic8800-fw
#
################################################################################

AIC8800_FW_VERSION = 2024-01-24
AIC8800_FW_SITE = "$(TOPDIR)/package/aic8800-fw/blobs/"
AIC8800_FW_SITE_METHOD = local

define AIC8800_FW_INSTALL_TARGET_CMDS
	mkdir -p $(TARGET_DIR)/lib/firmware/
	cp -rvf $(@D)/* $(TARGET_DIR)/lib/firmware/
endef

$(eval $(generic-package))
