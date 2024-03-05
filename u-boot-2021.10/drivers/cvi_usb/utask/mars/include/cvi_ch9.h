/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef CVI_CH9_H
#define CVI_CH9_H

#include "cvi_stdtypes.h"
#include "linux/bitops.h"

/** @defgroup ConfigInfo  Configuration and Hardware Operation Information
 *  The following definitions specify the driver operation environment that
 *  is defined by hardware configuration or client code. These defines are
 *  located in the header file of the core driver.
 *  @{
 */

/**********************************************************************
 * Defines
 **********************************************************************
 */

/** Data transfer direction */
#define	CH9_USB_DIR_HOST_TO_DEVICE 0

#define	CH9_USB_DIR_DEVICE_TO_HOST BIT(7)

/** Type of request */
#define	CH9_USB_REQ_TYPE_MASK (3 << 5)

#define	CH9_USB_REQ_TYPE_STANDARD (0 << 5)

#define	CH9_USB_REQ_TYPE_CLASS BIT(5)

#define	CH9_USB_REQ_TYPE_VENDOR (2 << 5)

#define	CH9_USB_REQ_TYPE_OTHER (3 << 5)

/** Recipient of request */
#define	CH9_REQ_RECIPIENT_MASK 0x0f

#define	CH9_USB_REQ_RECIPIENT_DEVICE 0

#define	CH9_USB_REQ_RECIPIENT_INTERFACE 1

#define	CH9_USB_REQ_RECIPIENT_ENDPOINT 2

#define	CH9_USB_REQ_RECIPIENT_OTHER 3

/** Standard  Request Code (chapter 9.4, Table 9-5 of USB Spec.) */
#define	CH9_USB_REQ_GET_STATUS 0

#define	CH9_USB_REQ_CLEAR_FEATURE 1

#define	CH9_USB_REQ_SET_FEATURE 3

#define	CH9_USB_REQ_SET_ADDRESS 5

#define	CH9_USB_REQ_GET_DESCRIPTOR 6

#define	CH9_USB_REQ_SET_DESCRIPTOR 7

#define	CH9_USB_REQ_GET_CONFIGURATION 8

#define	CH9_USB_REQ_SET_CONFIGURATION 9

#define	CH9_USB_REQ_GET_INTERFACE 10

#define	CH9_USB_REQ_SET_INTERFACE 11

#define	CH9_USB_REQ_SYNCH_FRAME 12

#define	CH9_USB_REQ_SET_ENCRYPTION 13

#define	CH9_USB_REQ_GET_ENCRYPTION 14

#define	CH9_USB_REQ_SET_HANDSHAKE 15

#define	CH9_USB_REQ_GET_HANDSHAKE 16

#define	CH9_USB_REQ_SET_CONNECTION 17

#define	CH9_USB_REQ_SET_SCURITY_DATA 18

#define	CH9_USB_REQ_GET_SCURITY_DATA 19

#define	CH9_USB_REQ_SET_WUSB_DATA 20

#define	CH9_USB_REQ_LOOPBACK_DATA_WRITE 21

#define	CH9_USB_REQ_LOOPBACK_DATA_READ 22

#define	CH9_USB_REQ_SET_INTERFACE_DS 23

#define CH9_USB_REQ_CVI_ENTER_DL 66

#define	CH9_USB_REQ_SET_SEL 48

#define	CH9_USB_REQ_ISOCH_DELAY 49

/** Standard Descriptor Types (chapter 9.4 - Table 9-6 of USB Spec.) */
#define	CH9_USB_DT_DEVICE 1

#define	CH9_USB_DT_CONFIGURATION 2

#define	CH9_USB_DT_STRING 3

#define	CH9_USB_DT_INTERFACE 4

#define	CH9_USB_DT_ENDPOINT 5

#define	CH9_USB_DT_DEVICE_QUALIFIER 6

/** USB 2 */
#define	CH9_USB_DT_OTHER_SPEED_CONFIGURATION 7

/** USB 2 */
#define	CH9_USB_DT_INTERFACE_POWER 8

#define	CH9_USB_DT_OTG 9

#define	CH9_USB_DT_DEBUG 10

#define	CH9_USB_DT_INTERFACE_ASSOCIATION 11

#define	CH9_USB_DT_BOS 15

#define	CH9_USB_DT_DEVICE_CAPABILITY 16

#define	CH9_USB_DT_SS_USB_ENDPOINT_COMPANION 48

#define	CH9_USB_DT_SS_PLUS_ISOCHRONOUS_ENDPOINT_COMPANION 49

#define	CH9_USB_DT_OTG 9

/** Descriptor size */
#define	CH9_USB_DS_DEVICE 18

#define	CH9_USB_DS_BOS 5

#define	CH9_USB_DS_DEVICE_ACM 12

/** Capability type: USB 2.0 EXTENSION */
#define	CH9_USB_DS_DEVICE_CAPABILITY_20 7

/** Capability type: SUPERSPEED_USB */
#define	CH9_USB_DS_DEVICE_CAPABILITY_30 10

/** Capability type: CONTAINER_ID */
#define	CH9_USB_DS_DEVICE_CAPABILITY_CONTAINER_ID 21

/** Capability type: Capability type: PRECISION_TIME_MEASUREMENT */
#define	CH9_USB_DS_DEVICE_CAPABILITY_PRECISION_TIME_MEASUREMENT 4

#define	CH9_USB_DS_CONFIGURATION 9

#define	CH9_USB_DS_INTERFACE_ASSOCIATION 8

#define	CH9_USB_DS_SS_USB_ENDPOINT_COMPANION 6

#define	CH9_USB_DS_SS_PLUS_ISOCHRONOUS_ENDPOINT_COMPANION 8

#define	CH9_USB_DS_INTERFACE 9

#define	CH9_USB_DS_ENDPOINT 7

#define	CH9_USB_DS_STRING 3

#define	CH9_USB_DS_OTG 5

/** USB2 */
#define	CH9_USB_DS_DEVICE_QUALIFIER 10

/** USB2 */
#define	CH9_USB_DS_OTHER_SPEED_CONFIGURATION 7

#define	CH9_USB_DS_INTERFACE_POWER 8

/** Standard Feature Selectors (chapter 9.4, Table 9-7 of USB Spec) */
#define	CH9_USB_FS_ENDPOINT_HALT 0

#define	CH9_USB_FS_FUNCTION_SUSPEND 0

#define	CH9_USB_FS_DEVICE_REMOTE_WAKEUP 1

#define	CH9_USB_FS_TEST_MODE 2

#define	CH9_USB_FS_B_HNP_ENABLE 3

#define	CH9_USB_FS_A_HNP_SUPPORT 4

#define	CH9_USB_FS_A_ALT_HNP_SUPPORT 5

#define	CH9_USB_FS_WUSB_DEVICE 6

#define	CH9_USB_FS_U1_ENABLE 48

#define	CH9_USB_FS_U2_ENABLE 49

#define	CH9_USB_FS_LTM_ENABLE 50

#define	CH9_USB_FS_B3_NTF_HOST_REL 51

#define	CH9_USB_FS_B3_RESP_ENABLE 52

#define	CH9_USB_FS_LDM_ENABLE 53

/** Recipient Device (Figure 9-4 of USB Spec) */
#define	CH9_USB_STATUS_DEV_SELF_POWERED BIT(0)

#define	CH9_USB_STATUS_DEV_REMOTE_WAKEUP BIT(1)

#define	CH9_USB_STATUS_DEV_U1_ENABLE BIT(2)

#define	CH9_USB_STATUS_DEV_U2_ENABLE BIT(3)

#define	CH9_USB_STATUS_DEV_LTM_ENABLE BIT(4)

/** Recipient Interface (Figure 9-5 of USB Spec) */
#define	CH9_USB_STATUS_INT_REMOTE_WAKE_CAPABLE BIT(0)

#define	CH9_USB_STATUS_INT_REMOTE_WAKEUP BIT(1)

/** Recipient Endpoint (Figure 9-6 of USB Spec) */
#define	CH9_USB_STATUS_EP_HALT BIT(1)

/** Recipient Endpoint - PTM GetStatus Request(Figure 9-7 of USB Spec) */
#define	CH9_USB_STATUS_EP_PTM_ENABLE BIT(0)

#define	CH9_USB_STATUS_EP_PTM_VALID BIT(1)

#define	CH9_USB_STATUS_EP_PTM_LINK_DELAY_OFFSET (16)

#define	CH9_USB_STATUS_EP_PTM_LINK_DELAY_MASK (0xFFFF << 16)

/**
 * Macros describing information for SetFeauture Request and FUNCTION_SUSPEND selector
 * (chapter 9.4.9, Table 9-9 of USB Spec)
 */
#define	CH9_USB_SF_LOW_POWER_SUSPEND_STATE 0x1

#define	CH9_USB_SF_REMOTE_WAKE_ENABLED 0x2

/**
 * Standard Class Code defined by usb.org
 * (link: http://www.usb.org/developers/defined_class)
 */
#define	CH9_USB_CLASS_INTERFACE 0x0

#define	CH9_USB_CLASS_AUDIO 0x01

#define	CH9_USB_CLASS_CDC 0x02

#define	CH9_USB_CLASS_COMMUNICATION 0x01

#define	CH9_USB_CLASS_HID 0x03

#define	CH9_USB_CLASS_PHYSICAL 0x05

#define	CH9_USB_CLASS_IMAGE 0x06

#define	CH9_USB_CLASS_PRINTER 0x07

#define	CH9_USB_CLASS_MASS_STORAGE 0x08

#define	CH9_USB_CLASS_HUB 0x09

#define	CH9_USB_CLASS_CDC_DATA 0x0A

#define	CH9_USB_CLASS_SMART_CARD 0x0B

#define	CH9_USB_CLASS_CONTENT_SEECURITY 0x0D

#define	CH9_USB_CLASS_VIDEO 0x0E

#define	CH9_USB_CLASS_HEALTHCARE 0x0F

#define	CH9_USB_CLASS_AUDIO_VIDEO 0x10

#define	CH9_USB_CLASS_DIAGNOSTIC 0xDC

#define	CH9_USB_CLASS_WIRELESS 0xE0

#define	CH9_USB_CLASS_MISCELLANEOUS 0xEF

#define	CH9_USB_CLASS_APPLICATION 0xFE

#define	CH9_USB_CLASS_VENDOR 0xFF

/** Device Capability Types Codes (see Table 9-14 of USB Spec 3.1 */
#define	CH9_USB_DCT_WIRELESS_USB 0x01

#define	CH9_USB_DCT_USB20_EXTENSION 0x02

#define	CH9_USB_DCT_SS_USB 0x03

#define	CH9_USB_DCT_CONTAINER_ID 0x04

#define	CH9_USB_DCT_PLATFORM 0x05

#define	CH9_USB_DCT_POWER_DELIVERY_CAPABILITY 0x06

#define	CH9_USB_DCT_BATTERY_INFO_CAPABILITY 0x07

#define	CH9_USB_DCT_PD_CONSUMER_PORT_CAPABILITY 0x08

#define	CH9_USB_DCT_PD_PROVIDER_PORT_CAPABILITY 0x09

#define	CH9_USB_DCT_SS_PLUS 0x0A

#define	CH9_USB_DCT_PRECISION_TIME_MEASUREMENT 0x0B

#define	CH9_USB_DCT_WIRELESS_USB_EXT 0x0C

/** Describe supports LPM defined in bm_attribues field of CUSBD_Usb20ExtensionDescriptor */
#define	CH9_USB_USB20_EXT_LPM_SUPPORT BIT(1)

#define	CH9_USB_USB20_EXT_BESL_AND_ALTERNATE_HIRD BIT(2)

/**
 * Describe supports LTM defined in bm_attribues field
 * of CUSBD_UsbSuperSpeedDeviceCapabilityDescriptor
 */
#define	CH9_USB_SS_CAP_LTM BIT(1)

/**
 * Describe speed supported defined in w_speed_supported field
 * of CUSBD_UsbSuperSpeedDeviceCapabilityDescriptor
 */
#define	CH9_USB_SS_CAP_SUPPORT_LS BIT(0)

#define	CH9_USB_SS_CAP_SUPPORT_FS BIT(1)

#define	CH9_USB_SS_CAP_SUPPORT_HS BIT(2)

#define	CH9_USB_SS_CAP_SUPPORT_SS BIT(3)

/** Describe encoding of bm_sublink_speed_attr0 filed from CUSBD_UsbSuperSpeedPlusDescriptor */
#define	CH9_USB_SSP_SID_OFFSET 0

#define	CH9_USB_SSP_SID_MASK 0 0x0000000f

#define	CH9_USB_SSP_LSE_OFFSET 4

#define	CH9_USB_SSP_LSE_MASK (0x00000003 << CUSBD_USB_SSP_LSE_OFFSET)

#define	CH9_USB_SSP_ST_OFFSET 6

#define	CH9_USB_SSP_ST_MASK (0x00000003 << CUSBD_USB_SSP_ST_OFFSET)

#define	CH9_USB_SSP_LP_OFFSET 14

#define	CH9_USB_SSP_LP_MASK (0x00000003 << CUSBD_USB_SSP_LP_OFFSET)

#define	CH9_USB_SSP_LSM_OFFSET 16

#define	CH9_USB_SSP_LSM_MASK (0x0000FFFF << CUSBD_USB_SSP_LSM_OFFSET)

/** Description of bm_attributes field from  Configuration Description */
#define	CH9_USB_CONFIG_RESERVED BIT(7)

/** Self Powered */
#define	CH9_USB_CONFIG_SELF_POWERED BIT(6)

#define CH9_USB_CONFIG_BUS_POWERED BIT(7)

/** Remote Wakeup */
#define	CH9_USB_CONFIG_REMOTE_WAKEUP BIT(5)

/** Definitions for b_endpoint_address field from  Endpoint descriptor */
#define	CH9_USB_EP_DIR_MASK 0x80

#define	CH9_USB_EP_DIR_IN 0x80

#define	CH9_USB_EP_NUMBER_MASK 0x0f

/** Endpoint attributes from Endpoint descriptor - bm_attributes field */
#define	CH9_USB_EP_TRANSFER_MASK 0x03

#define	CH9_USB_EP_CONTROL 0x0

#define	CH9_USB_EP_ISOCHRONOUS 0x01

#define	CH9_USB_EP_BULK 0x02

#define	CH9_USB_EP_INTERRUPT 0x03

/** Synchronization types for ISOCHRONOUS endpoints */
#define	CH9_USB_EP_SYNC_MASK 0xC

#define	CH9_USB_EP_SYNC_NO (0x00 >> 2)

#define	CH9_USB_EP_SYNC_ASYNCHRONOUS (0x1 >> 2)

#define	CH9_USB_EP_SYNC_ADAPTIVE (0x02 >> 2)

#define	CH9_USB_EP_SYNC_SYNCHRONOUS (0x03 >> 2)

#define	CH9_USB_EP_USAGE_MASK (0x3 >> 4)

/** Usage types for ISOCHRONOUS endpoints */
#define	CH9_USB_EP_USAGE_DATA (00 >> 4)

#define	CH9_USB_EP_USAGE_FEEDBACK (0x01 >> 4)

#define	CH9_USB_EP_USAGE_IMPLICIT_FEEDBACK (0x02 >> 4)

/** Usage types for INTERRUPTS endpoints */
#define	CH9_USB_EP_USAGE_PERIODIC (00 >> 4)

#define	CH9_USB_EP_USAGE_NOTIFICATION (0x01 >> 4)

/** Description of fields bm_attributes from OTG descriptor */
#define	CH9_USB_OTG_ADP_MASK 0x4

#define	CH9_USB_OTG_HNP_MASK 0x2

#define	CH9_USB_OTG_SRP_MASK 0x1

/**
 * Test Mode Selectors
 * See USB 2.0 spec Table 9-7
 */
#define	CH9_TEST_J 1

#define	CH9_TEST_K 2

#define	CH9_TEST_SE0_NAK 3

#define	CH9_TEST_PACKET 4

#define	CH9_TEST_FORCE_EN 5

#define	CH9_MAX_PACKET_SIZE_MASK 0x7ff

#define	CH9_PACKET_PER_FRAME_SHIFT 11

/**
 * OTG status selector
 * See USB_OTG_AND_EH_2-0 spec Table 6-4
 */
#define	CH9_OTG_STATUS_SELECTOR 0xF000

/**
 *  @}
 */

/* Conventional codes for class-specific descriptors.  The convention is
 * defined in the USB "Common Class" Spec (3.11).  Individual class specs
 * are authoritative for their usage, not the "common class" writeup.
 */
#define USB_DT_CS_DEVICE		(CH9_USB_REQ_TYPE_CLASS | CH9_USB_DT_DEVICE)
#define USB_DT_CS_CONFIG		(CH9_USB_REQ_TYPE_CLASS | CH9_USB_DT_CONFIG)
#define USB_DT_CS_STRING		(CH9_USB_REQ_TYPE_CLASS | CH9_USB_DT_STRING)
#define USB_DT_CS_INTERFACE		(CH9_USB_REQ_TYPE_CLASS | CH9_USB_DT_INTERFACE)
#define USB_DT_CS_ENDPOINT		(CH9_USB_REQ_TYPE_CLASS | CH9_USB_DT_ENDPOINT)

/** @defgroup DataStructure Dynamic Data Structures
 *  This section defines the data structures used by the driver to provide
 *  hardware information, modification and dynamic operation of the driver.
 *  These data structures are defined in the header file of the core driver
 *  and utilized by the API.
 *  @{
 */

/**********************************************************************
 * Forward declarations
 **********************************************************************/
struct ch9_usb_setup;
struct ch9_usb_device_descriptor;
struct ch9_usb_bos_descriptor;
struct ch9_usb_capability_descriptor;
struct ch9_usb20_extension_descriptor;
struct ch9_usb_ss_device_capability_descriptor;
struct ch9_usb_container_id_descriptor;
struct ch9_usb_platform_descriptor;
struct ch9_usb_ss_plus_descriptor;
struct ch9_usb_ptm_capability_descriptor;
struct ch9_usb_configuration_descriptor;
struct ch9_usb_interface_association_descriptor;
struct ch9_usb_interface_descriptor;
struct ch9_usb_endpoint_descriptor;
struct ch9_usb_ss_endpoint_companion_descriptor;
struct ch9_usb_ss_plus_isoc_endpoint_companion_descriptor;
struct ch9_usb_sstring_descriptor;
struct ch9_usb_device_qualifier_descriptor;
struct ch9_usb_other_speed_configuration_descriptor;
struct ch9_usb_header_descriptor;
struct ch9_usb_otg_descriptor;
struct ch9_config_params;

/**********************************************************************
 * Enumerations
 **********************************************************************/
/** USB States defined in USB Specification */
typedef enum {
    /** Device not attached yet */
	CH9_USB_STATE_NONE = 0,
    /** see Figure 9-1 of USB Spec */
	CH9_USB_STATE_ATTACHED = 1,
	CH9_USB_STATE_POWERED = 2,
	CH9_USB_STATE_DEFAULT = 3,
	CH9_USB_STATE_ADDRESS = 4,
	CH9_USB_STATE_CONFIGURED = 5,
	CH9_USB_STATE_SUSPENDED = 6,
	CH9_USB_STATE_ERROR = 7,
} ch9_usb_state;

/** Speeds defined in USB Specification */
typedef enum {
    /** unknown speed - before enumeration */
	CH9_USB_SPEED_UNKNOWN = 0,
    /** (1,5Mb/s) */
	CH9_USB_SPEED_LOW = 1,
    /** usb 1.1 (12Mb/s) */
	CH9_USB_SPEED_FULL = 2,
    /** usb 2.0 (480Mb/s) */
	CH9_USB_SPEED_HIGH = 3,
    /** usb 2.5 wireless */
	CH9_USB_SPEED_WIRELESS = 4,
    /** usb 3.0 GEN 1  (5Gb/s) */
	CH9_USB_SPEED_SUPER = 5,
    /** usb 3.1 GEN2 (10Gb/s) */
	CH9_USB_SPEED_SUPER_PLUS = 6,
} ch9_usb_speed;

/**********************************************************************
 * Structures and unions
 *********************************************************************
 */

/** Structure describes USB request (SETUP packet). See USB Specification (chapter 9.3) */
typedef struct ch9_usb_setup {
    /** Characteristics of request */
	u8 bm_request_type;
    /** Specific request */
	u8 b_request;
    /** Field that varies according to request */
	u16 w_value;
    /** typically used to pass an index or offset. */
	u16 w_index;
    /** Number of bytes to transfer if there is a data stage */
	u16 w_length;
} __packed ch9_usb_setup;

/** Standard Device Descriptor (see Table 9-11 of USB Spec 3.1) */
typedef struct ch9_usb_device_descriptor {
    /** Size of descriptor */
	u8 b_length;
    /** Device descriptor type */
	u8 b_descriptor_type;
    /** USB Specification Release Number */
	u16 bcd_usb;
    /** Class code (assigned by the USB-IF) */
	u8 b_device_class;
    /** Subclass code (assigned by the USB-IF */
	u8 b_device_sub_class;
    /** Protocol code (assigned by the USB-IF */
	u8 b_device_protocol;
    /** Maximum packet size for endpoint zero */
	u8 b_max_packet_size0;
    /** Vendor ID (assigned by the USB-IF */
	u16 id_vendor;
    /** Product ID (assigned by manufacturer) */
	u16 id_product;
    /** Device release number */
	u16 bcd_device;
    /** Index of string descriptor describing manufacturer */
	u8 i_manufacturer;
    /** Index of string descriptor describing product */
	u8 i_product;
    /** Index of string descriptor for serial number */
	u8 i_serial_number;
    /** Number of possible configurations */
	u8 b_num_configurations;
} __packed ch9_usb_device_descriptor;

/** Binary Device Object Store descriptor (see Table 9-12 of USB Spec 3.1) */
typedef struct ch9_usb_bos_descriptor {
    /** Size of this descriptor */
	u8 b_length;
    /** Descriptor type: BOS */
	u8 b_descriptor_type;
    /** Length of this descriptor and all of its sub descriptors */
	u16 w_total_length;
    /** The number of separate device capability descriptors in the BOS */
	u8 b_num_device_caps;
} __packed ch9_usb_bos_descriptor;

/** Device Capability Descriptor (see Table 9-12 of USB Spec 3.1) */
typedef struct ch9_usb_capability_descriptor {
    /** Size of this descriptor */
	u8 b_length;
    /** Descriptor type: DEVICE CAPABILITY type */
	u8 b_descriptor_type;
    /** Capability type: USB 2.0 EXTENSION (002h) */
	u8 b_dev_capability_type;
    /** Capability specific format */
	u32 bm_attributes;
} __packed ch9_usb_capability_descriptor;

/** USB 2.0 Extension Descriptor (see Table 9-15 of USB Spec 3.1) */
typedef struct ch9_usb20_extension_descriptor {
    /** Size of this descriptor */
	u8 b_length;
    /** Descriptor type: DEVICE CAPABILITY type */
	u8 b_descriptor_type;
    /** Capability type: USB 2.0 EXTENSION (002h) */
	u8 b_dev_capability_type;
    /** Capability specific format */
	u32 bm_attributes;
} __packed ch9_usb20_extension_descriptor;

/** SuperSpeed USB Device Capability Descriptor (see Table 9-16 of USB Spec 3.1) */
typedef struct ch9_usb_ss_device_capability_descriptor {
    /** Size of this descriptor */
	u8 b_length;
    /** DEVICE CAPABILITY Descriptor type */
	u8 b_descriptor_type;
    /** Capability type: SUPERSPEED_USB */
	u8 b_dev_capability_type;
    /** Bitmap encoding of supported device level features */
	u8 bm_attributes;
    /** Bitmap encoding of the speed supported by device */
	u16 w_speed_supported;
    /**
     * The lowest speed at which all the functionality
     * supported by the device is available to the user
     */
	u8 v_functionality_support;
    /** U1 Device Exit Latency */
	u8 b_u1_dev_exit_lat;
    /** U2 Device Exit Latency */
	u16 b_u2_dev_exit_lat;
} __packed ch9_usb_ss_device_capability_descriptor;

/** Container ID Descriptor (see Table 9-17 of USB Spec 3.1) */
typedef struct ch9_usb_container_id_descriptor {
    /** Size of this descriptor */
	u8 b_length;
    /** DEVICE CAPABILITY Descriptor type */
	u8 b_descriptor_type;
    /** Capability type: CONTAINER_ID */
	u8 b_dev_capability_type;
    /** Field reserved and shall be set to zero */
	u8 b_reserved;
    /** unique number to device instance */
	u8 container_id[16];
} __packed ch9_usb_container_id_descriptor;

typedef struct ch9_usb_platform_descriptor {
    /** Size of this descriptor */
	u8 b_length;
    /** DEVICE CAPABILITY Descriptor type */
	u8 b_descriptor_type;
    /** Capability type: PLATFORM */
	u8 b_dev_capability_type;
    /** Field reserved and shall be set to zero */
	u8 b_reserved;
    /** unique number to identifies a platform */
	u8 platform_capability_uuid[16];
    /** variable length */
	u8 capability_data[0];
} __packed ch9_usb_platform_descriptor;

/** SuperSpeedPlus USB Device Capability  (see Table 9-19 of USB Spec 3.1) */
typedef struct ch9_usb_ss_plus_descriptor {
    /** Size of this descriptor */
	u8 b_length;
    /** DEVICE CAPABILITY Descriptor type */
	u8 b_descriptor_type;
    /** Capability type: SUPERSPEED_PLUS */
	u8 b_dev_capability_type;
    /** Field reserved and shall be set to zero */
	u8 b_reserved;
    /** Bitmap encoding of supported SuperSpeedPlus features */
	u32 bm_attributes;
    /** supported functionality */
	u16 w_functionality_support;
    /** Reserved. Shall be set to zero */
	u16 w_reserved;
    /** Sublink Speed Attribute */
	u32 bm_sublink_speed_attr0;
    /** Additional Lane Speed Attributes */
	u32 bm_sublink_speed_attr_ssac;
} __packed ch9_usb_ss_plus_descriptor;

/** SuperSpeedPlus USB Device Capability  (see Table 9-19 of USB Spec 3.1) */
typedef struct ch9_usb_ptm_capability_descriptor {
	u8 b_length;
	u8 b_descriptor_type;
	u8 b_dev_capability_type;
} __packed ch9_usb_ptm_capability_descriptor;

/** Standard Configuration Descriptor (see Table 9-21 of USB Spec 3.1) */
typedef struct ch9_usb_configuration_descriptor {
    /** Size of descriptor */
	u8 b_length;
    /** Configuration descriptor type */
	u8 b_descriptor_type;
    /** Total length of configuration */
	u16 w_total_length;
    /** Number of interfaces supported by configuration */
	u8 b_num_interfaces;
    /** Value use as an argument to SetConfiguration() request */
	u8 b_configuration_value;
    /** Index of string descriptor describing this configuration */
	u8 i_configuration;
    /** Configuration attributes */
	u8 bm_attributes;
    /** Maximum power consumption of the USB device */
	u8 b_max_power;
} __packed ch9_usb_configuration_descriptor;

/** Standard Interface Association Descriptor  (see Table 9-22 of USB Spec 3.1) */
typedef struct ch9_usb_interface_association_descriptor {
    /** Size of descriptor */
	u8 b_length;
    /** Interface Association Descriptor Type */
	u8 b_descriptor_type;
    /** interface number of this interface that is associated with this function */
	u8 b_first_interface;
    /** Number of contiguous interfaces that are associated with this function */
	u8 b_interface_count;
    /** Class code assigned by USB-IF */
	u8 b_function_class;
    /** Subclass code */
	u8 b_function_sub_class;
    /** Protocol code */
	u8 b_function_protocol;
    /** Index of string descriptor describing this function */
	u8 i_function;
} __packed ch9_usb_interface_association_descriptor;

/** Standard Interface Descriptor (see Table 9-23 of USB Spec 3.1) */
typedef struct ch9_usb_interface_descriptor {
    /** Size of descriptor */
	u8 b_length;
    /** Interface Descriptor Type */
	u8 b_descriptor_type;
    /** Number of this interface */
	u8 b_interface_number;
    /** Value used to select this alternate setting */
	u8 b_alternate_setting;
    /** Class code */
	u8 b_num_endpoints;
    /** Subclass code */
	u8 b_interface_class;
    /** Subclass code */
	u8 b_interface_sub_class;
    /** Protocol code */
	u8 b_interface_protocol;
    /** Index of string */
	u8 i_interface;
} __packed ch9_usb_interface_descriptor;

#define USB_DT_INTERFACE_SIZE		9

#define USB_DIR_OUT			0		/* to device */
#define USB_DIR_IN			0x80		/* to host */
/** Standard Endpoint Descriptor */
typedef struct ch9_usb_endpoint_descriptor {
    /** Size of descriptor */
	u8 b_length;
    /** Endpoint Descriptor Type */
	u8 b_descriptor_type;
    /** The address of the endpoint */
	u8 b_endpoint_address;
    /** Endpoint attribute */
	u8 bm_attributes;
    /** Maximum packet size for this endpoint */
	u16 w_max_packet_size;
    /** interval for polling endpoint data transfer */
	u8 b_interval;
} __packed ch9_usb_endpoint_descriptor;

#define USB_DT_ENDPOINT_SIZE		7
#define USB_DT_ENDPOINT_AUDIO_SIZE	9	/* Audio extension */

/** Standard SuperSpeed Endpoint Companion Descriptor (see Table 9-26 of USB Spec 3.1) */
typedef struct ch9_usb_ss_endpoint_companion_descriptor {
    /** Size of descriptor in bytes */
	u8 b_length;
    /** SUPERSPEED_USB_ENDPOINT_COMPANION Descriptor types */
	u8 b_descriptor_type;
    /** Number of packets that endpoint can transmit as part of burst */
	u8 b_max_burst;
	u8 bm_attributes;
    /** The total number of bytes  for every service interval */
	u16 w_bytes_per_interval;
} __packed ch9_usb_ss_endpoint_companion_descriptor;

/**
 * Standard SuperSpeedPlus Isochronous Endpoint
 * Companion Descriptor (see Table 9-27 of USB Spec 3.1)
 */
typedef struct ch9_usb_ss_plus_isoc_endpoint_companion_descriptor {
    /** Size of descriptor in bytes */
	u8 b_length;
    /** SUPERSPEEDPLUS_ISOCHRONOUS_ENDPOINT_COMPANION Descriptor types */
	u8 b_descriptor_type;
    /** Reserved. Shall be set to zero */
	u16 w_reserved;
    /** The total number of bytes  for every service interval */
	u32 dw_bytes_per_interval;
} __packed ch9_usb_ss_plus_isoc_endpoint_companion_descriptor;

/** Standard String Descriptor */
typedef struct ch9_usb_sstring_descriptor {
    /** Size of descriptor */
	u8 b_length;
    /** STRING Descriptor Type */
	u8 b_descriptor_type;
    /** UNICODE encoded string */
	u8 *b_string;
} __packed ch9_usb_sstring_descriptor;

/** Standard Device Qualifier Descriptor (see Table 9-9 of USB Spec 2.0) */
typedef struct ch9_usb_device_qualifier_descriptor {
    /** Size of descriptor */
	u8 b_length;
    /** Device Qualifier type */
	u8 b_descriptor_type;
    /** USB Specification version number */
	u16 bcd_usb;
    /** Class code */
	u8 b_device_class;
    /** Subclass code */
	u8 b_device_sub_class;
    /** Protocol code */
	u8 b_device_protocol;
    /** Maximum packet size for other speed */
	u8 b_max_packet_size0;
    /** Number of other speed configuration */
	u8 b_num_configurations;
    /** Reserved for future use */
	u8 b_reserved;
} __packed ch9_usb_device_qualifier_descriptor;

/** Standard Other_Speed_Configuration descriptor (see Table 9-11 of USB Spec 2.0) */
typedef struct ch9_usb_other_speed_configuration_descriptor {
    /** Size of descriptor */
	u8 b_length;
    /** Configuration descriptor type */
	u8 b_descriptor_type;
    /** Total length of configuration */
	u16 w_total_length;
    /** Number of interfaces supported by this speed configuration */
	u8 b_num_interfaces;
    /** Value to use to select configuration */
	u8 b_configuration_value;
    /** Index of string descriptor describing this configuration */
	u8 i_configuration;
    /** Configuration attributes */
	u8 bm_attributes;
    /** Maximum power consumption of the USB device */
	u8 b_max_power;
} __packed ch9_usb_other_speed_configuration_descriptor;

/**
 * Header descriptor. All descriptor have the same header that
 * consist of b_length and b_descriptor_type fields
 */
typedef struct ch9_usb_header_descriptor {
    /** Size of descriptor */
	u8 b_length;
    /** descriptor Type */
	u8 b_descriptor_type;
} __packed ch9_usb_header_descriptor;

/** OTG descriptor (see OTG spec. Table 6.1) */
typedef struct ch9_usb_otg_descriptor {
    /** Size of descriptor */
	u8 b_length;
    /** OTG Descriptor Type */
	u8 b_descriptor_type;
    /** Attribute field */
	u8 bm_attributes;
    /** OTG and EH supplement release number */
	u16 bcd_otg;
} __packed ch9_usb_otg_descriptor;

typedef struct ch9_config_params {
    /** U1 Device exit Latency */
	u8 b_u1_dev_exit_lat;
    /** U2 Device exit Latency */
	u16 b_u2_dev_exit_lat;
} __packed ch9_config_params;

/**
 *  @}
 */

#endif	/* CVI_CH9_H */
