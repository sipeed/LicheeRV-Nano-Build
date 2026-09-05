# ipc/hello_world/hello_world.mk
################################################################################
#
# hello_world
#
################################################################################

HELLO_WORLD_VERSION = 1.0.0
HELLO_WORLD_SITE = $(BR2_EXTERNAL)/../ipc/hello_world
HELLO_WORLD_SITE_METHOD = local

define HELLO_WORLD_BUILD_CMDS
    $(MAKE) CC="$(TARGET_CC)" CFLAGS="$(TARGET_CFLAGS)" -C $(@D)
endef

define HELLO_WORLD_INSTALL_TARGET_CMDS
    $(INSTALL) -D -m 0755 $(@D)/hello_world $(TARGET_DIR)/usr/bin/hello_world
endef

$(eval $(generic-package))
