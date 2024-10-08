################################################################################
#
# nanokvm-server
#
################################################################################

NANOKVM_SERVER_VERSION = dc10e492df6f08061ed9098799dc5d7e9ee30aa2
NANOKVM_SERVER_SITE = $(call github,sipeed,NanoKVM,$(NANOKVM_SERVER_VERSION))

NANOKVM_SERVER_DEPENDENCIES = host-go

GO_BIN = $(HOST_DIR)/bin/go

NANOKVM_SERVER_GOMOD = server

# todo: build web, kvm_stream and kvm_system from source
define NANOKVM_SERVER_BUILD_CMDS
	cd $(@D)/$(NANOKVM_SERVER_GOMOD) ; \
	GOPROXY=direct $(GO_BIN) mod tidy ; \
	GOARCH=riscv64 GOOS=linux $(GO_BIN) build
endef

define NANOKVM_SERVER_INSTALL_TARGET_CMDS
	mkdir -pv $(TARGET_DIR)/kvmapp/server/
	rsync -r --verbose --copy-dirlinks --copy-links --hard-links ${@D}/server/NanoKVM-Server $(TARGET_DIR)/kvmapp/server/
endef

$(eval $(generic-package))
