#ifndef USB2UART_H
#define USB2UART_H

#include "chry_ringbuffer.h"
#include "usbd_core.h"
#include "usbd_cdc.h"
#include "config.h"

#define USB2UART_PORT USART2
#define USB2UART_IRQn USART2_IRQn
#define USART_BUFFER_SIZE (4096)

typedef struct {
    uint16_t Rx_RemainLen;
    uint16_t Rx_DealPtr; // Pointer to serial x data receive buffer processing
    uint16_t UARTx_Rx_DMACurCount; // Serial port x receive dma current counter
    uint16_t UARTx_Rx_DMALastCount; // Serial port x receive dma last value counter
    uint32_t g_uart_tx_transfer_length;
    uint8_t usbrx_idle_flag;
    uint8_t usbtx_idle_flag;
    uint8_t uarttx_idle_flag;
    uint8_t *usart_rx_buffer;
    uint8_t *usbrx_ringbuffer;
    uint8_t *usbrx_tmpbuffer;
    uint8_t *usbtx_tmpbuffer;
    chry_ringbuffer_t *g_usbrx;

    struct cdc_line_coding g_cdc_lincoding;
    uint8_t config_uart;
    uint8_t config_uart_transfer;
} usb2uart_state_t;

extern usb2uart_state_t usart1_state;

void usart1_preinit(void);
void usart3_preinit(void);
void chry_dap_usb2uart_rx(usb2uart_state_t *usart_state);
void chry_dap_usb2uart_uart_config_callback(usb2uart_state_t *usart_state);
void chry_dap_usb2uart_uart_send_bydma(usb2uart_state_t *usart_state, uint8_t *data, uint16_t len);
void chry_dap_usb2uart_uart_send_complete(usb2uart_state_t *usart_state);

#endif //USB2UART_H
