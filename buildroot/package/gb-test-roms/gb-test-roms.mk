################################################################################
#
# gb-test-roms
#
################################################################################

GB_TEST_ROMS_VERSION = 7.0
GB_TEST_ROMS_SOURCE = game-boy-test-roms-v$(GB_TEST_ROMS_VERSION).zip
GB_TEST_ROMS_SITE = https://github.com/c-sp/gameboy-test-roms/releases/download/v$(GB_TEST_ROMS_VERSION)

define GB_TEST_ROMS_EXTRACT_CMDS
        unzip $(GB_TEST_ROMS_DL_DIR)/$(GB_TEST_ROMS_SOURCE) -d $(@D)
endef

define GB_TEST_ROMS_INSTALL_TARGET_CMDS
	mkdir -pv $(TARGET_DIR)/usr/share/gb-test-roms/
	rsync -r --verbose --copy-dirlinks --copy-links --hard-links ${@D}/* $(TARGET_DIR)/usr/share/gb-test-roms/
endef

$(eval $(generic-package))
