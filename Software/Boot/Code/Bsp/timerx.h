#ifndef __TIMERX_H__
#define __TIMERX_H__

#define BASE_TIM_DIV        TMR_CLOCK_DIV1
#define BASE_TIM_PERIOD     1000U   // 1KHz


#define TIME100US_(x)        (x)
#define TIME1MS_(x)          (x)
#define TIME10MS_(x)         (x)
#define TIME100MS_(x)        (x)
#define TIME1S_(x)           (x)

extern unsigned int u32SysTick1ms;

void TIMx_Init(void);
void delay_ms(const unsigned int ms);
void delay_us(const unsigned int us);
void OneMs_Define(void);


#endif  // __TIMERX_H__


