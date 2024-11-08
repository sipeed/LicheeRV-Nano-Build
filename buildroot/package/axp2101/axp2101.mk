################################################################################
#
# axp2101
#
################################################################################

AXP2101_VERSION = 1.1.0
AXP2101_SITE = $(TOPDIR)/package/axp2101/src
AXP2101_SITE_METHOD = local
AXP2101_CFLAGS = -Iinclude -IXPowersLib -IXPowersLib/REG -Wall -Wextra

define AXP2101_BUILD_CMDS
    $(MAKE) $(TARGET_CONFIGURE_OPTS) CFLAGS="$(AXP2101_CFLAGS)" -C $(@D)
endef

define AXP2101_INSTALL_TARGET_CMDS
    $(INSTALL) -D -m 0755 $(@D)/axp2101 $(TARGET_DIR)/usr/bin/axp2101
endef

$(eval $(generic-package))
