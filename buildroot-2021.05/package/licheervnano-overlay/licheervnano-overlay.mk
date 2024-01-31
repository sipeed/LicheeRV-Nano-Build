################################################################################
#
# licheervnano-overlay
#
################################################################################

LICHEERVNANO_OVERLAY_VERSION = 2024-01-24
LICHEERVNANO_OVERLAY_SITE = "$(TOPDIR)/package/licheervnano-overlay/files/"
LICHEERVNANO_OVERLAY_SITE_METHOD = local

define LICHEERVNANO_OVERLAY_INSTALL_TARGET_CMDS
	mkdir -p $(TARGET_DIR)/
	cp -rvf $(@D)/* $(TARGET_DIR)/
endef

$(eval $(generic-package))
