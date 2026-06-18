#ifndef __RECV_H__
#define __RECV_H__

#include "base_types.h"


void IAPRecv_Func(uint8_t);
void RecvValue_Init(uint8_t* rxbuf, TransStruct* transStr, LoadingPmStruct* loadingPm);

void ModemRX_Handle(RecvStruct* recvStr, uint8_t* frameData, uint8_t recvData, uint8_t isHost);
void NormalRX_Handle(RecvStruct* recvStr, uint8_t* frameData, uint8_t recvData, uint8_t isHost);

#endif // __RECV_H__