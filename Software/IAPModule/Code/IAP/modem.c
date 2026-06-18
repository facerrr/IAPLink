#include "usb.h"
#include "easy_ui.h"

RecvStruct RecvFH;  // Recv From Host
CurrentFileStruct CurrentFile;
uint8_t u8RecvModemFH[FRAME_MAX_SIZE];
uint8_t u8CdcTxBuffer[7];
uint32_t u32FrameRecvOvertime;
FileInfoStruct FileInfo;

uint8_t u8RecvNormalFH[12];
uint8_t u8FileDownloadStart = 0;
uint8_t u8FileName[128] = {0};
uint8_t u8UpgradeFlag = 0;
uint8_t u8FileIndex = 0;


static uint16_t FLASH_PageNumber(uint32_t u32Size);
void CdcIAP_Handle(void);
void ModemInit_Handle(void);
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
            u16DataLength = u8RecvModemFH[FRAME_LENGTH_INDEX] + (u8RecvModemFH[FRAME_LENGTH_INDEX + 1] << 8) - PACKET_INSTRUCT_SIZE;
            memset(u8FileName, 0, sizeof(u8FileName));
            memcpy(u8FileName, &u8RecvModemFH[PACKET_DATA_INDEX], u16DataLength);
            u8FileName[u16DataLength] = '\0';
            FileInfo.FileNameLen = u16DataLength;
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
                FileInfo.FileLenght = u32Temp;
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
                    FileInfo.FileCrc = u16Ret;
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
            u8Ret = Flashx_EraseSector(CurrentFile.FileAddrInfoSt);
            u8Ret = Flashx_WriteBytes(CurrentFile.FileAddrInfoSt, (uint8_t*)&FileInfo, sizeof(FileInfoStruct));
            u8Ret = Flashx_WriteBytes(CurrentFile.FileAddrInfoSt + sizeof(FileInfoStruct), u8FileName, FileInfo.FileNameLen + 1); 
            EasyUI_FileReInit(u8FileName, FileInfo.FileNameLen + 1);
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
void Modem_Func(void){
    uint8_t tempData;
    if(u8FileDownloadStart){
        if(u8UpgradeFlag == 1){
            u8UpgradeFlag = 0;
            CdcIAP_Handle();
        }
        if(!chry_ringbuffer_check_empty(&usb_rx_ringbuffer)){
            if(chry_ringbuffer_read_byte(&usb_rx_ringbuffer, &tempData)){
                u32FrameRecvOvertime = 0;
                ModemRX_Handle(&RecvFH, u8RecvModemFH, tempData, 1);
            }
        }
        en_result_t u8Ret = Modem_Process();
        if(u8Ret != OperationInProgress){
            u8FileDownloadStart = 0;
            if(u8Ret == Ok){
                EasyUI_TransEnd(MODEM_TRANS_SUCCESS);
            }else{
                EasyUI_TransEnd(MODEM_TRANS_FAILED);
            }
            
            memset(&RecvFH, 0, sizeof(RecvStruct));
        }
        
    }else{
        if(!chry_ringbuffer_check_empty(&usb_rx_ringbuffer)){
            if(chry_ringbuffer_read_byte(&usb_rx_ringbuffer, &tempData)){
                NormalRX_Handle(&RecvFH, u8RecvNormalFH, tempData, 1);
            }
        }
        if(RecvFH.FrameRecvStatus == FRAME_RECV_PROC_STATUS){
            if(u8RecvNormalFH[4] == 0xDD && u8RecvNormalFH[5] == 0xDD){
                if(u8RecvNormalFH[6] == 0xDD){
                    u8UpgradeFlag = 1;
                    u8FileIndex = EasyUI_FileIndexGet(MODEM_TRANSFER) - 1;
                    if(u8FileIndex < 8){
                        u8CdcTxBuffer[5] = u8FileIndex;
                        u8CdcTxBuffer[6] = 0x68 + u8FileIndex;
                        memset(&FileInfo, 0, sizeof(FileInfoStruct));
                        FileFlashAddr_Init(u8FileIndex);
                        
                    }else{
                        u8CdcTxBuffer[5] = 0xEE;
                        u8CdcTxBuffer[6] = 0x56;
                        CdcIAP_Handle();
                    }
                }else if(u8RecvNormalFH[6] == 0xE4){
                    ModemInit_Handle();
                }
            }
            RecvFH.FrameRecvStatus = FRAME_RECV_IDLE_STATUS;
        }
    }
}


void CdcIAP_Handle(void){
    u8CdcTxBuffer[0] = 0xAC;
    u8CdcTxBuffer[1] = 0x00;
    u8CdcTxBuffer[2] = 0x02;
    u8CdcTxBuffer[3] = 0xDD;
    u8CdcTxBuffer[4] = 0xDD;
    usb_vcp_send_data(&usb_core_dev, u8CdcTxBuffer, 7);
}

void ModemInit_Handle(void){
    u8CdcTxBuffer[0] = 0xAC;
    u8CdcTxBuffer[1] = 0x00;
    u8CdcTxBuffer[2] = 0x02;
    u8CdcTxBuffer[3] = 0x5A;
    u8CdcTxBuffer[4] = 0x5A;
    u8CdcTxBuffer[5] = 0x5A;
    u8CdcTxBuffer[6] = 0xBC;
    usb_vcp_send_data(&usb_core_dev, u8CdcTxBuffer, 7);
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
    u8FileDownloadStart = 1;
    u32FrameRecvOvertime = 0;
}


/**
 * @brief  文件下载计时
 * @param  None
 * @retval None
 */
void FileDownLoad_Tick(void){
    if(u8FileDownloadStart){
        u32FrameRecvOvertime++;
        if((RecvFH.FrameRecvStatus == FRAME_RECV_HEADER_STATUS) || (RecvFH.FrameRecvStatus == FRAME_RECV_DATA_STATUS)){
            // 超过10ms未接收到数据
            if(u32FrameRecvOvertime++ > 50){
                RecvFH.FrameRecvStatus = FRAME_RECV_IDLE_STATUS;
                u8FileDownloadStart = 0;
                EasyUI_TransEnd(MODEM_TRANS_FAILED);
            }
        }
        else if(RecvFH.FrameRecvStatus == FRAME_RECV_PROC_STATUS){
            // 超过4500ms未接收到数据
            if(u32FrameRecvOvertime++ > 3000){
                RecvFH.FrameRecvStatus = FRAME_RECV_IDLE_STATUS;
                u8FileDownloadStart = 0;
                EasyUI_TransEnd(MODEM_TRANS_FAILED);
            }
        }
    }
    
}
