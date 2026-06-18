#include "drv_usb.h"

#define USB_ID                           0x00
/**
  * @brief  this function config gpio.
  * @param  none
  * @retval none
  */

void usb_dc_low_level_init(void)
{
    crm_usb_clock_source_select(CRM_USB_CLOCK_SOURCE_HICK);
    crm_periph_clock_enable(CRM_ACC_PERIPH_CLOCK, TRUE);

    acc_write_c1(7980);
    acc_write_c2(8000);
    acc_write_c3(8020);

    acc_calibration_mode_enable(ACC_CAL_HICKTRIM, TRUE);

    crm_periph_clock_enable(CRM_USB_PERIPH_CLOCK, TRUE);

    nvic_irq_enable(USBFS_L_CAN1_RX0_IRQn, 0, 0);
}

void USBFS_L_CAN1_RX0_IRQHandler(void) {
  extern void USBD_IRQHandler(uint8_t busid);
  USBD_IRQHandler(USB_ID);
}

