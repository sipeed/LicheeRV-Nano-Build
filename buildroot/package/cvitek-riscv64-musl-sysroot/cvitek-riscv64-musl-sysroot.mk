################################################################################
#
# cvitek-riscv64-musl-sysroot
#
################################################################################

CVITEK_RISCV64_MUSL_SYSROOT_VERSION = 1.0.0
CVITEK_RISCV64_MUSL_SYSROOT_SITE = $(TOPDIR)/package/cvitek-riscv64-musl-sysroot
CVITEK_RISCV64_MUSL_SYSROOT_SITE_METHOD = local

define CVITEK_RISCV64_MUSL_SYSROOT_BUILD_CMDS
	rm -f $(@D)/Config*
	rm -f $(@D)/*.mk
	mkdir -pv $(@D)/usr/lib/
	if [ ! -e $(@D)/lib ]; then \
		ln -s usr/lib $(@D)/lib ; \
	fi
	if [ -e $(realpath $(TOOLCHAIN_EXTERNAL_BIN)../sysroot/lib) ]; then \
		rsync -r --verbose --copy-dirlinks --copy-links --hard-links $(realpath $(TOOLCHAIN_EXTERNAL_BIN)../sysroot/lib)/ld-*.so* $(@D)/usr/lib/ ; \
	fi
endef

define CVITEK_RISCV64_MUSL_SYSROOT_INSTALL_TARGET_CMDS
	rsync -av ${@D}/* $(TARGET_DIR)/
endef

$(eval $(generic-package))
