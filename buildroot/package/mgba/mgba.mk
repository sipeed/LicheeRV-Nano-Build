################################################################################
#
# mgba
#
################################################################################

MGBA_VERSION = a2587cb8cff265d9f327a9413b4f2c5e607850fb
MGBA_SITE = $(call github,mgba-emu,mgba,$(MGBA_VERSION))
MGBA_DEPENDENCIES = qt5base qt5svg qt5multimedia sdl

MGBA_CONF_OPTS += -DBUILD_SDL=ON
MGBA_CONF_OPTS += -DBUILD_QT=OFF
MGBA_CONF_OPTS += -DBUILD_GL=OFF
MGBA_CONF_OPTS += -DBUILD_GLES2=OFF
MGBA_CONF_OPTS += -DBUILD_GLES2=OFF
MGBA_CONF_OPTS += -DUSE_PNG=OFF

$(eval $(cmake-package))
