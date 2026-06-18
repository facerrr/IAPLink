#ifndef DAP_MAIN_H
#define DAP_MAIN_H

#include "swd_host.h"
#include "dap_descriptors.h"
#include "drv_usb2uart.h"

extern uint32_t IC_UID;

void chry_dap_init(void);
void chry_dap_handle(void);
void chry_dap_usb2uart_handle(usb2uart_state_t *usart_state);
void chry_dap_hid_handle(void);

#endif // DAP_MAIN_H