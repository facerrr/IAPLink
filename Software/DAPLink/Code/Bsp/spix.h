#ifndef __SPIX_H__
#define __SPIX_H__

#include <stdint.h>

#define DMA_ENABLE  1

void SPIx_Init(void);
void spi_write_8bit(const unsigned char data);
void spi_write_16bit(const unsigned short int data);
void spi_send_by_dma(uint8_t *data, uint16_t len);

#endif // __SPIX_H__