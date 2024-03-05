################################################################################
#
# tpudemo-sg200x
#
################################################################################

TPUDEMO_SG200X_VERSION = 118431aa403322f040dcad8ccd7f9fd7c07b9923
TPUDEMO_SG200X_SITE = $(call github,0x754C,tpudemo-sg200x,$(TPUDEMO_SG200X_VERSION))

define TPUDEMO_SG200X_INSTALL_TARGET_CMDS
	mkdir -pv $(TARGET_DIR)/usr/bin/
	rsync -r --verbose --copy-dirlinks --copy-links --hard-links ${@D}/* $(TARGET_DIR)/usr/bin/
endef

$(eval $(generic-package))
