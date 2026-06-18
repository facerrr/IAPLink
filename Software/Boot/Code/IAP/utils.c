#include "main.h"

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
uint8_t Cal_ADD8(uint8_t* data, int offset, uint32_t size){
    uint8_t checksum = 0;
    for (uint32_t i = 0; i < size; ++i) {
        checksum += data[offset + i];
    }
    return checksum;
}


void App_Clear_Sys_Status(void){
    /*Close Peripherals Clock*/
    CRM->apb2rst = 0xFFFF;
    CRM->apb2rst = 0;
    CRM->apb1rst = 0xFFFF;
    CRM->apb1rst = 0;
    CRM->apb1en = 0;
    CRM->apb2en = 0;
    /*Close PLL*/
    /* Reset SW, AHBDIV, APB1DIV, APB2DIV, ADCDIV and CLKOUT_SEL bits */
    CRM->cfg_bit.sclksel = 0;
    CRM->cfg_bit.ahbdiv = 0;
    CRM->cfg_bit.apb1div = 0;
    CRM->cfg_bit.apb2div = 0;
    CRM->cfg_bit.adcdiv_l = 0;
    CRM->cfg_bit.adcdiv_h = 0;
    CRM->cfg_bit.clkout_sel = 0;
    CRM->ctrl_bit.hexten = 0;
    CRM->ctrl_bit.cfden = 0;
    CRM->ctrl_bit.pllen = 0;
    CRM->cfg_bit.pllrcs = 0;
    CRM->cfg_bit.pllhextdiv = 0;
    CRM->cfg_bit.pllmult_l = 0;
    CRM->cfg_bit.pllmult_h = 0;
    CRM->cfg_bit.usbdiv_l = 0;
    CRM->cfg_bit.usbdiv_h = 0;
    CRM->cfg_bit.pllrange = 0;
    /* Disable all interrupts and clear pending bits */
    CRM->clkint_bit.lickstblfc = 0;
    CRM->clkint_bit.lextstblfc = 0;
    CRM->clkint_bit.hickstblfc = 0;
    CRM->clkint_bit.hextstblfc = 0;
    CRM->clkint_bit.pllstblfc = 0;
    CRM->clkint_bit.cfdfc = 0;
    /*Colse Systick*/
    SysTick->CTRL = 0;
    
    Clock_Config(FALSE);
    __disable_irq();
}
