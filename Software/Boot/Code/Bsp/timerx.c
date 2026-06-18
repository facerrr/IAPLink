#include "main.h"

uint8_t u8timerFlag;
uint8_t u8Time1ms;
uint8_t u8Time100ms;

uint32_t _NOP_MS;
uint32_t u32Time1s;
uint32_t u32SysTick1ms;

void BaseTimer_Init(void);
void ADCTimer_Init(void);

/**
 * @brief  定时器初始化
 * @param  None
 * @retval None
 */
void TIMx_Init(void){
    u8timerFlag = 0;
    u8Time1ms = 0;
    u8Time100ms = 0;
    u32Time1s = 0;
    u32SysTick1ms = 0;
    BaseTimer_Init();
}

/**
 * @brief  基础定时器
 * @param  None
 * @retval None
 */
void BaseTimer_Init(void){
    tmr_base_init(TMR2, 1000U - 1U, 240U - 1U);
    tmr_cnt_dir_set(TMR2, TMR_COUNT_UP);
    tmr_clock_source_div_set(TMR2, TMR_CLOCK_DIV1);
    tmr_repetition_counter_set(TMR2, 0);

    nvic_priority_group_config(NVIC_PRIORITY_GROUP_4);
    nvic_irq_enable(TMR2_GLOBAL_IRQn, 15, 0);

    tmr_interrupt_enable(TMR2, TMR_OVF_INT, TRUE);
    tmr_counter_enable(TMR2, TRUE);
}



/**
 * @brief  1s Function
 * @param  None
 * @retval None
 */
void tick1s(void){

}

/**
 * @brief  100ms Function
 * @param  None
 * @retval None
 */
void tick100ms(void){

}


/**
 * @brief  1ms Function
 * @param  None
 * @retval None
 */
void tick1ms(void){
    FileDownLoad_Tick();
}


/**
 * @brief  定义毫秒延时
 * @param  None
 * @retval None
 */
void OneMs_Define(void){
    while(!u8timerFlag){
    }
    u8timerFlag = 0;
    _NOP_MS = 0;
    while(!u8timerFlag){
        __NOP();
        _NOP_MS++;
    }
    u8timerFlag = 0;
    _NOP_MS = _NOP_MS * 2 * 0.95;
}

/**
 * @brief  毫秒延时
 * @param  ms: 延时时间
 * @retval None
 */
void delay_ms(const unsigned int ms)
{
    volatile uint32_t i = ms * _NOP_MS;
    while (i-- > 0) {
        __NOP();
    }
}

/**
 * @brief  微秒延时
 * @param  us: 延时时间
 * @retval None
 */
void delay_us(const unsigned int us){
    volatile uint32_t i = us * _NOP_MS / 1000;
    while (i-- > 0) {
        __NOP();
    }
}


/**
 * @brief  定时器2中断
 * @param  None
 * @retval None
 */
void TMR2_GLOBAL_IRQHandler(void){
    if(tmr_interrupt_flag_get(TMR2, TMR_OVF_FLAG) != RESET){
        tmr_flag_clear(TMR2, TMR_OVF_FLAG);
        u8timerFlag = 1;
        u32SysTick1ms++;
        tick1ms();
        u8Time1ms++;
        if(u8Time1ms >= 100){
            u8Time1ms = 0;
            tick100ms();
            u8Time100ms++;
            if(u8Time100ms >= 10){
                u8Time100ms = 0;
                u32Time1s++;
                tick1s();
            }
        }
    }
}