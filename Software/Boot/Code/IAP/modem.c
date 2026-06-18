#include "usb.h"
RecvStruct RecvFH;  // Recv From Host
CurrentFileStruct CurrentFile;
uint8_t u8RecvModemFH[FRAME_MAX_SIZE];
uint32_t u32FrameRecvOvertime;
FileInfoStruct FileInfo;


static uint16_t FLASH_PageNumber(uint32_t u32Size);
/**
 * @brief  数据回传
 * @param  None
 * @retval None
 */
void Modem_SendFrame(uint8_t *u8TxBuff, uint16_t u16TxLength){
    uint16_t u16Crc16;
    u8TxBuff[FRAME_LENGTH_INDEX] = u16TxLength & 0xFF;
    u8TxBuff[FRAME_LENGTH_INDEX + 1] = u16TxLength >> 8;
    u16Crc16 = Cal_CRC16(&u8TxBuff[FRAME_PACKET_INDEX], 0, u16TxLength);
    u8TxBuff[FRAME_PACKET_INDEX + u16TxLength] = u16Crc16 & 0x00FF;
    u8TxBuff[FRAME_PACKET_INDEX + u16TxLength + 1] = u16Crc16 >> 8;
    usb_vcp_send_data(&usb_core_dev, u8TxBuff, FRAME_PACKET_INDEX + u16TxLength + 2);
}

/**
 * @brief  File Downloading
 * @param  None
 * @retval en_result_t 
 */
en_result_t Modem_Process(void){
    uint8_t  u8Cmd, u8FlashAddrValid, u8Cnt, u8Ret;
    uint16_t u16DataLength, u16PageNum, u16Ret;
    uint32_t u32FlashAddr, u32FlashLength, u32Temp;

    if(RecvFH.FrameRecvStatus == FRAME_RECV_PROC_STATUS){
        u8Cmd = u8RecvModemFH[PACKET_CMD_INDEX];
        if (PACKET_TYPE_DATA == u8RecvModemFH[PACKET_TYPE_INDEX]){
            u8FlashAddrValid = 0;
            u32FlashAddr = u8RecvModemFH[PACKET_ADDRESS_INDEX] +
                           (u8RecvModemFH[PACKET_ADDRESS_INDEX + 1] << 8) +
                           (u8RecvModemFH[PACKET_ADDRESS_INDEX + 2] << 16) +
                           (u8RecvModemFH[PACKET_ADDRESS_INDEX + 3] << 24);
            if((u32FlashAddr >= (CurrentFile.FileAddrAppSt)) && (u32FlashAddr < CurrentFile.FileAddrAppEd)){
                u8FlashAddrValid = 1;
            }
        }
        switch (u8Cmd)
        {
        case PACKET_CMD_HANDSHAKE:
            u8RecvModemFH[PACKET_RESULT_INDEX] = PACKET_ACK_OK;
            Modem_SendFrame(&u8RecvModemFH[0], PACKET_INSTRUCT_SIZE);
            break;
        case PACKET_CMD_ERASE_FLASH:
            if ((u32FlashAddr % FLASH_SECTOR_SIZE) != 0){
                u8FlashAddrValid = 0;
            }
            if(u8FlashAddrValid){
                u32Temp = u8RecvModemFH[PACKET_DATA_INDEX] +
                          (u8RecvModemFH[PACKET_DATA_INDEX + 1] << 8) +
                          (u8RecvModemFH[PACKET_DATA_INDEX + 2] << 16)+
                          (u8RecvModemFH[PACKET_DATA_INDEX + 3] << 24);
                u16PageNum = FLASH_PageNumber(u32Temp);
                u8Ret = Flashx_EraseSector(CurrentFile.FileAddrInfoSt);
                if(u8Ret != Ok){
                    u8RecvModemFH[PACKET_RESULT_INDEX] = PACKET_ACK_ERROR;
                    break;
                }
                FileInfo.FileLenght = u32Temp;
                u8Ret = Flashx_WriteBytes(CurrentFile.FileAddrInfoSt, (uint8_t*)&FileInfo, sizeof(FileInfoStruct));
                if(u8Ret != Ok){
                    u8RecvModemFH[PACKET_RESULT_INDEX] = PACKET_ACK_ERROR;
                    break;
                }
                for(u8Cnt = 0; u8Cnt < u16PageNum; u8Cnt++){
                    u8Ret = Flashx_EraseSector(u32FlashAddr + (u8Cnt * FLASH_SECTOR_SIZE));
                    if(u8Ret != Ok){
                        u8RecvModemFH[PACKET_RESULT_INDEX] = PACKET_ACK_ERROR;
                        break;
                    }
                }
                if(u8Ret == Ok){
                    u8RecvModemFH[PACKET_RESULT_INDEX] = PACKET_ACK_OK;
                }else{
                    u8RecvModemFH[PACKET_RESULT_INDEX] = PACKET_ACK_TIMEOUT;
                }
            }else{
                u8RecvModemFH[PACKET_RESULT_INDEX] = PACKET_ACK_ADDR_ERROR;
            }
            Modem_SendFrame(&u8RecvModemFH[0], PACKET_INSTRUCT_SIZE);
            break;
        case PACKET_CMD_APP_DOWNLOAD:
            if(u8FlashAddrValid){
                u16DataLength = u8RecvModemFH[FRAME_LENGTH_INDEX] + (u8RecvModemFH[FRAME_LENGTH_INDEX + 1] << 8) - PACKET_INSTRUCT_SIZE;
                if (u16DataLength > PACKET_DATA_SIZE){
                    u16DataLength = PACKET_DATA_SIZE;
                }
                u8Ret = Flashx_WriteBytes(u32FlashAddr, (uint8_t *)&u8RecvModemFH[PACKET_DATA_INDEX], u16DataLength);
                if(Ok != u8Ret){
                    u8RecvModemFH[PACKET_RESULT_INDEX] = PACKET_ACK_ERROR;
                }else{
                    u8RecvModemFH[PACKET_RESULT_INDEX] = PACKET_ACK_OK;
                }
            }else{
                u8RecvModemFH[PACKET_RESULT_INDEX] = PACKET_ACK_ADDR_ERROR;
            }
            Modem_SendFrame(&u8RecvModemFH[0], PACKET_INSTRUCT_SIZE);
            break;
        case PACKET_CMD_FLASH_CRC:
            if(u8FlashAddrValid){
                u32FlashLength = u8RecvModemFH[PACKET_DATA_INDEX] +            
                                (u8RecvModemFH[PACKET_DATA_INDEX + 1] << 8) +
                                (u8RecvModemFH[PACKET_DATA_INDEX + 2] << 16) +
                                (u8RecvModemFH[PACKET_DATA_INDEX + 3] << 24);  
                if((u32FlashLength + u32FlashAddr) > (CurrentFile.FileAddrAppEd)){
                    u8RecvModemFH[PACKET_RESULT_INDEX] = PACKET_ACK_FLASH_SIZE_ERROR;
                }else{
                    u16Ret = Cal_CRC16(((unsigned char *)u32FlashAddr), 0, u32FlashLength);
                    FileInfo.Crc16 = u16Ret;
                    u8RecvModemFH[PACKET_FLASH_CRC_INDEX] = (uint8_t)u16Ret;
                    u8RecvModemFH[PACKET_FLASH_CRC_INDEX + 1] = (uint8_t)(u16Ret>>8);
                    u8RecvModemFH[PACKET_RESULT_INDEX] = PACKET_ACK_OK;
                }
            }else{
                u8RecvModemFH[PACKET_RESULT_INDEX] = PACKET_ACK_ADDR_ERROR;
            }
            Modem_SendFrame(&u8RecvModemFH[0], PACKET_INSTRUCT_SIZE + 2);
            break;
        case PACKET_CMD_JUMP_TO_APP:
            u8RecvModemFH[PACKET_RESULT_INDEX] = PACKET_ACK_OK;
            Modem_SendFrame(&u8RecvModemFH[0], PACKET_INSTRUCT_SIZE);
            delay_ms(20);
            return Ok;
        case PACKET_CMD_APP_UPLOAD:
            if(u8FlashAddrValid){
                u32Temp = u8RecvModemFH[PACKET_DATA_INDEX] +
                        (u8RecvModemFH[PACKET_DATA_INDEX + 1] << 8) +
                        (u8RecvModemFH[PACKET_DATA_INDEX + 2] << 16) +
                        (u8RecvModemFH[PACKET_DATA_INDEX + 3] << 24);  
                if(u32Temp > PACKET_DATA_SIZE){
                    u32Temp = PACKET_DATA_SIZE; 
                }
                Flashx_ReadBytes(u32FlashAddr, (uint8_t *)&u8RecvModemFH[PACKET_DATA_INDEX], u32Temp);
                u8RecvModemFH[PACKET_RESULT_INDEX] = PACKET_ACK_OK;   
                Modem_SendFrame(&u8RecvModemFH[0], PACKET_INSTRUCT_SIZE + u32Temp);
            }else{
                u8RecvModemFH[PACKET_RESULT_INDEX] = PACKET_ACK_ADDR_ERROR;
                Modem_SendFrame(&u8RecvModemFH[0], PACKET_INSTRUCT_SIZE);
            }
            break;
        default:
            break;
        }
        RecvFH.FrameRecvStatus = FRAME_RECV_IDLE_STATUS;
        if(u8RecvModemFH[PACKET_RESULT_INDEX] != PACKET_ACK_OK){
            return Error;
        }
    }
    return OperationInProgress;
}


/**
 * @brief  计算页数
 * @param  u32Size: The size of the data
 * @retval uint16_t: The number of pages
 */
static uint16_t FLASH_PageNumber(uint32_t u32Size){
    uint16_t u32PageNum = u32Size / FLASH_SECTOR_SIZE;

    if ((u32Size % FLASH_SECTOR_SIZE) != 0){
        u32PageNum += 1u;
    }
    return u32PageNum;
}


/**
 * @brief  Modem_Func
 * @param  None
 * @retval None
 */
en_result_t Modem_Func(void){
    uint8_t tempData;
    if(!chry_ringbuffer_check_empty(&usb_rx_ringbuffer)){
        u32FrameRecvOvertime = 0;
        if(chry_ringbuffer_read_byte(&usb_rx_ringbuffer, &tempData)){
            ModemRX_Handle(&RecvFH, u8RecvModemFH, tempData, 1);
        }
    }
    en_result_t enRet = Modem_Process();
    return enRet;
}


/**
 * @brief  模块RAM初始化
 * @param  None
 * @retval None
 */
void Modem_RamInit(void){
    memset(u8RecvModemFH, 0, FRAME_MAX_SIZE);
    memset(&RecvFH, 0, sizeof(RecvStruct));
    RecvFH.FrameRecvStatus = FRAME_RECV_IDLE_STATUS;
    u32FrameRecvOvertime = 0;
    FileFlashAddr_Init(0);
}


void FileFlashAddr_Init(uint8_t index){
    CurrentFile.FileAddrAppSt = FLASH_BINK2_BASE + (IAP_FILE_SIZE * index);
    CurrentFile.FileAddrAppEd = CurrentFile.FileAddrAppSt + IAP_FILE_APP_SIZE - 1;
    CurrentFile.FileAddrInfoSt = CurrentFile.FileAddrAppEd + 1;
    CurrentFile.FileAddrInfoEd = CurrentFile.FileAddrInfoSt + IAP_FILE_INFO_SIZE - 1;
}



/**
 * @brief  文件下载计时
 * @param  None
 * @retval None
 */
void FileDownLoad_Tick(void){
    u32FrameRecvOvertime++;
    if((RecvFH.FrameRecvStatus == FRAME_RECV_HEADER_STATUS) || (RecvFH.FrameRecvStatus == FRAME_RECV_DATA_STATUS)){
        // 超过10ms未接收到数据
        if(u32FrameRecvOvertime++ > 10){
            RecvFH.FrameRecvStatus = FRAME_RECV_IDLE_STATUS;
        }
    }
    else if(RecvFH.FrameRecvStatus == FRAME_RECV_PROC_STATUS){
        // 超过4500ms未接收到数据
        if(u32FrameRecvOvertime++ > 4500){
            RecvFH.FrameRecvStatus = FRAME_RECV_IDLE_STATUS;
        }
    }
}


void ModemRX_Handle(RecvStruct* recvStr, uint8_t* frameData, uint8_t recvData, uint8_t isHost){
    uint16_t crc16;
    switch (recvStr->FrameRecvStatus){
    case FRAME_RECV_IDLE_STATUS:
        if(recvData == FRAME_HEAD_L){
            memset(frameData, 0, FRAME_MAX_SIZE);
            frameData[FRAME_HEAD_INDEX] = recvData;
            recvStr->FrameRecvStatus = FRAME_RECV_HEADER_STATUS;
        }
        break;
    case FRAME_RECV_HEADER_STATUS:
        if(recvData == FRAME_HEAD_H){
            frameData[FRAME_HEAD_L_INDEX] = recvData;
            recvStr->FrameDataIndex = FRAME_NUM_INDEX;
            recvStr->FrameRecvStatus = FRAME_RECV_DATA_STATUS;
        }else if(recvData == FRAME_HEAD_L){
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
                recvStr->FrameRecvStatus = FRAME_RECV_PROC_STATUS;
            }else{
                recvStr->FrameRecvStatus = FRAME_RECV_IDLE_STATUS;
            }
        }
        break;
    case FRAME_RECV_PROC_STATUS:
        break;
    default:
        break;
    }
}