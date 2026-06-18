#ifndef __UARTX_H__
#define __UARTX_H__

#include <stdint.h>

#define UART_STRUCT_NUM         1

#define UART_IAP                USART1

void UARTx_Init(void);

uint16_t Cal_CRC16(uint8_t* data, int offset, uint32_t size);
uint32_t Cal_ADD8(uint8_t* data, int offset, uint32_t size);



#endif  // __UARTX_H__