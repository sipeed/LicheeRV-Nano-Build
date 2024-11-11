################################################################################
#
# cvitek-riscv64-musl-sysroot
#
################################################################################

CVITEK_RISCV64_MUSL_SYSROOT_VERSION = fb0a6ac7409fb477c5f46ced75bf05527def7cb9
CVITEK_RISCV64_MUSL_SYSROOT_SITE = $(call github,0x754C,cvitek-riscv64-musl-sysroot,$(CVITEK_RISCV64_MUSL_SYSROOT_VERSION))

CVITEK_EXT_HOST_TOOLS = ../../../../host-tools
CVITEK_EXT_SYSROOT_LIB = gcc/riscv64-linux-musl-x86_64/sysroot/lib

define CVITEK_RISCV64_MUSL_SYSROOT_EXTRACT_CMDS
	mkdir -pv $(@D)/usr/lib/
	if [ -e $(@D)/$(CVITEK_EXT_HOST_TOOLS)/$(CVITEK_EXT_SYSROOT_LIB) ]; then \
		rsync -r --verbose --copy-dirlinks --copy-links --hard-links $(@D)/$(CVITEK_EXT_HOST_TOOLS)/$(CVITEK_EXT_SYSROOT_LIB)/ld-*.so* $(@D)/usr/lib/ ; \
	fi
	if [ ! -e $(@D)/lib ]; then \
		ln -s usr/lib $(@D)/lib ; \
	fi
endef

define CVITEK_RISCV64_MUSL_SYSROOT_INSTALL_TARGET_CMDS
	rsync -av ${@D}/* $(TARGET_DIR)/
endef

$(eval $(generic-package))
