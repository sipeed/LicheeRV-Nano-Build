EXTRA_CFLAGS += -DCONFIG_RTL8733B

#ifeq ($(CONFIG_MP_INCLUDED), y)
### 8733B Default Enable VHT MP HW TX MODE ###
#EXTRA_CFLAGS += -DCONFIG_MP_VHT_HW_TX_MODE
#CONFIG_MP_VHT_HW_TX_MODE = y
#endif

_HAL_INTFS_FILES +=	hal/rtl8733b/rtl8733b_halinit.o \
			hal/rtl8733b/rtl8733b_mac.o \
			hal/rtl8733b/rtl8733b_cmd.o \
			hal/rtl8733b/rtl8733b_phy.o \
			hal/rtl8733b/rtl8733b_ops.o \
			hal/rtl8733b/hal8733b_fw.o

ifeq ($(CONFIG_USB_HCI), y)
_HAL_INTFS_FILES +=	hal/rtl8733b/$(HCI_NAME)/rtl8733bu_halinit.o \
			hal/rtl8733b/$(HCI_NAME)/rtl8733bu_halmac.o \
			hal/rtl8733b/$(HCI_NAME)/rtl8733bu_io.o \
			hal/rtl8733b/$(HCI_NAME)/rtl8733bu_xmit.o \
			hal/rtl8733b/$(HCI_NAME)/rtl8733bu_recv.o \
			hal/rtl8733b/$(HCI_NAME)/rtl8733bu_led.o \
			hal/rtl8733b/$(HCI_NAME)/rtl8733bu_ops.o

_HAL_INTFS_FILES +=	hal/efuse/rtl8733b/HalEfuseMask8733B_USB.o
endif
ifeq ($(CONFIG_SDIO_HCI), y)
_HAL_INTFS_FILES +=	hal/rtl8733b/$(HCI_NAME)/rtl8733bs_halinit.o \
			hal/rtl8733b/$(HCI_NAME)/rtl8733bs_halmac.o \
			hal/rtl8733b/$(HCI_NAME)/rtl8733bs_io.o \
			hal/rtl8733b/$(HCI_NAME)/rtl8733bs_xmit.o \
			hal/rtl8733b/$(HCI_NAME)/rtl8733bs_recv.o \
			hal/rtl8733b/$(HCI_NAME)/rtl8733bs_led.o \
			hal/rtl8733b/$(HCI_NAME)/rtl8733bs_ops.o

_HAL_INTFS_FILES +=	hal/efuse/rtl8733b/HalEfuseMask8733B_SDIO.o

_HAL_INTFS_FILES +=	hal/hal_hci/hal_sdio_coex.o
endif

include $(src)/halmac-rs.mk

_BTC_FILES +=		hal/btc/halbtc8733bwifionly.o
ifeq ($(CONFIG_BT_COEXIST), y)
_BTC_FILES +=		hal/btc/halbtccommon.o \
			hal/btc/halbtc8733b.o
endif
