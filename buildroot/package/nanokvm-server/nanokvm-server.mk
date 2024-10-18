################################################################################
#
# nanokvm-server
#
################################################################################

NANOKVM_SERVER_VERSION = bcfa09b03ea636ae7dcdc39ca9c49732168286a2
NANOKVM_SERVER_SITE = $(call github,sipeed,NanoKVM,$(NANOKVM_SERVER_VERSION))

NANOKVM_SERVER_DEPENDENCIES = host-go host-nodejs host-python3

GO_BIN = $(HOST_DIR)/bin/go

HOST_NODEJS_BIN_ENV = $(HOST_CONFIGURE_OPTS) \
	LDFLAGS="$(NODEJS_LDFLAGS)" \
	LD="$(HOST_CXX)" \
	PATH=$(BR_PATH) \
	npm_config_build_from_source=true \
	npm_config_nodedir=$(HOST_DIR)/usr \
	npm_config_prefix=$(HOST_DIR)/usr \
	npm_config_cache=$(BUILD_DIR)/.npm-cache

HOST_COREPACK = $(HOST_NODEJS_BIN_ENV) $(HOST_DIR)/bin/corepack

NANOKVM_SERVER_GOMOD = server

# todo: build kvm_stream and kvm_system from source
define NANOKVM_SERVER_BUILD_CMDS
	cd $(@D)/$(NANOKVM_SERVER_GOMOD) ; \
	GOPROXY=direct $(GO_BIN) mod tidy
	cd $(@D)/$(NANOKVM_SERVER_GOMOD) ; \
	GOARCH=riscv64 GOOS=linux $(GO_BIN) build
	cd $(@D)/web ; \
	$(HOST_COREPACK) pnpm install
	cd $(@D)/web ; \
	$(HOST_COREPACK) pnpm build
endef

define NANOKVM_SERVER_INSTALL_TARGET_CMDS
	if [ "X$(BR2_PACKAGE_TAILSCALE_RISCV64)" = "Xy" ]; then \
		rm -f $(TARGET_DIR)/etc/tailscale_disabled ; \
	else \
		mkdir -pv $(TARGET_DIR)/etc/ ; \
		touch $(TARGET_DIR)/etc/tailscale_disabled ; \
	fi
	mkdir -pv $(TARGET_DIR)/kvmapp/
	#touch $(TARGET_DIR)/kvmapp/force_dl_lib
	mkdir -pv $(TARGET_DIR)/kvmapp/server/
	rsync -r --verbose --copy-dirlinks --copy-links --hard-links ${@D}/server/NanoKVM-Server $(TARGET_DIR)/kvmapp/server/
	mkdir -pv $(TARGET_DIR)/kvmapp/server/web/
	rsync -r --verbose --copy-dirlinks --copy-links --hard-links ${@D}/web/dist/ $(TARGET_DIR)/kvmapp/server/web/
endef

$(eval $(generic-package))
