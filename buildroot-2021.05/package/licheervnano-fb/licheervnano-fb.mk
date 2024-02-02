################################################################################
#
# licheervnano-fb
#
################################################################################

LICHEERVNANO_FB_VERSION = 2024-02-01-rev23
LICHEERVNANO_FB_SITE = "$(TOPDIR)/package/licheervnano-fb/src/"
LICHEERVNANO_FB_SITE_METHOD = local

define LICHEERVNANO_FB_BUILD_CMDS
	$(TARGET_MAKE_ENV) CC=$(TARGET_CC) $(MAKE) -C $(@D)/
endef

define LICHEERVNANO_FB_INSTALL_TARGET_CMDS
	mkdir -p $(TARGET_DIR)/opt/lcd/
	$(INSTALL) -m 755 -D $(@D)/fbbar \
		$(TARGET_DIR)/opt/lcd/fbbar
	$(INSTALL) -m 755 -D $(@D)/fbpattern \
		$(TARGET_DIR)/opt/lcd/fbpattern
	$(INSTALL) -m 755 -D $(@D)/fbdaemon \
		$(TARGET_DIR)/opt/lcd/fbdaemon
	mkdir -pv $(TARGET_DIR)/etc/init.d/
	$(INSTALL) -m 755 -D $(@D)/S05fbdaemon \
		$(TARGET_DIR)/etc/init.d/S05fbdaemon
endef

$(eval $(generic-package))
