#include "main.h"

uint8_t u8RecvNormalFD[7];
RecvStruct RecvFD;

uint8_t* u8IAPRxPtr = NULL;
TransStruct* IAPTransPtr = NULL;
LoadingPmStruct* LoadingPmPtr = NULL;

uint8_t u8CheckSumIndex;

void CommModemHandler(uint8_t* frameData);


void RecvValue_Init(uint8_t* rxbuf, TransStruct* transStr, LoadingPmStruct* loadingPm){
    u8IAPRxPtr = rxbuf;
    IAPTransPtr = transStr;
    LoadingPmPtr = loadingPm;
}


void IAPRecv_Func(uint8_t data){
    if(u8IAPRxPtr == NULL){
        return;
    }
    if(RecvFD.FrameRecvStatus == FRAME_RECV_IDLE_STATUS){
        RecvFD.FrameDataIndex = 0;
        RecvFD.FrameSize = 0;
    }
    if(IAPTransPtr->recvType == RECV_NORMAL){
        NormalRX_Handle(&RecvFD, u8RecvNormalFD, data, 0);
        if(RecvFD.FrameRecvStatus == FRAME_RECV_PROC_STATUS){
            IAPTransPtr->appIsRuning = 1;
            if(IAPTransPtr->iapVerion == XSERIES){
                if(u8RecvNormalFD[3] == 0xDD){
                    if(u8RecvNormalFD[4] == 0xDD){
                        if(u8RecvNormalFD[5] == 0xDD){
                            IAPTransPtr->recvType = 1;
                            if(IAPTransPtr->transStatus == TransBegin){
                                StopTimer();
                                IAPTransPtr->transStatus = TransFinished;
                            }
                        }else{
                            IAPTransPtr->transStatus = TransAddrError;
                        }
                    }
                }
            }
            RecvFD.FrameRecvStatus = FRAME_RECV_IDLE_STATUS;
        }
    }else if(IAPTransPtr->recvType == RECV_ZSERIES_IAP){
        if(IAPTransPtr->iapIngFlag){
            switch(LoadingPmPtr->transProcess){
            case PACKET_CMD_APP_UPGRADE:
                if(data == 0xB0){
                    IAPTransPtr->transStatus = TransFinished;
                    StopTimer();
                }
                break;
            case PACKET_CMD_HANDSHAKE:
                if(data == 0xAC){
                    IAPTransPtr->transStatus = TransFinished;
                    IAPTransPtr->sum8check = 0;
                    u8CheckSumIndex = 0;
                    StopTimer();
                }
                break;
            case PACKET_CMD_FLASH_CRC:
                IAPTransPtr->sum8check += (data << (24 - u8CheckSumIndex * 8));
                u8CheckSumIndex ++;
                if(u8CheckSumIndex == 4){
                    IAPTransPtr->transStatus = TransFinished;
                    StopTimer();
                }
                break;  
            default:
                break;
            }
        }
    }else if(IAPTransPtr->recvType == RECV_XSERIES_IAP){
        ModemRX_Handle(&RecvFD, u8IAPRxPtr, data, 0);
        if(RecvFD.FrameRecvStatus == FRAME_RECV_PROC_STATUS){
            IAPTransPtr->transNumber++;
            if(IAPTransPtr->transNumber == 0){
                IAPTransPtr->transNumber = 1;
            }
            memset(u8IAPRxPtr + RecvFD.FrameDataIndex - 2, 0, 2);
            CommModemHandler(u8IAPRxPtr);
            RecvFD.FrameRecvStatus = FRAME_RECV_IDLE_STATUS;
        }
    }
}


void CommModemHandler(uint8_t* frameData){
    uint8_t retSta;
    uint8_t transSta;
    retSta = frameData[PACKET_RESULT_INDEX];
    if (IAPTransPtr->transStatus == TransBegin){
        StopTimer();
        switch(retSta)
        {
        case PACKET_ACK_OK:
            transSta = TransFinished;
            break;
        case PACKET_ACK_ERROR:
            transSta = TransFailed;
            break;
        case PACKET_ACK_ADDR_ERROR:
            transSta = TransAddrError;
            break;
        default:
            transSta = TransFailed;
            break;
        }
        IAPTransPtr->transStatus = transSta;
    }
}


void ModemRX_Handle(RecvStruct* recvStr, uint8_t* frameData, uint8_t recvData, uint8_t isHost){
    uint16_t crc16;
    switch (recvStr->FrameRecvStatus){
    case FRAME_RECV_IDLE_STATUS:
        if(recvData == 0xAC){
            memset(frameData, 0, FRAME_MAX_SIZE);
            frameData[FRAME_HEAD_INDEX] = recvData;
            recvStr->FrameRecvStatus = FRAME_RECV_HEADER_STATUS;
        }
        break;
    case FRAME_RECV_HEADER_STATUS:
        if(recvData == 0x6D){
            frameData[FRAME_HEAD_L_INDEX] = recvData;
            recvStr->FrameDataIndex = FRAME_NUM_INDEX;
            recvStr->FrameRecvStatus = FRAME_RECV_DATA_STATUS;
        }else if(recvData == 0xAC){
            frameData[FRAME_HEAD_H_INDEX] = recvData;
            recvStr->FrameRecvStatus = FRAME_RECV_HEADER_STATUS;
        }else{
            recvStr->FrameRecvStatus = FRAME_RECV_IDLE_STATUS;
        }
        break;
    case FRAME_RECV_DATA_STATUS:
        frameData[recvStr->FrameDataIndex++] = recvData;
        if(recvStr->FrameDataIndex == (FRAME_NUM_INDEX + 2)){
            if((frameData[FRAME_NUM_INDEX] != (frameData[FRAME_XORNUM_INDEX] ^ FRAME_NUM_XOR_BYTE))){
                recvStr->FrameRecvStatus = FRAME_RECV_IDLE_STATUS;
            }
        }else if(recvStr->FrameDataIndex == (FRAME_LENGTH_INDEX + 2)){
            recvStr->FrameSize = frameData[FRAME_LENGTH_INDEX] + (frameData[FRAME_LENGTH_INDEX + 1] << 8) + FRAME_SHELL_SIZE;
            if((recvStr->FrameSize < FRAME_MIN_SIZE) || (recvStr->FrameSize > FRAME_MAX_SIZE)){
                recvStr->FrameRecvStatus = FRAME_RECV_IDLE_STATUS;
            }
        }else if((recvStr->FrameDataIndex > (FRAME_LENGTH_INDEX + 2)) && (recvStr->FrameDataIndex == recvStr->FrameSize)){
            crc16 = frameData[recvStr->FrameDataIndex - 2] + (frameData[recvStr->FrameDataIndex - 1] << 8);
            // frameRecvStatus = FRAME_RECV_PROC_STATUS;
            if(Cal_CRC16(frameData, FRAME_PACKET_INDEX, (recvStr->FrameSize - FRAME_SHELL_SIZE)) == crc16){
                if(!isHost){
                    if(frameData[PACKET_CMD_INDEX] == PACKET_CMD_FLASH_CRC){
                        IAPTransPtr->crcFlash = frameData[PACKET_DATA_INDEX] + (frameData[PACKET_DATA_INDEX + 1] << 8);
                    }
                }
                recvStr->FrameRecvStatus = FRAME_RECV_PROC_STATUS;
            }else{
                recvStr->FrameRecvStatus = FRAME_RECV_IDLE_STATUS;
            }
        }
        break;
        default:
            break;
    }
}


void NormalRX_Handle(RecvStruct* recvStr, uint8_t* frameData, uint8_t recvData, uint8_t isHost){
    uint8_t add8;
    uint8_t rxLen, checkData;
    if(isHost){
        rxLen = 12;
        checkData = 0x02;
    }else{
        rxLen = 7;
        checkData = 0x00;
    }
    switch (recvStr->FrameRecvStatus){
    case FRAME_RECV_IDLE_STATUS:
        if(recvData == 0xAC){
            memset(frameData, 0, rxLen);
            frameData[FRAME_HEAD_H_INDEX] = recvData;
            recvStr->FrameRecvStatus = FRAME_RECV_HEADER_STATUS;
        }else{
            recvStr->FrameRecvStatus = FRAME_RECV_IDLE_STATUS;
        }
        break;
    case FRAME_RECV_HEADER_STATUS:
        if(recvData == checkData){
            frameData[FRAME_HEAD_L_INDEX] = recvData;
            recvStr->FrameDataIndex = 2;
            recvStr->FrameRecvStatus = FRAME_RECV_DATA_STATUS;
        }else if(recvData == 0xAC){
            frameData[FRAME_HEAD_H_INDEX] = recvData;
            recvStr->FrameRecvStatus = FRAME_RECV_HEADER_STATUS;
        }else{
            recvStr->FrameRecvStatus = FRAME_RECV_IDLE_STATUS;
        }
        break;
    case FRAME_RECV_DATA_STATUS:
        frameData[recvStr->FrameDataIndex++] = recvData;
        if(recvStr->FrameDataIndex == rxLen){
            add8 = frameData[recvStr->FrameDataIndex - 1];
            if(add8 == (uint8_t)Cal_ADD8(frameData, 0, rxLen - 1)){
                recvStr->FrameRecvStatus = FRAME_RECV_PROC_STATUS;
            }else{
                recvStr->FrameRecvStatus = FRAME_RECV_IDLE_STATUS;
            }
        }
        break;
    default:
        break;
    }
}
