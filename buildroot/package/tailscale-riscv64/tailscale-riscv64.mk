################################################################################
#
# tailscale-riscv64
#
################################################################################

TAILSCALE_RISCV64_VERSION = latest
TAILSCALE_RISCV64_BASE = tailscale_riscv64
TAILSCALE_RISCV64_SOURCE = $(TAILSCALE_RISCV64_BASE).zip
TAILSCALE_RISCV64_SITE = https://cdn.sipeed.com/nanokvm/resources

define TAILSCALE_RISCV64_EXTRACT_CMDS
	$(UNZIP) -d $(@D) \
		$(TAILSCALE_RISCV64_DL_DIR)/$(TAILSCALE_RISCV64_SOURCE)
endef

define TAILSCALE_RISCV64_INSTALL_TARGET_CMDS
	mkdir -pv $(TARGET_DIR)/usr/bin
	mkdir -pv $(TARGET_DIR)/usr/sbin
	rsync -r --verbose --copy-dirlinks --copy-links --hard-links ${@D}/$(TAILSCALE_RISCV64_BASE)/tailscale $(TARGET_DIR)/usr/bin/
	rsync -r --verbose --copy-dirlinks --copy-links --hard-links ${@D}/$(TAILSCALE_RISCV64_BASE)/tailscaled $(TARGET_DIR)/usr/sbin/
endef

$(eval $(generic-package))
