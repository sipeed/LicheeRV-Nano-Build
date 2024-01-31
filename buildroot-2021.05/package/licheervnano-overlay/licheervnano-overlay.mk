################################################################################
#
# licheervnano-overlay
#
################################################################################

LICHEERVNANO_OVERLAY_VERSION = 2024-01-31
LICHEERVNANO_OVERLAY_SITE = "$(TOPDIR)/package/licheervnano-overlay/files/"
LICHEERVNANO_OVERLAY_SITE_METHOD = local

define LICHEERVNANO_OVERLAY_INSTALL_TARGET_CMDS
	cp -arvf $(@D)/* $(TARGET_DIR)/
endef

$(eval $(generic-package))
