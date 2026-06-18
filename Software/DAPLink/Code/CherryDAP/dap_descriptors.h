#ifndef __DAP_DESCRIPTORS_H
#define __DAP_DESCRIPTORS_H

#include <stdio.h>
#include "usbd_core.h"
#include "usbd_cdc.h"
//#include "usbd_msc.h"
#include "chry_ringbuffer.h"
#include "DAP_config.h"
#include "DAP.h"

#define DAP_IN_EP  0x81
#define DAP_OUT_EP 0x01

#define CDC1_IN_EP  0x82
#define CDC1_OUT_EP 0x02
#define CDC1_INT_EP 0x88

#define CDC2_IN_EP  0x83
#define CDC2_OUT_EP 0x03
#define CDC2_INT_EP 0x89

#define USBD_VID           0x0D28
#define USBD_PID           0x0204
#define USBD_MAX_POWER     500
#define USBD_LANGID_STRING 1033

#define DAP_BUS_ID 0

#define CMSIS_DAP_INTERFACE_SIZE (9 + 7 + 7)
#define USB_CONFIG_SIZE (9 + CMSIS_DAP_INTERFACE_SIZE + CDC_ACM_DESCRIPTOR_LEN * 1)
#define INTF_NUM        3

#ifdef CONFIG_USB_HS
#if DAP_PACKET_SIZE != 512
#error "DAP_PACKET_SIZE must be 512 in hs"
#endif
#else
#if DAP_PACKET_SIZE != 64
#error "DAP_PACKET_SIZE must be 64 in fs"
#endif
#endif

#define USBD_WINUSB_VENDOR_CODE 0x20

#define USBD_WEBUSB_ENABLE 0
#define USBD_BULK_ENABLE   1
#define USBD_WINUSB_ENABLE 1

/* WinUSB Microsoft OS 2.0 descriptor sizes */
#define WINUSB_DESCRIPTOR_SET_HEADER_SIZE  10
#define WINUSB_FUNCTION_SUBSET_HEADER_SIZE 8
#define WINUSB_FEATURE_COMPATIBLE_ID_SIZE  20

#define FUNCTION_SUBSET_LEN                160
#define DEVICE_INTERFACE_GUIDS_FEATURE_LEN 132

#define USBD_WINUSB_DESC_SET_LEN (WINUSB_DESCRIPTOR_SET_HEADER_SIZE + USBD_WEBUSB_ENABLE * FUNCTION_SUBSET_LEN + USBD_BULK_ENABLE * FUNCTION_SUBSET_LEN)

#define USBD_NUM_DEV_CAPABILITIES (USBD_WEBUSB_ENABLE + USBD_WINUSB_ENABLE)

#define USBD_WEBUSB_DESC_LEN 24
#define USBD_WINUSB_DESC_LEN 28

#define USBD_BOS_WTOTALLENGTH (0x05 +                                      \
                               USBD_WEBUSB_DESC_LEN * USBD_WEBUSB_ENABLE + \
                               USBD_WINUSB_DESC_LEN * USBD_WINUSB_ENABLE)

#ifdef CONFIG_USBDEV_ADVANCE_DESC
extern const struct usb_descriptor cmsisdap_descriptor;
extern uint8_t string2_descriptor[];
extern uint8_t string3_descriptor[];
#else
extern struct usb_msosv2_descriptor msosv2_desc;
extern struct usb_bos_descriptor bos_desc;
extern uint8_t cmsisdap_descriptor[];
#endif // CONFIG_USBDEV_ADVANCE_DESC

#endif // __DAP_DESCRIPTORS_H
