#ifndef __IAP_H__
#define __IAP_H__

#include "base_types.h"

#define IAP_ZSERIES_SIZE    256

void IAPModule_Init(void);
void IAPValue_Init(void);
void StopTimer(void);
void IAPFunc(void);
void IAP_Tick1ms(void);
void IAPRam_Init(unsigned char index);
void FileFlashAddr_Init(uint8_t index);
TransStruct* IAPGetTransStr(void);
unsigned char IAPIsRunning(void);

#endif  // __IAP_H__

