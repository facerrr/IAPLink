#include "main.h"

void SPIx_Init(void){    
    spi_init_type spi_init_struct;
    spi_default_para_init(&spi_init_struct);

    spi_init_struct.transmission_mode = SPI_TRANSMIT_HALF_DUPLEX_TX;
    spi_init_struct.master_slave_mode = SPI_MODE_MASTER;
    
    spi_init_struct.first_bit_transmission = SPI_FIRST_BIT_MSB;
    spi_init_struct.mclk_freq_division = SPI_MCLK_DIV_4;

    spi_init_struct.clock_polarity = SPI_CLOCK_POLARITY_HIGH;
    spi_init_struct.frame_bit_num = SPI_FRAME_8BIT;
    spi_init_struct.clock_phase = SPI_CLOCK_PHASE_2EDGE;

    spi_init_struct.cs_mode_selection = SPI_CS_SOFTWARE_MODE;
    spi_init(SPI3, &spi_init_struct);
    
    #if DMA_ENABLE
    dma_init_type dma_init_struct;
    dma_reset(DMA2_CHANNEL1);
    dma_default_para_init(&dma_init_struct);
    dma_init_struct.direction = DMA_DIR_MEMORY_TO_PERIPHERAL;
    dma_init_struct.memory_base_addr = (uint32_t) 0;
    dma_init_struct.memory_data_width = DMA_MEMORY_DATA_WIDTH_BYTE;
    dma_init_struct.memory_inc_enable = TRUE;
    dma_init_struct.peripheral_base_addr = (uint32_t)&SPI3->dt;
    dma_init_struct.peripheral_data_width = DMA_PERIPHERAL_DATA_WIDTH_BYTE;
    dma_init_struct.peripheral_inc_enable = FALSE;
    dma_init_struct.priority = DMA_PRIORITY_VERY_HIGH;
    dma_init_struct.loop_mode_enable = FALSE;
    dma_init(DMA2_CHANNEL1, &dma_init_struct);
    
    dma_flexible_config(DMA2, FLEX_CHANNEL1, DMA_FLEXIBLE_SPI3_TX);
    DMA2_CHANNEL1->ctrl_bit.chen = FALSE;
    
    spi_i2s_dma_transmitter_enable(SPI3, FALSE);
    #endif
    spi_enable(SPI3, TRUE);
}


void spi_write_8bit(const unsigned char data){

    #if DMA_ENABLE
    static uint8_t temp_buffer;
    temp_buffer = data;
    DMA2_CHANNEL1->maddr = (uint32_t)&temp_buffer;
    DMA2_CHANNEL1->dtcnt = 1;
    DMA2_CHANNEL1->ctrl_bit.chen = TRUE;
    SPI3->ctrl2_bit.dmaten = TRUE;
    while((DMA2->sts & DMA2_FDT1_FLAG) == RESET);
    DMA2->clr = DMA2_FDT1_FLAG;
    delay_us(2);
    DMA2_CHANNEL1->ctrl_bit.chen = FALSE;
    SPI3->ctrl2_bit.dmaten = FALSE;
    #else
    SPI3->dt = data;
    while(spi_i2s_flag_get(SPI3, SPI_I2S_BF_FLAG) == SET);
    SPI3->dt;
    #endif
}


void spi_write_16bit (const unsigned short int data){
    
    #if DMA_ENABLE
    static uint8_t temp_buffer[2];
    temp_buffer[0] = (uint8_t)((data & 0xFF00) >> 8); // 高8位
    temp_buffer[1] = (uint8_t)(data & 0x00FF);        // 低8位
    DMA2_CHANNEL1->maddr = (uint32_t)temp_buffer;
    DMA2_CHANNEL1->dtcnt = 2;
    DMA2_CHANNEL1->ctrl_bit.chen = TRUE;
    SPI3->ctrl2_bit.dmaten = TRUE;
    while((DMA2->sts & DMA2_FDT1_FLAG) == RESET);
    DMA2->clr = DMA2_FDT1_FLAG;
    delay_us(2);
    DMA2_CHANNEL1->ctrl_bit.chen = FALSE;
    SPI3->ctrl2_bit.dmaten = FALSE;
    #else
    uint8_t tmp = (uint8_t)((data & 0xFF00)>>8);
    spi_write_8bit(tmp);
    tmp = (uint8_t)(data & 0x00FF);
    spi_write_8bit(tmp);
    #endif
}


void spi_send_by_dma(uint8_t *data, uint16_t len){
    if (len <= 0) {
        return;
    }
    DMA2_CHANNEL1->maddr = (uint32_t)data;
    DMA2_CHANNEL1->dtcnt = len;
    DMA2_CHANNEL1->ctrl_bit.chen = TRUE;
    SPI3->ctrl2_bit.dmaten = TRUE;
    while((DMA2->sts & DMA2_FDT1_FLAG) == RESET);
    DMA2->clr = DMA2_FDT1_FLAG;
    delay_us(2);
    DMA2_CHANNEL1->ctrl_bit.chen = FALSE;
    SPI3->ctrl2_bit.dmaten = FALSE;
}
