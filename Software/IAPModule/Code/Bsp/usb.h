#ifndef __USB_H__
#define __USB_H__


#include "main.h"
#include "usbd_core.h"
#include "cdc_class.h"
#include "cdc_desc.h"
#include "usbd_int.h"
#include "chry_ringbuffer.h"


extern usbd_core_type usb_core_dev;
extern chry_ringbuffer_t usb_rx_ringbuffer;

void USB_Init(void);
void USB_RX_Handle(void);
void USBFS_L_CAN1_RX0_IRQHandler(void);
    
#endif // 


