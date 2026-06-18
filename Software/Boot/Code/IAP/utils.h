#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdint.h>


uint16_t Cal_CRC16(uint8_t* data, int offset, uint32_t size);
uint8_t Cal_ADD8(uint8_t* data, int offset, uint32_t size);
void App_Clear_Sys_Status(void);
#endif // __UTILS_H__