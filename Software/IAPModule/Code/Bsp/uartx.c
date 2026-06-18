#include "main.h"

/**
 * @brief  UART初始化
 * @param  None
 * @retval None
 */
void UARTx_Init(void){
    // gpio
    gpio_init_type gpio_init_struct;
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
    usart_init(UART_IAP, 115200, USART_DATA_8BITS, USART_STOP_1_BIT);
    usart_transmitter_enable(UART_IAP, TRUE);
    usart_receiver_enable(UART_IAP, TRUE);

    nvic_irq_enable(USART1_IRQn, 15, 0);
    usart_interrupt_enable(UART_IAP, USART_RDBF_INT, TRUE);

    usart_enable(UART_IAP, TRUE);
}


/**
 * @brief  CRC16计算
 * @param  data: 数据指针 offset: 偏移 size: 数据长度
 * @retval None
 */
uint16_t Cal_CRC16(uint8_t* data, int offset, uint32_t size){
    uint8_t u8Cnt;
    uint16_t u16CrcResult = 0xA28C;
    uint32_t u32Offset = (uint32_t)offset;
    while(size != 0){
        u16CrcResult ^= data[u32Offset++];
        for(u8Cnt = 0; u8Cnt < 8; u8Cnt++){
            if((u16CrcResult & 0x1) == 0x1){
                u16CrcResult >>= 1;
                u16CrcResult ^= 0x8408;
            }else{
                u16CrcResult >>= 1;
            }
        }
        size--;
    }
    u16CrcResult = (uint16_t)(~u16CrcResult);
    return u16CrcResult;
}


/**
 * @brief  ADD8计算
 * @param  data: 数据指针 offset: 偏移 size: 数据长度
 * @retval None
 */
uint32_t Cal_ADD8(uint8_t* data, int offset, uint32_t size){
    uint32_t checksum = 0;
    for (uint32_t i = 0; i < size; ++i) {
        checksum += data[offset + i];
    }
    return checksum;
}



void USART1_IRQHandler(void){
    if(usart_interrupt_flag_get(UART_IAP, USART_RDBF_FLAG) != RESET){
        usart_flag_clear(UART_IAP, USART_RDBF_FLAG);
        uint8_t data = usart_data_receive(UART_IAP);
        IAPRecv_Func(data);
    }
}


