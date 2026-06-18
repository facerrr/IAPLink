#include "usb.h"

usbd_core_type usb_core_dev;
chry_ringbuffer_t usb_rx_ringbuffer;
uint8_t u8CdcRxBuffer[USB_BUFFER_SIZE];
uint32_t u32IapAddr;

uint8_t UpgradeFlag = 0;

void USB_Init(void)
{
    chry_ringbuffer_init(&usb_rx_ringbuffer, u8CdcRxBuffer, USB_BUFFER_SIZE);
    //    crm_usb_clock_div_set(CRM_USB_DIV_4);
    crm_usb_clock_source_select(CRM_USB_CLOCK_SOURCE_HICK);
    crm_periph_clock_enable(CRM_ACC_PERIPH_CLOCK, TRUE);

    acc_write_c1(7980);
    acc_write_c2(8000);
    acc_write_c3(8020);

    acc_calibration_mode_enable(ACC_CAL_HICKTRIM, TRUE);

    crm_periph_clock_enable(CRM_USB_PERIPH_CLOCK, TRUE);

    nvic_priority_group_config(NVIC_PRIORITY_GROUP_4);
    nvic_irq_enable(USBFS_L_CAN1_RX0_IRQn, 0, 0);

    usbd_core_init(&usb_core_dev, USB, &cdc_class_handler, &cdc_desc_handler, 0);

    usbd_connect(&usb_core_dev);
}


void USBFS_L_CAN1_RX0_IRQHandler(void)
{
    usbd_irq_handler(&usb_core_dev);
    USB_RX_Handle();
}

void USB_RX_Handle(void)
{
    uint16_t usb_rx_len = 0;
    usbd_core_type *pudev = (usbd_core_type *)(&usb_core_dev);
    cdc_struct_type *pcdc = (cdc_struct_type *)pudev->class_handler->pdata;
    if(pcdc->g_rx_completed != 0){
        pcdc->g_rx_completed = 0;
        usb_rx_len = pcdc->g_rxlen;
        chry_ringbuffer_overwrite(&usb_rx_ringbuffer, pcdc->g_rx_buff, usb_rx_len);
        usbd_ept_recv(pudev, USBD_CDC_BULK_OUT_EPT, pcdc->g_rx_buff, USBD_CDC_OUT_MAXPACKET_SIZE);
    }
}


void usb_delay_ms(uint32_t ms)
{
    delay_ms(ms);
}

void usb_delay_us(uint32_t us)
{
    delay_us(us);
}

