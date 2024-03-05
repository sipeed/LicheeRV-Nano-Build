################################################################################
#
# cvitek-riscv64-musl-sysroot
#
################################################################################

CVITEK_RISCV64_MUSL_SYSROOT_VERSION = fb0a6ac7409fb477c5f46ced75bf05527def7cb9
CVITEK_RISCV64_MUSL_SYSROOT_SITE = $(call github,0x754C,cvitek-riscv64-musl-sysroot,$(CVITEK_RISCV64_MUSL_SYSROOT_VERSION))

define CVITEK_RISCV64_MUSL_SYSROOT_INSTALL_TARGET_CMDS
	rsync -av ${@D}/* $(TARGET_DIR)/
endef

$(eval $(generic-package))
