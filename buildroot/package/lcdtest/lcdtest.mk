################################################################################
#
# lcdtest
#
################################################################################

LCDTEST_VERSION = d6defe34015d64b70963e16ee0e1d274387ec060
LCDTEST_SITE = $(call github,0x754C,lcdtest,$(LCDTEST_VERSION))

define LCDTEST_BUILD_CMDS
	$(TARGET_CONFIGURE_OPTS) $(MAKE) -C $(@D)
endef

define LCDTEST_INSTALL_TARGET_CMDS
	mkdir -p $(TARGET_DIR)/usr/bin
	cp $(@D)/fbpattern $(TARGET_DIR)/usr/bin/
	cp $(@D)/fbbar $(TARGET_DIR)/usr/bin/
endef

$(eval $(generic-package))
