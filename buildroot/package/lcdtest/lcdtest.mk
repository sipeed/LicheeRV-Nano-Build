################################################################################
#
# lcdtest
#
################################################################################

LCDTEST_VERSION = 9dc8793ff0a6cdc973cf823af6447a79a4da6cac
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
