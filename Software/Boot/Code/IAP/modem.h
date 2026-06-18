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

typedef struct{
    uint32_t FileLenght;
    uint16_t Crc16;
}FileInfoStruct;


void Modem_Init(void);
en_result_t Modem_Process(void);
void Modem_RamInit(void);
void FileDownLoad_Tick(void);
en_result_t Modem_Func(void);
void FileFlashAddr_Init(uint8_t index);
void ModemRX_Handle(RecvStruct* recvStr, uint8_t* frameData, uint8_t recvData, uint8_t isHost);


#endif // __MODEM_H__