#if 0

#include "dap_hid.h"
#include "swd_host.h"
#include "dap_descriptors.h"
#include "dap_main.h"
#include "user_param.h"
#include "mbcrc.h"

static uint8_t hid_send_len = 0;

uint8_t hid_cmd_01(uint8_t *data)
{
    // len
    hid_send_buffer[hid_send_len++] = 3;
    hid_send_buffer[hid_send_len++] = user_param.major;
    hid_send_buffer[hid_send_len++] = user_param.minor;
    hid_send_buffer[hid_send_len++] = user_param.patch;
    return 0;
}

uint8_t hid_cmd_02(uint8_t *data)
{
    // len
    hid_send_buffer[hid_send_len++] = 0;
    NVIC_SystemReset();
    return 0;
}

uint8_t hid_cmd_03(uint8_t *data)
{
    flash_ee_data_write(PARAM_DAP_ID_ADDR, (uint16_t) data[0]);
    // len
    hid_send_buffer[hid_send_len++] = 0;
    return 0;
}

uint8_t hid_cmd_04(uint8_t *data)
{
    // len
    hid_send_buffer[hid_send_len++] = 1;
    hid_send_buffer[hid_send_len++] = LED_CON_STATE();
    return 0;
}

uint8_t hid_cmd_05(uint8_t *data)
{
    uint8_t led_state = data[0];
    if(led_state)
    {
        LED_CON_ON();
    }
    else
    {
        LED_CON_OFF();
    }
    // len
    hid_send_buffer[hid_send_len++] = 0;
    return 0;
}

uint8_t hid_cmd_06(uint8_t *data)
{
    PIN_nRESET_GPIO->scr = PIN_nRESET;
    delay_ms(5);
    PIN_nRESET_GPIO->clr = PIN_nRESET;
    // len
    hid_send_buffer[hid_send_len++] = 0;
    return 0;
}

uint8_t hid_cmd_07(uint8_t *data)
{
    uint8_t result = swd_init_debug();
    if(!result)
    {
        goto end;
    }
    result = swd_write_word((uint32_t) &SCB->AIRCR, ((0x5FA << SCB_AIRCR_VECTKEY_Pos) | SCB_AIRCR_SYSRESETREQ_Msk));

end:
    // len
    hid_send_buffer[hid_send_len++] = 1;
    // result
    hid_send_buffer[hid_send_len++] = result;
    return 0;
}

uint8_t hid_cmd_0c(uint8_t *data)
{
    // len
    hid_send_buffer[hid_send_len++] = 4;
    // result
    hid_send_buffer[hid_send_len++] = (IC_UID >> 24) & 0xff;
    hid_send_buffer[hid_send_len++] = (IC_UID >> 16) & 0xff;
    hid_send_buffer[hid_send_len++] = (IC_UID >> 8) & 0xff;
    hid_send_buffer[hid_send_len++] = (IC_UID >> 0) & 0xff;
    return 0;
}

uint8_t hid_cmd_0d(uint8_t *data)
{
    uint8_t result = swd_init_debug();
    uint32_t cpuid = 0xffffffff;
    if(!result)
    {
        goto end;
    }
    result = swd_read_word((uint32_t) &SCB->CPUID, &cpuid);
    
end:
    // len
    hid_send_buffer[hid_send_len++] = 4;
    // result
    hid_send_buffer[hid_send_len++] = (cpuid >> 24) & 0xff;
    hid_send_buffer[hid_send_len++] = (cpuid >> 16) & 0xff;
    hid_send_buffer[hid_send_len++] = (cpuid >> 8) & 0xff;
    hid_send_buffer[hid_send_len++] = (cpuid >> 0) & 0xff;
    return 0;
}

uint8_t hid_cmd_0e(uint8_t *data)
{
    uint8_t result = swd_init_debug();
    uint32_t idcode = 0xffffffff;
    if(!result)
    {
        goto end;
    }
    result = swd_read_word((uint32_t) 0xE0042000, &idcode);
    
end:
    // len
    hid_send_buffer[hid_send_len++] = 4;
    // result
    hid_send_buffer[hid_send_len++] = (idcode >> 24) & 0xff;
    hid_send_buffer[hid_send_len++] = (idcode >> 16) & 0xff;
    hid_send_buffer[hid_send_len++] = (idcode >> 8) & 0xff;
    hid_send_buffer[hid_send_len++] = (idcode >> 0) & 0xff;
    return 0;
}

uint8_t hid_cmd_0f(uint8_t *data)
{
    uint8_t result = swd_init_debug();
    uint32_t flash_size = 0xffffffff;
    if(!result)
    {
        goto end;
    }
    result = swd_read_word((uint32_t) 0X1FFFF7E0, &flash_size);
    
end:
    // len
    hid_send_buffer[hid_send_len++] = 2;
    // result
    hid_send_buffer[hid_send_len++] = (flash_size >> 8);
    hid_send_buffer[hid_send_len++] = (flash_size >> 0) & 0xff;
    return 0;
}

uint8_t hid_cmd_10(uint8_t *data)
{
    uint32_t speed = data[0] << 24;
    speed |= data[1] << 16;
    speed |= data[2] << 8;
    speed |= data[3] << 0;
    
    extern void Set_Clock_Delay(uint32_t clock);
    if(speed <= 10000000)
    {
        Set_Clock_Delay(speed);
    }
    
    // len
    hid_send_buffer[hid_send_len++] = 0;
    return 0;
}

uint8_t hid_cmd_08(uint8_t *data)
{
    uint32_t addr = data[0] << 24;
    addr |= data[1] << 16;
    addr |= data[2] << 8;
    addr |= data[3] << 0;
    
    uint32_t val = data[4] << 24;
    val |= data[5] << 16;
    val |= data[6] << 8;
    val |= data[7] << 0;
    
    uint8_t result = swd_init_debug();
    if(!result)
    {
        goto end;
    }
    result = swd_write_word(addr, val);

end:
    // len
    hid_send_buffer[hid_send_len++] = 1;
    // result
    hid_send_buffer[hid_send_len++] = result;
    return 0;
}

uint8_t hid_cmd_09(uint8_t *data)
{
    uint32_t addr = data[0] << 24;
    addr |= data[1] << 16;
    addr |= data[2] << 8;
    addr |= data[3] << 0;
    
    uint8_t result = swd_init_debug();
    uint32_t memval = 0xFFFFFFFF;
    if(!result)
    {
        goto end;
    }
    result = swd_read_word((uint32_t) addr, &memval);

end:
    // len
    hid_send_buffer[hid_send_len++] = 5;
    // result
    hid_send_buffer[hid_send_len++] = result;
    // data
    hid_send_buffer[hid_send_len++] = (memval >> 24) & 0xff;
    hid_send_buffer[hid_send_len++] = (memval >> 16) & 0xff;
    hid_send_buffer[hid_send_len++] = (memval >> 8) & 0xff;
    hid_send_buffer[hid_send_len++] = (memval >> 0) & 0xff;
    return 0;
}

uint8_t hid_cmd_0a(uint8_t *data)
{
    uint32_t addr = data[0] << 24;
    addr |= data[1] << 16;
    addr |= data[2] << 8;
    addr |= data[3] << 0;
    
    uint8_t val = data[4];
    
    uint8_t result = swd_init_debug();
    if(!result)
    {
        goto end;
    }
    result = swd_write_byte(addr, val);

end:
    // len
    hid_send_buffer[hid_send_len++] = 1;
    // result
    hid_send_buffer[hid_send_len++] = result;
    return 0;
}

uint8_t hid_cmd_0b(uint8_t *data)
{
    uint32_t addr = data[0] << 24;
    addr |= data[1] << 16;
    addr |= data[2] << 8;
    addr |= data[3] << 0;
    
    uint8_t result = swd_init_debug();
    uint8_t memval = 0xFF;
    if(!result)
    {
        goto end;
    }
    result = swd_read_byte((uint32_t) addr, &memval);

end:
    // len
    hid_send_buffer[hid_send_len++] = 2;
    // result
    hid_send_buffer[hid_send_len++] = result;
    // data
    hid_send_buffer[hid_send_len++] = memval;
    return 0;
}

void chry_dap_hid_exec(uint8_t *data, uint8_t len)
{
    uint8_t result = 1;
    hid_send_len = 0;
    uint8_t cmd = data[0];
    uint8_t *data_ptr = &data[2];
    
    memset(hid_send_buffer, 0x0, HIDRAW_IN_EP_SIZE);
    // report id
    hid_send_buffer[hid_send_len++] = HID_IN_REPORT_ID;
    // header
    hid_send_buffer[hid_send_len++] = HID_PACK_HEADER_0;
    hid_send_buffer[hid_send_len++] = HID_PACK_HEADER_1;
    // CMD
    hid_send_buffer[hid_send_len++] = cmd;
    switch(cmd)
    {
        case 0x01:{
            if(len == 0)
            {
                result = hid_cmd_01(data_ptr);
            }
            break;
        }
        case 0x02:{
            if(len == 0)
            {
                result = hid_cmd_02(data_ptr);
            }
            break;
        }
        case 0x03:{
            if(len == 1)
            {
                result = hid_cmd_03(data_ptr);
            }
            break;
        }
        case 0x04:{
            if(len == 0)
            {
                result = hid_cmd_04(data_ptr);
            }
            break;
        }
        case 0x05:{
            if(len == 1)
            {
                result = hid_cmd_05(data_ptr);
            }
            break;
        }
        case 0x06:{
            if(len == 0)
            {
                result = hid_cmd_06(data_ptr);
            }
            break;
        }
        case 0x07:{
            if(len == 0)
            {
                result = hid_cmd_07(data_ptr);
            }
            break;
        }
        case 0x08:{
            if(len == 8)
            {
                result = hid_cmd_08(data_ptr);
            }
            break;
        }
        case 0x09:{
            if(len == 4)
            {
                result = hid_cmd_09(data_ptr);
            }
            break;
        }
        case 0x0A:{
            if(len == 5)
            {
                result = hid_cmd_0a(data_ptr);
            }
            break;
        }
        case 0x0B:{
            if(len == 4)
            {
                result = hid_cmd_0b(data_ptr);
            }
            break;
        }
        case 0x0c:{
            if(len == 0)
            {
                result = hid_cmd_0c(data_ptr);
            }
            break;
        }
        case 0x0d:{
            if(len == 0)
            {
                result = hid_cmd_0d(data_ptr);
            }
            break;
        }
        case 0x0e:{
            if(len == 0)
            {
                result = hid_cmd_0e(data_ptr);
            }
            break;
        }
        case 0x0f:{
            if(len == 0)
            {
                result = hid_cmd_0f(data_ptr);
            }
            break;
        }
        case 0x10:{
            if(len == 4)
            {
                result = hid_cmd_10(data_ptr);
            }
            break;
        }
        default:{
            break;
        }
    }
    // CRC
    uint8_t hid_send_data_len = hid_send_buffer[4];
    /* Calculate CRC16 checksum for Modbus-Serial-Line-PDU. */
    uint16_t result_crc = usMBCRC16(&hid_send_buffer[1], hid_send_data_len + (HID_PACK_FRONT_LEN + HID_PACK_CMD_LEN + HID_PACK_LENGTH_LEN));
    hid_send_buffer[hid_send_len++] = ( UCHAR )( result_crc & 0xFF );
    hid_send_buffer[hid_send_len++] = ( UCHAR )( result_crc >> 8 );
    
    // write
    if(result == 0)
    {
        usbd_ep_start_write(DAP_BUS_ID, HIDRAW_IN_EP, hid_send_buffer, HIDRAW_IN_EP_SIZE);
    }
}

#endif