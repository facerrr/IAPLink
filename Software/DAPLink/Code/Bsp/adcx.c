#include "config.h"

strADC ADCs;
void DMA_Init(void);
void ADC_Result_Process(void);

/**
 * @brief  ADC初始化
 * @param  None
 * @retval None
 */
void ADCx_Init(void){
    // dma
    DMA_Init();
    
    // gpio
    gpio_init_type gpio_init_struct;
    gpio_default_para_init(&gpio_init_struct);
    
    gpio_init_struct.gpio_mode = GPIO_MODE_ANALOG;
    gpio_init_struct.gpio_pins = GPIO_PINS_0;
    gpio_init(GPIOB, &gpio_init_struct);
    
    gpio_init_struct.gpio_mode = GPIO_MODE_ANALOG;
    gpio_init_struct.gpio_pins = GPIO_PINS_1;
    gpio_init(GPIOB, &gpio_init_struct);

    // adc
    adc_base_config_type adc_base_struct;
    crm_adc_clock_div_set(CRM_ADC_DIV_6);
    adc_combine_mode_select(ADC_INDEPENDENT_MODE);

    adc_base_default_para_init(&adc_base_struct);
    adc_base_struct.sequence_mode = TRUE;
    adc_base_struct.data_align = ADC_RIGHT_ALIGNMENT;
    adc_base_struct.ordinary_channel_length = 2;
    adc_base_config(ADC1, &adc_base_struct);

    adc_ordinary_channel_set(ADC1, ADC_CHANNEL_8, 1, ADC_SAMPLETIME_28_5);
    adc_ordinary_channel_set(ADC1, ADC_CHANNEL_9, 2, ADC_SAMPLETIME_28_5);
    adc_ordinary_conversion_trigger_set(ADC1, ADC12_ORDINARY_TRIG_TMR1CH1, TRUE);
    
    adc_dma_mode_enable(ADC1, TRUE);
    adc_enable(ADC1, TRUE);

    adc_calibration_init(ADC1);
    while(adc_calibration_init_status_get(ADC1));
    adc_calibration_start(ADC1);
    while(adc_calibration_status_get(ADC1));
}

/**
 * @brief  DMA初始化
 * @param  None
 * @retval None
 */
void DMA_Init(void){
    dma_init_type dma_init_struct;

    dma_reset(DMA1_CHANNEL3);
    dma_default_para_init(&dma_init_struct);
    dma_init_struct.buffer_size = ADC_NUM;
    dma_init_struct.direction = DMA_DIR_PERIPHERAL_TO_MEMORY;
    dma_init_struct.memory_base_addr = (uint32_t)ADCs.adc_result;

    dma_init_struct.memory_data_width = DMA_MEMORY_DATA_WIDTH_HALFWORD;
    dma_init_struct.memory_inc_enable = TRUE;
    dma_init_struct.peripheral_base_addr = (uint32_t) & (ADC1->odt);
    dma_init_struct.peripheral_data_width = DMA_PERIPHERAL_DATA_WIDTH_HALFWORD;
    dma_init_struct.peripheral_inc_enable = FALSE;
    dma_init_struct.priority = DMA_PRIORITY_HIGH;
    dma_init_struct.loop_mode_enable = TRUE;
    dma_init(DMA1_CHANNEL3, &dma_init_struct);
    dma_flexible_config(DMA1, FLEX_CHANNEL3, DMA_FLEXIBLE_ADC1);

    dma_interrupt_enable(DMA1_CHANNEL3, DMA_FDT_INT, TRUE);
    nvic_irq_enable(DMA1_Channel3_IRQn, 0, 0);
    
    dma_channel_enable(DMA1_CHANNEL3, TRUE);
}

/**
 * @brief  ADC结果处理
 * @param  None
 * @retval None
 */
void ADC_Result_Process(void){
    for (unsigned char i = 0; i < ADC_NUM; i++){
        UTILS_LP_MOVING_AVG_APPROX(ADCs.adc_average[i], ADCs.adc_result[i], 3);
    }
}


/**
 * @brief  ADC采样
 * @param  None
 * @retval None
 */
void ADCx_Func(void){
    if(ADCs.u8ADCSampleCMPFlag == 1){
        ADC_Result_Process();
        ADCs.u8ADCSampleCMPFlag = 0;
    }
}

/**
 * @brief  获取ADC平均值
 * @param  ch: 通道
 * @retval 平均值
 */
unsigned short int ADC_GetAvgValue(unsigned char ch){
    return (uint16_t)(ADCs.adc_average[ch]);
}


/**
 * @brief  获取ADC原始值
 * @param  ch: 通道
 * @retval 原始值
 */
unsigned short int ADC_GetRawValue(unsigned char ch){
    return (uint16_t)(ADCs.adc_result[ch]);
}


/**
 * @brief  DMA1通道1中断
 * @param  None
 * @retval None
 */
void DMA1_Channel3_IRQHandler(void){
    if(DMA1->sts & DMA1_FDT3_FLAG){
        DMA1->clr = DMA1_FDT3_FLAG;
        ADCs.u8ADCSampleCMPFlag = 1;
    }
}

