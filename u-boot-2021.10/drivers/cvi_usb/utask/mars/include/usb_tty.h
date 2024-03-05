/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef _USB_TTY_H
#define _USB_TTY_H

#include "byteorder.h"
#include "cvi_ch9.h"

//------------------------------------------------------------------------------
/* CDC DEFINE*/
#define USB_CDC_SUBCLASS_ACM 0x02

#define USB_CDC_PROTO_NONE 0

#define USB_CDC_ACM_PROTO_AT_V25TER 1
#define USB_CDC_ACM_PROTO_AT_PCCA101 2
#define USB_CDC_ACM_PROTO_AT_PCCA101_WAKE 3
#define USB_CDC_ACM_PROTO_AT_GSM 4
#define USB_CDC_ACM_PROTO_AT_3G 5
#define USB_CDC_ACM_PROTO_AT_CDMA 6
#define USB_CDC_ACM_PROTO_VENDOR 0xff

/*-------------------------------------------------------------------------*/

/*
 * Class-Specific descriptors ... there are a couple dozen of them
 */

#define USB_CDC_HEADER_TYPE 0x00 /* header_desc */
#define USB_CDC_CALL_MANAGEMENT_TYPE 0x01 /* call_mgmt_descriptor */
#define USB_CDC_ACM_TYPE 0x02 /* acm_descriptor */
#define USB_CDC_UNION_TYPE 0x06 /* union_desc */
#define USB_CDC_COUNTRY_TYPE 0x07

/* "Header Functional Descriptor" from CDC spec  5.2.3.1 */
struct usb_cdc_header_desc {
	uint8_t b_length;
	uint8_t b_descriptor_type;
	uint8_t b_descriptor_sub_type;

	uint16_t bcd_cdc;
} __packed;

/* "Call Management Descriptor" from CDC spec  5.2.3.2 */
struct usb_cdc_call_mgmt_descriptor {
	uint8_t b_length;
	uint8_t b_descriptor_type;
	uint8_t b_descriptor_sub_type;

	uint8_t bm_capabilities;
#define USB_CDC_CALL_MGMT_CAP_CALL_MGMT 0x01
#define USB_CDC_CALL_MGMT_CAP_DATA_INTF 0x02

	uint8_t b_data_interface;
} __packed;

/* "Abstract Control Management Descriptor" from CDC spec  5.2.3.3 */
struct usb_cdc_acm_descriptor {
	uint8_t b_length;
	uint8_t b_descriptor_type;
	uint8_t b_descriptor_sub_type;

	uint8_t bm_capabilities;
} __packed;

/* capabilities from 5.2.3.3 */

#define USB_CDC_COMM_FEATURE 0x01
#define USB_CDC_CAP_LINE 0x02
#define USB_CDC_CAP_BRK 0x04
#define USB_CDC_CAP_NOTIFY 0x08

/* "Union Functional Descriptor" from CDC spec 5.2.3.8 */
struct usb_cdc_union_desc {
	uint8_t b_length;
	uint8_t b_descriptor_type;
	uint8_t b_descriptor_sub_type;

	uint8_t b_master_interface0;
	uint8_t b_slave_interface0;
	/* ... and there could be other slave interfaces */
} __packed;

/* "Country Selection Functional Descriptor" from CDC spec 5.2.3.9 */
struct usb_cdc_country_functional_desc {
	uint8_t b_length;
	uint8_t b_descriptor_type;
	uint8_t b_descriptor_sub_type;

	uint8_t i_country_code_rel_date;
	uint16_t w_county_code0;
	/* ... and there can be a lot of country codes */
} __packed;

/*-------------------------------------------------------------------------*/

/*
 * Class-Specific Control Requests (6.2)
 *
 * section 3.6.2.1 table 4 has the ACM profile, for modems.
 * section 3.8.2 table 10 has the ethernet profile.
 *
 * Microsoft's RNDIS stack for Ethernet is a vendor-specific CDC ACM variant,
 * heavily dependent on the encapsulated (proprietary) command mechanism.
 */

#define USB_CDC_SEND_ENCAPSULATED_COMMAND 0x00
#define USB_CDC_GET_ENCAPSULATED_RESPONSE 0x01
#define USB_CDC_REQ_SET_LINE_CODING 0x20
#define USB_CDC_REQ_GET_LINE_CODING 0x21
#define USB_CDC_REQ_SET_CONTROL_LINE_STATE 0x22
#define USB_CDC_REQ_SEND_BREAK 0x23

/* Line Coding Structure from CDC spec 6.2.13 */
struct usb_cdc_line_coding {
	int dw_dte_rate;
	uint8_t b_char_format;
#define USB_CDC_1_STOP_BITS 0
#define USB_CDC_1_5_STOP_BITS 1
#define USB_CDC_2_STOP_BITS 2

	uint8_t b_parity_type;
#define USB_CDC_NO_PARITY 0
#define USB_CDC_ODD_PARITY 1
#define USB_CDC_EVEN_PARITY 2
#define USB_CDC_MARK_PARITY 3
#define USB_CDC_SPACE_PARITY 4

	uint8_t b_data_bits;
} __packed;

/* table 62; bits in multicast filter */
#define USB_CDC_PACKET_TYPE_PROMISCUOUS (1 << 0)
#define USB_CDC_PACKET_TYPE_ALL_MULTICAST (1 << 1) /* no filter */
#define USB_CDC_PACKET_TYPE_DIRECTED (1 << 2)
#define USB_CDC_PACKET_TYPE_BROADCAST (1 << 3)
#define USB_CDC_PACKET_TYPE_MULTICAST (1 << 4) /* filtered */

/*-------------------------------------------------------------------------*/

/*
 * Class-Specific Notifications (6.3) sent by interrupt transfers
 *
 * section 3.8.2 table 11 of the CDC spec lists Ethernet notifications
 * section 3.6.2.1 table 5 specifies ACM notifications, accepted by RNDIS
 * RNDIS also defines its own bit-incompatible notifications
 */

#define USB_CDC_NOTIFY_NETWORK_CONNECTION 0x00
#define USB_CDC_NOTIFY_RESPONSE_AVAILABLE 0x01
#define USB_CDC_NOTIFY_SERIAL_STATE 0x20
#define USB_CDC_NOTIFY_SPEED_CHANGE 0x2a

struct usb_cdc_notification {
	uint8_t bm_request_type;
	uint8_t b_notification_type;
	uint16_t w_value;
	uint16_t w_index;
	uint16_t w_length;
} __packed;

/* utility to simplify dealing with string descriptors */

/**
 * struct usb_string - wraps a C string and its USB id
 * @id:the (nonzero) ID for this string
 * @s:the string, in UTF-8 encoding
 *
 * If you're using usb_gadget_get_string(), use this to wrap a string
 * together with its ID.
 */
struct usb_string {
	uint8_t id;
	const char *s;
};

/* notification endpoint uses smallish and infrequent fixed-size messages */

#define GS_LOG2_NOTIFY_INTERVAL 5 /* 1 << 5 == 32 msec */
#define GS_NOTIFY_MAXPACKET 10 /* notification + 2 bytes */

/* interface and class descriptors: */

#define BCD_CDC 0x0110

//------------------- descriptor set--------------------------------------------
// device descriptor for HighSpeed mode
static ch9_usb_device_descriptor dev_hs_desc = { CH9_USB_DS_DEVICE,
					     CH9_USB_DT_DEVICE,
					     cpu_to_le16(BCD_USB_HS),
					     2,
					     0,
					     0,
					     64,
					     cpu_to_le16(ID_VENDOR),
					     cpu_to_le16(ID_PRODUCT),
					     cpu_to_le16(BCD_DEVICE_HS),
					     1,
					     2,
					     3,
					     1 };

//------------- Start of Super Speed configuration descriptors -----------------
static struct ch9_usb_interface_association_descriptor acm_iad_descriptor = {
	.b_length = sizeof(acm_iad_descriptor),
	.b_descriptor_type = CH9_USB_DT_INTERFACE_ASSOCIATION,

	/* .b_first_interface =   DYNAMIC, */
	.b_interface_count = 2, // control + data
	.b_function_class = CH9_USB_CLASS_CDC,
	.b_function_sub_class = USB_CDC_SUBCLASS_ACM,
	.b_function_protocol = USB_CDC_ACM_PROTO_AT_V25TER,
	/* .i_function =     DYNAMIC */
};

static struct ch9_usb_interface_descriptor acm_control_interface_desc = {
	.b_length = USB_DT_INTERFACE_SIZE,
	.b_descriptor_type = CH9_USB_DT_INTERFACE,
	.b_interface_number = 0,
	.b_alternate_setting = 0,
	.b_num_endpoints = 1,
	.b_interface_class = CH9_USB_CLASS_CDC,
	.b_interface_sub_class = USB_CDC_SUBCLASS_ACM,
	.b_interface_protocol = USB_CDC_ACM_PROTO_AT_V25TER,
	.i_interface = 0,
};

static struct ch9_usb_interface_descriptor acm_data_interface_desc = {
	.b_length = USB_DT_INTERFACE_SIZE,
	.b_descriptor_type = CH9_USB_DT_INTERFACE,
	.b_interface_number = 1,
	.b_alternate_setting = 0,
	.b_num_endpoints = 2,
	.b_interface_class = CH9_USB_CLASS_CDC_DATA,
	.b_interface_sub_class = 0,
	.b_interface_protocol = 0,
	.i_interface = 0,
};

static struct usb_cdc_header_desc acm_header_desc = {
	.b_length = sizeof(acm_header_desc),
	.b_descriptor_type = USB_DT_CS_INTERFACE,
	.b_descriptor_sub_type = USB_CDC_HEADER_TYPE,
	.bcd_cdc = cpu_to_le16(BCD_CDC),
};

static struct usb_cdc_call_mgmt_descriptor acm_call_mgmt_descriptor = {
	.b_length = sizeof(acm_call_mgmt_descriptor),
	.b_descriptor_type = USB_DT_CS_INTERFACE,
	.b_descriptor_sub_type = USB_CDC_CALL_MANAGEMENT_TYPE,
	.bm_capabilities = 3,
	.b_data_interface = 1,
};

static struct usb_cdc_acm_descriptor acm_descriptor = {
	.b_length = sizeof(acm_descriptor),
	.b_descriptor_type = USB_DT_CS_INTERFACE,
	.b_descriptor_sub_type = USB_CDC_ACM_TYPE,
	.bm_capabilities = USB_CDC_CAP_LINE,
};

static struct usb_cdc_union_desc acm_union_desc = {
	.b_length = sizeof(acm_union_desc),
	.b_descriptor_type = USB_DT_CS_INTERFACE,
	.b_descriptor_sub_type = USB_CDC_UNION_TYPE,
	.b_master_interface0 = 0,
	.b_slave_interface0 = 1,
};

/* full speed support: */

static struct ch9_usb_endpoint_descriptor acm_fs_notify_desc = {
	.b_length = USB_DT_ENDPOINT_SIZE,
	.b_descriptor_type = CH9_USB_DT_ENDPOINT,
	.b_endpoint_address = BULK_EP_NOTIFY,
	.bm_attributes = CH9_USB_EP_INTERRUPT,
	.w_max_packet_size = cpu_to_le16(GS_NOTIFY_MAXPACKET),
	.b_interval = 1 << GS_LOG2_NOTIFY_INTERVAL,
};

static struct ch9_usb_endpoint_descriptor acm_fs_in_desc = {
	.b_length = USB_DT_ENDPOINT_SIZE,
	.b_descriptor_type = CH9_USB_DT_ENDPOINT,
	.b_endpoint_address = BULK_EP_IN,
	.bm_attributes = CH9_USB_EP_BULK,
	.w_max_packet_size = cpu_to_le16(64),
};

static struct ch9_usb_endpoint_descriptor acm_fs_out_desc = {
	.b_length = USB_DT_ENDPOINT_SIZE,
	.b_descriptor_type = CH9_USB_DT_ENDPOINT,
	.b_endpoint_address = BULK_EP_OUT,
	.bm_attributes = CH9_USB_EP_BULK,
	.w_max_packet_size = cpu_to_le16(64),
};

static void *descriptors_fs[] = {
	//&acm_iad_descriptor,
	&acm_control_interface_desc,
	&acm_header_desc,
	&acm_descriptor,
	&acm_union_desc,
	&acm_call_mgmt_descriptor,
	&acm_fs_notify_desc,
	&acm_data_interface_desc,
	&acm_fs_out_desc,
	&acm_fs_in_desc,
	NULL,
};

/* high speed support: */
static struct ch9_usb_endpoint_descriptor acm_hs_notify_desc = {
	.b_length = USB_DT_ENDPOINT_SIZE,
	.b_descriptor_type = CH9_USB_DT_ENDPOINT,
	.b_endpoint_address = BULK_EP_NOTIFY,
	.bm_attributes = CH9_USB_EP_INTERRUPT,
	.w_max_packet_size = cpu_to_le16(GS_NOTIFY_MAXPACKET),
	.b_interval = GS_LOG2_NOTIFY_INTERVAL + 4,
};

static struct ch9_usb_endpoint_descriptor acm_hs_in_desc = {
	.b_length = USB_DT_ENDPOINT_SIZE,
	.b_descriptor_type = CH9_USB_DT_ENDPOINT,
	.b_endpoint_address = BULK_EP_IN,
	.bm_attributes = CH9_USB_EP_BULK,
	.w_max_packet_size = cpu_to_le16(512),
};

static struct ch9_usb_endpoint_descriptor acm_hs_out_desc = {
	.b_length = USB_DT_ENDPOINT_SIZE,
	.b_descriptor_type = CH9_USB_DT_ENDPOINT,
	.b_endpoint_address = BULK_EP_OUT,
	.bm_attributes = CH9_USB_EP_BULK,
	.w_max_packet_size = cpu_to_le16(512),
};

static void *descriptors_hs[] = {
	//&acm_iad_descriptor,
	&acm_control_interface_desc,
	&acm_header_desc,
	&acm_descriptor,
	&acm_union_desc,
	&acm_call_mgmt_descriptor,
	&acm_hs_notify_desc,
	&acm_data_interface_desc,
	&acm_hs_out_desc,
	&acm_hs_in_desc,
	NULL,
};

/* string descriptors: */

#define ACM_CTRL_IDX 0
#define ACM_DATA_IDX 1
#define ACM_IAD_IDX 2

// language descriptor for english
static uint8_t language_desc[] = { 4, CH9_USB_DT_STRING, 0x09, 0x04 };

static ch9_usb_configuration_descriptor conf_desc = {
	.b_length = CH9_USB_DS_CONFIGURATION,
	.b_descriptor_type = CH9_USB_DT_CONFIGURATION,
	.w_total_length = 0x0000, // will be filed in get_descriptor request
	.b_num_interfaces = 2,
	.b_configuration_value = 1,
	.i_configuration = 0,
	.bm_attributes = CH9_USB_CONFIG_RESERVED | CH9_USB_CONFIG_BUS_POWERED,
	.b_max_power = 0xFA
};

//-------------- BOS descriptor set start --------------------------------------

static ch9_usb_bos_descriptor bos_desc = {
	CH9_USB_DS_BOS, CH9_USB_DT_BOS,
	cpu_to_le16(CH9_USB_DS_BOS + CH9_USB_DS_DEVICE_CAPABILITY_20), 1
};

static ch9_usb_capability_descriptor capability_ext_desc = {
	CH9_USB_DS_DEVICE_CAPABILITY_20, CH9_USB_DT_DEVICE_CAPABILITY,
	CH9_USB_DCT_USB20_EXTENSION, cpu_to_le32(CH9_USB_USB20_EXT_LPM_SUPPORT)
};
//-------------- BOS descriptor set end ----------------------------------------

static ch9_usb_device_qualifier_descriptor qualifier_desc = {
	CH9_USB_DS_DEVICE_QUALIFIER,
	CH9_USB_DT_DEVICE_QUALIFIER,
	cpu_to_le16(0x0200),
	0x00,
	0x00,
	0x00,
	64,
	0x01,
	0x00
};

struct f_acm {
	//struct gserial          port;
	uint8_t ctrl_id, data_id;
	uint8_t port_num;

	uint8_t pending;

	/* lock is mostly for pending and notify_req ... they get accessed
	 * by callbacks both from tty (open/close/break) under its spinlock,
	 * and notify_req.complete() which can't use that lock.
	 */
	//spinlock_t          lock;

	//    struct usb_ep           *notify;
	//    struct usb_request      *notify_req;

	struct usb_cdc_line_coding port_line_coding; /* 8-N-1 etc */

	/* SetControlLineState request -- CDC 1.1 section 6.2.14 (INPUT) */
	uint16_t port_handshake_bits;
#define ACM_CTRL_RTS (1 << 1) /* unused with full duplex */
#define ACM_CTRL_DTR (1 << 0) /* host is ready for data r/w */

	/* SerialState notification -- CDC 1.1 section 6.3.5 (OUTPUT) */
	uint16_t serial_state;
#define ACM_CTRL_OVERRUN (1 << 6)
#define ACM_CTRL_PARITY (1 << 5)
#define ACM_CTRL_FRAMING (1 << 4)
#define ACM_CTRL_RI (1 << 3)
#define ACM_CTRL_BRK (1 << 2)
#define ACM_CTRL_DSR (1 << 1)
#define ACM_CTRL_DCD (1 << 0)
};

#endif /*_USB_TTY_H*/
