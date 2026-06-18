#ifndef __MODEM_H__
#define __MODEM_H__

#include "base_types.h"

#define FRAME_HEAD_L                        0xACu
#define FRAME_HEAD_H                        0x6Du

#define FRAME_RECV_TIMEOUT                  5               // ms
#define FRAME_NUM_XOR_BYTE                  0xFF

typedef struct{
    uint32_t FileAddrAppSt;
    uint32_t FileAddrAppEd;
    uint32_t FileAddrInfoSt;
    uint32_t FileAddrInfoEd;
}CurrentFileStruct;

extern CurrentFileStruct CurrentFile;
extern uint8_t u8FileDownloadStart;

void Modem_Init(void);
en_result_t Modem_Process(void);
void Modem_RamInit(void);
void FileDownLoad_Tick(void);
void Modem_Func(void);

#endif  // __MODEM_H__