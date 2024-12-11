################################################################################
#
# tailscale-riscv64
#
################################################################################

TAILSCALE_RISCV64_VERSION = 1.72.1
TAILSCALE_RISCV64_COMMIT = f4a95663c
TAILSCALE_RISCV64_SITE = $(call github,tailscale,tailscale,v$(TAILSCALE_RISCV64_VERSION))

TAILSCALE_RISCV64_DEPENDENCIES = host-go

GO_BIN = $(HOST_DIR)/bin/go

TAILSCALE_RISCV64_GO_ENV = GOARCH=riscv64 GOOS=linux
TAILSCALE_RISCV64_TAGS = osusergo,netgo
TAILSCALE_RISCV64_LDFLAGS = -extldflags '-static'
TAILSCALE_RISCV64_LDFLAGS += -X tailscale.com/version.longStamp=$(TAILSCALE_RISCV64_VERSION)-t$(TAILSCALE_RISCV64_COMMIT)
TAILSCALE_RISCV64_LDFLAGS += -X tailscale.com/version.shortStamp=$(TAILSCALE_RISCV64_VERSION)

TAILSCALE_RISCV64_BUILD_OPTS = -tags=$(TAILSCALE_RISCV64_TAGS) -ldflags="$(TAILSCALE_RISCV64_LDFLAGS)"

define TAILSCALE_RISCV64_BUILD_CMDS
	cd $(@D) ; \
	GOPROXY=direct $(TAILSCALE_RISCV64_GO_ENV) $(GO_BIN) build -v $(TAILSCALE_RISCV64_BUILD_OPTS) tailscale.com/cmd/tailscale
	cd $(@D) ; \
	GOPROXY=direct $(TAILSCALE_RISCV64_GO_ENV) $(GO_BIN) build -v $(TAILSCALE_RISCV64_BUILD_OPTS) tailscale.com/cmd/tailscaled
endef

define TAILSCALE_RISCV64_INSTALL_TARGET_CMDS
	mkdir -pv $(TARGET_DIR)/usr/bin
	mkdir -pv $(TARGET_DIR)/usr/sbin
	rsync -r --verbose --copy-dirlinks --copy-links --hard-links ${@D}/tailscale $(TARGET_DIR)/usr/bin/
	rsync -r --verbose --copy-dirlinks --copy-links --hard-links ${@D}/tailscaled $(TARGET_DIR)/usr/sbin/
endef

$(eval $(generic-package))
