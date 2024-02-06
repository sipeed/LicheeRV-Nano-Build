################################################################################
#
# licheervnano-overlay
#
################################################################################

LICHEERVNANO_OVERLAY_VERSION = 2024-02-02-06-rev3
LICHEERVNANO_OVERLAY_SITE = "$(TOPDIR)/package/licheervnano-overlay/files/"
LICHEERVNANO_OVERLAY_SITE_METHOD = local
# these package must install before licheervnano-overlay
LICHEERVNANO_OVERLAY_DEPENDENCIES += avahi busybox

define LICHEERVNANO_OVERLAY_INSTALL_TARGET_CMDS
	cp -arvf $(@D)/* $(TARGET_DIR)/
endef

$(eval $(generic-package))
