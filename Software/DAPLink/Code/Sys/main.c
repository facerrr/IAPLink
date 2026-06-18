#include "config.h"
#include "dap_main.h"
#include "drv_usb2uart.h"
#include "drv_led.h"
#include "user_param.h"
#include "easy_ui.h"

void SysClkInit(void);
void Clock_Config(void);


void SysClkInit(void)
{
    crm_reset();

    crm_clock_source_enable(CRM_CLOCK_SOURCE_LICK, TRUE);
    while(crm_flag_get(CRM_LICK_STABLE_FLAG) != SET);

    #if USE_HEXT == TRUE
    crm_clock_source_enable(CRM_CLOCK_SOURCE_HEXT, TRUE);
    while(crm_hext_stable_wait() == ERROR)
    #endif
    
    crm_clock_source_enable(CRM_CLOCK_SOURCE_HICK, TRUE);
    while(crm_flag_get(CRM_HICK_STABLE_FLAG) != SET);
    
    #if USE_HEXT == TRUE
    crm_pll_config(CRM_PLL_SOURCE_HEXT, CRM_PLL_MULT_30, CRM_PLL_OUTPUT_RANGE_GT72MHZ);
//    crm_hext_clock_div_set(CRM_HEXT_DIV_2);
    #else
    crm_pll_config(CRM_PLL_SOURCE_HICK, CRM_PLL_MULT_60, CRM_PLL_OUTPUT_RANGE_GT72MHZ);
    #endif

    crm_clock_source_enable(CRM_CLOCK_SOURCE_PLL, TRUE);
    while(crm_flag_get(CRM_PLL_STABLE_FLAG) != SET);

    crm_ahb_div_set(CRM_AHB_DIV_1);
    crm_apb2_div_set(CRM_APB2_DIV_2);
    crm_apb1_div_set(CRM_APB1_DIV_2);

    crm_auto_step_mode_enable(TRUE);

    crm_sysclk_switch(CRM_SCLK_PLL);
    while(crm_sysclk_switch_status_get() != CRM_SCLK_PLL);

    crm_auto_step_mode_enable(FALSE);;
    system_core_clock_update();
    
    crm_periph_clock_enable(CRM_IOMUX_PERIPH_CLOCK, TRUE);
    crm_periph_clock_enable(CRM_GPIOA_PERIPH_CLOCK, TRUE);
    crm_periph_clock_enable(CRM_GPIOB_PERIPH_CLOCK, TRUE);
    crm_periph_clock_enable(CRM_GPIOC_PERIPH_CLOCK, TRUE);
    crm_periph_clock_enable(CRM_GPIOD_PERIPH_CLOCK, TRUE);
    
    crm_periph_clock_enable(CRM_TMR1_PERIPH_CLOCK, TRUE);
    crm_periph_clock_enable(CRM_TMR2_PERIPH_CLOCK, TRUE);
    
    crm_periph_clock_enable(CRM_DMA1_PERIPH_CLOCK, TRUE);
    crm_periph_clock_enable(CRM_DMA2_PERIPH_CLOCK, TRUE);
    crm_periph_clock_enable(CRM_ADC1_PERIPH_CLOCK, TRUE);
    
    crm_periph_clock_enable(CRM_SPI3_PERIPH_CLOCK, TRUE);
    
    gpio_pin_remap_config(SWJTAG_GMUX_010, TRUE);
}


static void app_wdtinit(void)
{
    wdt_register_write_enable(TRUE);
    wdt_divider_set(WDT_CLK_DIV_32);
    wdt_reload_value_set(1250 - 1);
    wdt_counter_reload();
    wdt_enable();
}

int main(void){
    
    SysClkInit();
    delay_init();
    nvic_priority_group_config(NVIC_PRIORITY_GROUP_4);
    
    ADCx_Init();
    TIMx_Init();
    led_init();
    
    user_param_init();
    usart1_preinit();
    chry_dap_init();
//    app_wdtinit();
    
    LED_MODE_ON();
    LED_PWR_ON();
    
    EasyUIInit(1);
    delay_ms(100);
    
    KeyValue_Init();
   
    while(1){
//        wdt_counter_reload();
        ADCx_Func();
        KeyFunc();
        if(KeyFlag_Read(&KEY_MOD) == 2){
            AppToBoot();
        }
        chry_dap_handle();
        chry_dap_usb2uart_rx(&usart1_state);
        chry_dap_usb2uart_handle(&usart1_state);
        
        EasyUI_Func();
    }
}  


void extend_sram(void){
    if(((USD->eopb0) & 0xFF) != 0xFF){
        flash_unlock();
        flash_user_system_data_erase();
        flash_user_system_data_program((uint32_t)&USD->eopb0, 0xFF);
        NVIC_SystemReset();
    }
}


void HardFault_Handler(void){
    while(1){
        
    }
}
