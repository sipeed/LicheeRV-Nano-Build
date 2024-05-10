################################################################################
#
# tpudemo-sg200x
#
################################################################################

TPUDEMO_SG200X_VERSION = e50c97cbfc3f6018d674883a6a60bccd63bbf01d
TPUDEMO_SG200X_SITE = $(call github,0x754C,tpudemo-sg200x,$(TPUDEMO_SG200X_VERSION))

define TPUDEMO_SG200X_INSTALL_TARGET_CMDS
	mkdir -pv $(TARGET_DIR)/usr/bin/
	rsync -r --verbose --copy-dirlinks --copy-links --hard-links ${@D}/* $(TARGET_DIR)/usr/bin/
endef

$(eval $(generic-package))
