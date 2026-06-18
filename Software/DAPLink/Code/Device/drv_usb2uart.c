#include "drv_usb2uart.h"

static USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t usart1_rx_buffer[USART_BUFFER_SIZE];
static USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t usbrx_ringbuffer_usart1[USART_BUFFER_SIZE];
static USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t usbrx_tmpbuffer_usart1[512];
static USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t usbtx_tmpbuffer_usart1[512];
static USB_NOCACHE_RAM_SECTION chry_ringbuffer_t g_usbrx_usart1;

usb2uart_state_t usart1_state;

void DMA1_Channel2_IRQHandler(void)
{
    if(DMA1->sts & DMA1_FDT2_FLAG)
    {
        DMA1->clr = DMA1_FDT2_FLAG;
        chry_dap_usb2uart_uart_send_complete(&usart1_state);
    }
}

void usart1_preinit(void)
{
    gpio_init_type gpio_init_struct;

    chry_ringbuffer_init(&g_usbrx_usart1, usbrx_ringbuffer_usart1, USART_BUFFER_SIZE);
    usart1_state.usart_rx_buffer = usart1_rx_buffer;
    usart1_state.g_usbrx = &g_usbrx_usart1;
    usart1_state.usbrx_tmpbuffer = usbrx_tmpbuffer_usart1;
    usart1_state.usbtx_tmpbuffer = usbtx_tmpbuffer_usart1;
    
    /* enable the usart1 and gpio clock */
    crm_periph_clock_enable(CRM_USART1_PERIPH_CLOCK, TRUE);
    crm_periph_clock_enable(CRM_GPIOA_PERIPH_CLOCK, TRUE);
    crm_periph_clock_enable(CRM_IOMUX_PERIPH_CLOCK, TRUE);

    gpio_default_para_init(&gpio_init_struct);
    /* configure the usart1 rx/tx pin */
    gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_MODERATE;
    gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
    gpio_init_struct.gpio_mode = GPIO_MODE_MUX;
    gpio_init_struct.gpio_pins = GPIO_PINS_9;
    gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
    gpio_init(GPIOA, &gpio_init_struct);

    /* configure the RX pin */
    gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_MODERATE;
    gpio_init_struct.gpio_out_type  = GPIO_OUTPUT_PUSH_PULL;
    gpio_init_struct.gpio_mode = GPIO_MODE_INPUT;
    gpio_init_struct.gpio_pins = GPIO_PINS_10;
    gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
    gpio_init(GPIOA, &gpio_init_struct);

    /* configure usart1 param */
    usart_init(USART1, 115200, USART_DATA_8BITS, USART_STOP_1_BIT);
    usart_transmitter_enable(USART1, TRUE);
    usart_receiver_enable(USART1, TRUE);
    usart_dma_transmitter_enable(USART1, TRUE);
    usart_dma_receiver_enable(USART1, TRUE);
    usart_enable(USART1, TRUE);

    dma_init_type dma_init_struct;
    /* enable dma1 clock */
    crm_periph_clock_enable(CRM_DMA1_PERIPH_CLOCK, TRUE);
//    dmamux_enable(DMA1, TRUE);
    
    /* dma1 channel for usart1 rx configuration */
    dma_reset(DMA1_CHANNEL1);
    dma_default_para_init(&dma_init_struct);
    dma_init_struct.buffer_size = USART_BUFFER_SIZE;
    dma_init_struct.direction = DMA_DIR_PERIPHERAL_TO_MEMORY;
    dma_init_struct.memory_base_addr = (uint32_t) usart1_state.usart_rx_buffer;
    dma_init_struct.memory_data_width = DMA_MEMORY_DATA_WIDTH_BYTE;
    dma_init_struct.memory_inc_enable = TRUE;
    dma_init_struct.peripheral_base_addr = (uint32_t) &USART1->dt;
    dma_init_struct.peripheral_data_width = DMA_PERIPHERAL_DATA_WIDTH_BYTE;
    dma_init_struct.peripheral_inc_enable = FALSE;
    dma_init_struct.priority = DMA_PRIORITY_MEDIUM;
    dma_init_struct.loop_mode_enable = TRUE;
    dma_init(DMA1_CHANNEL1, &dma_init_struct);
    dma_flexible_config(DMA1, FLEX_CHANNEL1, DMA_FLEXIBLE_UART1_RX);
    dma_channel_enable(DMA1_CHANNEL1, TRUE);
    
    /* dma1 channel for usart2 tx configuration */
    dma_reset(DMA1_CHANNEL2);
    dma_default_para_init(&dma_init_struct);
    dma_init_struct.buffer_size = USART_BUFFER_SIZE;
    dma_init_struct.direction = DMA_DIR_MEMORY_TO_PERIPHERAL;
    dma_init_struct.memory_base_addr = (uint32_t) 0;
    dma_init_struct.memory_data_width = DMA_MEMORY_DATA_WIDTH_BYTE;
    dma_init_struct.memory_inc_enable = TRUE;
    dma_init_struct.peripheral_base_addr = (uint32_t) &USART1->dt;
    dma_init_struct.peripheral_data_width = DMA_PERIPHERAL_DATA_WIDTH_BYTE;
    dma_init_struct.peripheral_inc_enable = FALSE;
    dma_init_struct.priority = DMA_PRIORITY_HIGH;
    dma_init_struct.loop_mode_enable = FALSE;
    dma_init(DMA1_CHANNEL2, &dma_init_struct);
    dma_flexible_config(DMA1, FLEX_CHANNEL2, DMA_FLEXIBLE_UART1_TX);
    /* enable transfer full data intterrupt */
    dma_interrupt_enable(DMA1_CHANNEL2, DMA_FDT_INT, TRUE);
    /* dma1 channel4 interrupt nvic init */
    nvic_irq_enable(DMA1_Channel2_IRQn, 0, 0);
}


void chry_dap_usb2uart_uart_config_callback(usb2uart_state_t *usart_state)
{
    usart_stop_bit_num_type usart_stop_bit = USART_STOP_1_BIT;
    usart_data_bit_num_type usart_data_bit = USART_DATA_8BITS;
    usart_parity_selection_type usart_parity_select = USART_PARITY_NONE;

    /* stop bit */
    switch (usart_state->g_cdc_lincoding.bCharFormat)
    {
        case 0x0:
            usart_stop_bit = USART_STOP_1_BIT;
            break;
        /* to be used when transmitting and receiving data in smartcard mode */
        case 0x1:
            usart_stop_bit = USART_STOP_1_5_BIT;
            break;
        case 0x2:
            usart_stop_bit = USART_STOP_2_BIT;
            break;
        default:
            break;
    }
    
    switch (usart_state->g_cdc_lincoding.bDataBits)
    {
        /* hardware usart not support data bits for 5/6 */
        case 0x5:
        case 0x6:
        case 0x7:
            break;
        case 0x8:
            usart_data_bit = USART_DATA_8BITS;
            break;
        /* hardware usart not support data bits for 16 */
        case 0x10:
            break;
        default:
            break;
    }
    /* parity */
    switch (usart_state->g_cdc_lincoding.bParityType)
    {
        case 0x0:
            usart_parity_select = USART_PARITY_NONE;
            break;
        case 0x1:
            usart_parity_select = USART_PARITY_ODD;
            break;
        case 0x2:
            usart_parity_select = USART_PARITY_EVEN;
            break;
        /* hardware usart not support partiy for mark and space */
        case 0x3:
        case 0x4:
            break;
        default:
            break;
    }

    if (usart_parity_select == USART_PARITY_NONE)
    {
        usart_data_bit = USART_DATA_8BITS;
    }
    else
    {
        usart_data_bit = USART_DATA_9BITS;
    }
    
    usart_init(USART1, usart_state->g_cdc_lincoding.dwDTERate, usart_data_bit, usart_stop_bit);
    usart_parity_selection_config(USART1, usart_parity_select);

    usart_state->UARTx_Rx_DMACurCount = 0;
    usart_state->UARTx_Rx_DMALastCount = 0;
    usart_state->Rx_RemainLen = 0;
    usart_state->Rx_DealPtr = 0;
    
    DMA1_CHANNEL1->ctrl_bit.chen = FALSE;
    DMA1_CHANNEL1->dtcnt = USART_BUFFER_SIZE;
    DMA1_CHANNEL1->ctrl_bit.chen = TRUE;
}

void chry_dap_usb2uart_uart_send_bydma(usb2uart_state_t *usart_state, uint8_t *data, uint16_t len)
{
    if (len <= 0) {
        return;
    }
    usart_state->g_uart_tx_transfer_length = len;
    
    DMA1_CHANNEL2->ctrl_bit.chen = FALSE;
    DMA1_CHANNEL2->maddr = (uint32_t)data;
    DMA1_CHANNEL2->dtcnt = usart_state->g_uart_tx_transfer_length;
    DMA1_CHANNEL2->ctrl_bit.chen = TRUE;
}

void chry_dap_usb2uart_rx(usb2uart_state_t *usart_state)
{
    uint16_t temp16;

    // 获得DMA当前位置
    usart_state->UARTx_Rx_DMACurCount = DMA1_CHANNEL1->dtcnt;
    
    // 当前位置和上次位置不一样
    if( usart_state->UARTx_Rx_DMALastCount != usart_state->UARTx_Rx_DMACurCount )
    {
        if( usart_state->UARTx_Rx_DMALastCount > usart_state->UARTx_Rx_DMACurCount )
        {
            // 没有到数组末的情况
            temp16 = usart_state->UARTx_Rx_DMALastCount - usart_state->UARTx_Rx_DMACurCount;
        }
        else
        {
            // 到数组末的情况, 要加上数组末尾没发送完的
            temp16 = USART_BUFFER_SIZE - usart_state->UARTx_Rx_DMACurCount;
            temp16 += usart_state->UARTx_Rx_DMALastCount;
        }
        usart_state->UARTx_Rx_DMALastCount = usart_state->UARTx_Rx_DMACurCount;
        // 防止越界,如果产生越界需要加大缓冲区
        NVIC_DisableIRQ(USBFS_L_CAN1_RX0_IRQn);
        
        if( ( usart_state->Rx_RemainLen + temp16 ) > USART_BUFFER_SIZE )
        {
            // while(1){};
        }
        else
        {
            usart_state->Rx_RemainLen += temp16;
        }
        NVIC_EnableIRQ(USBFS_L_CAN1_RX0_IRQn);
    }
}

/* called by user */
void chry_dap_usb2uart_uart_send_complete(usb2uart_state_t *usart_state)
{
    uint8_t *buffer;

    chry_ringbuffer_linear_read_done(usart_state->g_usbrx, usart_state->g_uart_tx_transfer_length);

    if (chry_ringbuffer_get_used(usart_state->g_usbrx)) {
        buffer = chry_ringbuffer_linear_read_setup(usart_state->g_usbrx, &usart_state->g_uart_tx_transfer_length);
        chry_dap_usb2uart_uart_send_bydma(usart_state, buffer, usart_state->g_uart_tx_transfer_length);
    } else {
        usart_state->uarttx_idle_flag = 1;
    }
}