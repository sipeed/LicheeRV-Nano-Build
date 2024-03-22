################################################################################
#
# tpudemo-sg200x
#
################################################################################

TPUDEMO_SG200X_VERSION = 345541b2f2a30e4efaa7505aa3b911826ec78505
TPUDEMO_SG200X_SITE = $(call github,0x754C,tpudemo-sg200x,$(TPUDEMO_SG200X_VERSION))

define TPUDEMO_SG200X_INSTALL_TARGET_CMDS
	mkdir -pv $(TARGET_DIR)/usr/bin/
	rsync -r --verbose --copy-dirlinks --copy-links --hard-links ${@D}/* $(TARGET_DIR)/usr/bin/
endef

$(eval $(generic-package))
