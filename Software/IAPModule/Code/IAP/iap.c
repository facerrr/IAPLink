#include "main.h"
#include "easy_ui.h"

uint8_t u8IAPTxBuffer[FRAME_MAX_SIZE];
uint8_t u8IAPRxBuffer[FRAME_MAX_SIZE];

TransStruct IAPTransStr;
TimerStruct IAPTimer;
LoadingPmStruct LoadingPm;

uint8_t u8IAPOutFlag;

uint8_t* iapFilePtr;
volatile uint32_t u32AppFlashAddr;
uint32_t u32IAPFileLength;

uint32_t u32ZSeriesTransTick;

uint8_t u8App2BootCmd[12] = {0xAC, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0xDD, 0xDD, 0xDD, 0x00, 0x45};


uint8_t CommModemPackget(uint8_t cmd, uint8_t type, uint32_t addr, uint8_t* data, uint16_t length, uint32_t timeout);
uint8_t CommModemSendData(uint8_t* pu8Data, uint16_t length, uint32_t timeout);
/**
 * @brief  IAP模块初始化
 * @param  None
 * @retval None
 */
void IAPModule_Init(void){
    IAPValue_Init();
    RecvValue_Init(u8IAPRxBuffer, &IAPTransStr, &LoadingPm);
}


/**
 * @brief  IAP值初始化
 * @param  None
 * @retval None
 */
void IAPValue_Init(void){
    memset(u8IAPTxBuffer, 0, FRAME_MAX_SIZE);
    memset(u8IAPRxBuffer, 0, FRAME_MAX_SIZE);
    memset(&IAPTransStr, 0, sizeof(TransStruct));
    memset(&IAPTimer, 0, sizeof(TimerStruct));
    memset(&LoadingPm, 0, sizeof(LoadingPmStruct));
    u8IAPOutFlag = 0;
    u32AppFlashAddr = 0;
    u32IAPFileLength = 0;
}


/**
 * @brief  IAP定时器
 * @param  None
 * @retval None
 */
void IAP_Tick1ms(void){
    if(IAPTimer.timerEn){
        IAPTimer.timerTick++;
        if(IAPTimer.timerTick >= IAPTimer.timeout){
            IAPTimer.timerTick = IAPTimer.timeout;
            IAPTransStr.transStatus = TransTimeout;
        }
    }   
    if(LoadingPm.transProcess == PACKET_CMD_APP_DOWNLOAD){
        u32ZSeriesTransTick++;
    }
}


/**
 * @brief  IAP Func
 * @param  None
 * @retval None
 */
void IAPFunc(void){
    uint8_t txResult;
    uint32_t checkSum;
    if(IAPTransStr.iapIngFlag){
        if(IAPTransStr.iapVerion == ZSERIES){
            switch(LoadingPm.transProcess){
            case PACKET_CMD_APP_UPGRADE:
                if(IAPTransStr.appIsRuning){
                    if(!LoadingPm.sendAlready){
                        txResult = CommModemSendData(u8App2BootCmd, 12, 1000);
                        LoadingPm.sendAlready = 1;
                        IAPTransStr.recvType = RECV_ZSERIES_IAP;
                        delay_ms(10);
                        IAPTimer.timerTick = 0;
                    }else{
                        if(txResult){
                            if(IAPTransStr.transStatus == TransFinished){
                                LoadingPm.sendAlready = 0;
                                LoadingPm.transProcess = PACKET_CMD_HANDSHAKE;
                            }
                        }else{
                            IAPTransStr.transStatus = TransFailed;
                            StopTimer();
                        }
                    }
                }else{
                    LoadingPm.transProcess = PACKET_CMD_HANDSHAKE;
                    IAPTransStr.recvType = RECV_ZSERIES_IAP;
                }
                break;
            case PACKET_CMD_HANDSHAKE:
                if(!LoadingPm.sendAlready){
                    u8IAPTxBuffer[0] = 0xA5;
                    u8IAPTxBuffer[1] = 0x5A;
                    txResult = CommModemSendData(u8IAPTxBuffer, 2, 1000);
                    LoadingPm.sendAlready = 1;
                }else{
                    if(txResult){
                        if(IAPTransStr.transStatus == TransFinished){
                            LoadingPm.sendAlready = 0;
                            LoadingPm.transProcess = PACKET_CMD_APP_DOWNLOAD;
                            u32ZSeriesTransTick = 50;
                        }
                    }else{
                        IAPTransStr.transStatus = TransFailed;
                        StopTimer();
                    }
                }
                break;
            case PACKET_CMD_APP_DOWNLOAD:
                if(u32ZSeriesTransTick > 50){
                    u32ZSeriesTransTick = 0;
                    if(LoadingPm.fileLength > IAP_ZSERIES_SIZE){
                        memcpy(LoadingPm.transBuffer, iapFilePtr + LoadingPm.fileIndex, IAP_ZSERIES_SIZE);
                        txResult = CommModemSendData(LoadingPm.transBuffer, IAP_ZSERIES_SIZE, 2000);
                    }else{
                        memcpy(LoadingPm.transBuffer, iapFilePtr + LoadingPm.fileIndex, LoadingPm.fileLength);
                        txResult = CommModemSendData(LoadingPm.transBuffer, LoadingPm.fileLength, 2000);
                    }
                    if(LoadingPm.fileLength > IAP_ZSERIES_SIZE){
                        LoadingPm.fileIndex += IAP_ZSERIES_SIZE;
                        LoadingPm.fileLength -= IAP_ZSERIES_SIZE;
                    }else{
                        LoadingPm.transProcess = PACKET_CMD_FLASH_CRC;
                        LoadingPm.add8 = Cal_ADD8(iapFilePtr, 0, (uint32_t)LoadingPm.transFileSize);
                    }
                }
                break;
            case PACKET_CMD_FLASH_CRC:
                if(txResult){
                    if(IAPTransStr.transStatus == TransFinished){
                        if(IAPTransStr.sum8check == LoadingPm.add8){
                            u8IAPTxBuffer[0] = 0x59;
                            u8IAPOutFlag = 0;
                        }else{
                            u8IAPTxBuffer[0] = 0x4E;
                            u8IAPOutFlag = 1;
                        }
                        
                        txResult = CommModemSendData(u8IAPTxBuffer, 1, 1000);
                        if(txResult){
                            LoadingPm.transProcess = 0;
                            LoadingPm.threadSta = TransFinished;
                            if(u8IAPOutFlag){
                                IAPTransStr.transStatus  = TransFailed;
                            } 
                            
                        }else{
                            IAPTransStr.transStatus = TransFailed;
                            StopTimer();
                        }
                    }
                }else{
                    IAPTransStr.transStatus = TransFailed;
                    StopTimer();
                }
                break;
            default:
                break;
            }
        }else{
            switch(LoadingPm.transProcess){
            case PACKET_CMD_HANDSHAKE:
                if(!LoadingPm.sendAlready){
                    txResult = CommModemPackget(PACKET_CMD_HANDSHAKE, PACKET_TYPE_CONTROL, 0, NULL, 0, 1000);
                    LoadingPm.sendAlready = 1;
                }else{
                    if(txResult){
                        if(IAPTransStr.transStatus == TransFinished){
                            LoadingPm.sendAlready = 0;
                            LoadingPm.transProcess = PACKET_CMD_ERASE_FLASH;
                        }
                    }else{
                        IAPTransStr.transStatus = TransFailed;
                        StopTimer();
                    }
                }
                break;
            case PACKET_CMD_ERASE_FLASH:
                if(!LoadingPm.sendAlready){
                    LoadingPm.transBuffer[0] = (uint8_t)(LoadingPm.transFileSize & 0x000000ff);
                    LoadingPm.transBuffer[1] = (uint8_t)((LoadingPm.transFileSize >> 8) & 0x000000ff);
                    LoadingPm.transBuffer[2] = (uint8_t)((LoadingPm.transFileSize >> 16) & 0x000000ff);
                    LoadingPm.transBuffer[3] = (uint8_t)((LoadingPm.transFileSize >> 24) & 0x000000ff);
                    txResult = CommModemPackget(PACKET_CMD_ERASE_FLASH, PACKET_TYPE_DATA, u32AppFlashAddr, LoadingPm.transBuffer, 4, 5000);
                    LoadingPm.sendAlready = 1;
                }else{
                    if(txResult){
                        if (IAPTransStr.transStatus == TransFinished){
                            LoadingPm.sendAlready = 0;
                            LoadingPm.transProcess = PACKET_CMD_APP_DOWNLOAD;
                            LoadingPm.fileIndex = 0;
                        }
                    }else{
                        IAPTransStr.transStatus = TransFailed;
                        StopTimer();
                    }
                }
                break;
            case PACKET_CMD_APP_DOWNLOAD:
                if(!LoadingPm.sendAlready){
                    if (LoadingPm.fileLength > PACKET_DATA_SIZE){
                        memcpy(LoadingPm.transBuffer, iapFilePtr + LoadingPm.fileIndex, PACKET_DATA_SIZE);
                        txResult = CommModemPackget(PACKET_CMD_APP_DOWNLOAD, PACKET_TYPE_DATA, LoadingPm.flashAddr, LoadingPm.transBuffer, PACKET_DATA_SIZE, 2000);
                    }else{
                        memcpy(LoadingPm.transBuffer, iapFilePtr + LoadingPm.fileIndex, LoadingPm.fileLength);
                        txResult = CommModemPackget(PACKET_CMD_APP_DOWNLOAD, PACKET_TYPE_DATA, LoadingPm.flashAddr, LoadingPm.transBuffer, LoadingPm.fileLength, 2000);
                    }
                    LoadingPm.sendAlready = 1;
                }else{
                    if(txResult){
                        if(IAPTransStr.transStatus == TransFinished){
                            LoadingPm.sendAlready = 0;
                            if (LoadingPm.fileLength > PACKET_DATA_SIZE){
                                LoadingPm.fileIndex += PACKET_DATA_SIZE;
                                LoadingPm.fileLength -= PACKET_DATA_SIZE;
                                LoadingPm.flashAddr += PACKET_DATA_SIZE;
                            }else{
                                LoadingPm.transProcess = PACKET_CMD_FLASH_CRC;
                            }
                        }
                    }else{
                        IAPTransStr.transStatus = TransFailed;
                        StopTimer();
                    }
                }
                break;
            case PACKET_CMD_JUMP_TO_APP:
                if(!LoadingPm.sendAlready){
                    txResult = CommModemPackget(PACKET_CMD_JUMP_TO_APP, PACKET_TYPE_CONTROL, 0, NULL, 0, 1000);
                    LoadingPm.sendAlready = 1;
                }else{
                    if(txResult){
                        if(IAPTransStr.transStatus == TransFinished){
                            LoadingPm.sendAlready = 0;
                            LoadingPm.threadSta = TransFinished;
                        }
                    }else{
                        IAPTransStr.transStatus = TransFailed;
                        StopTimer();
                    }
                }
                break;
            case PACKET_CMD_FLASH_CRC:
                if(!LoadingPm.sendAlready){
                    LoadingPm.transBuffer[0] = (uint8_t)(LoadingPm.transFileSize & 0x000000ff);
                    LoadingPm.transBuffer[1] = (uint8_t)((LoadingPm.transFileSize >> 8) & 0x000000ff);
                    LoadingPm.transBuffer[2] = (uint8_t)((LoadingPm.transFileSize >> 16) & 0x000000ff);
                    LoadingPm.transBuffer[3] = (uint8_t)((LoadingPm.transFileSize >> 24) & 0x000000ff);
                    txResult = CommModemPackget(PACKET_CMD_FLASH_CRC, PACKET_TYPE_DATA,  u32AppFlashAddr, LoadingPm.transBuffer, 4, 3000);
                    LoadingPm.sendAlready = 1;
                }else{
                    if(txResult){
                        if(IAPTransStr.transStatus == TransFinished){
                            LoadingPm.sendAlready = 0;
                            LoadingPm.crc16 = IAPTransStr.crcFlash;
                            
                            if(LoadingPm.crc16 == Cal_CRC16(iapFilePtr, 0, (uint32_t)LoadingPm.transFileSize)){
                                LoadingPm.transProcess = PACKET_CMD_JUMP_TO_APP;
                            }else{
                                IAPTransStr.transStatus = TransFailed;
                            }
                        }
                    }else{
                        IAPTransStr.transStatus = TransFailed;
                        StopTimer();
                    }
                }
                break;
            case PACKET_CMD_APP_UPGRADE:
                if(IAPTransStr.appIsRuning){
                    if(!LoadingPm.sendAlready){
                        txResult = CommModemPackget(PACKET_CMD_APP_UPGRADE, PACKET_TYPE_CONTROL, u32AppFlashAddr, NULL, 0, 1000);
                        LoadingPm.sendAlready = 1;
                    }else{
                        if(txResult){
                            if(IAPTransStr.transStatus == TransFinished){
                                LoadingPm.sendAlready = 0;
                                IAPTransStr.recvType = RECV_XSERIES_IAP;
                                LoadingPm.transProcess = PACKET_CMD_HANDSHAKE;
                                delay_ms(200);
                            }
                        }else{
                            IAPTransStr.transStatus = TransFailed;
                            StopTimer();
                        }
                    }
                }else{
                    IAPTransStr.recvType = RECV_XSERIES_IAP;
                    LoadingPm.transProcess = PACKET_CMD_HANDSHAKE;
                }
                break;
            default:
                break;
            }
        }
        if ((IAPTransStr.transStatus == TransTimeout)   ||
            (IAPTransStr.transStatus == TransFailed)    ||
            (IAPTransStr.transStatus == TransAbort)     ||
            (IAPTransStr.transStatus == TransAddrError))
        {
            LoadingPm.threadSta = IAPTransStr.transStatus;
            LED_MOD.Mode = LED_ON;
            LED_DOWNLOAD.Mode = LED_OFF;
            u8IAPOutFlag = 1;
        }
        // finished
        if(LoadingPm.threadSta == TransFinished){
            LED_MOD.Mode = LED_OFF;
            LED_DOWNLOAD.Mode = LED_ON;
            u8IAPOutFlag = 1;
        }
    }
    if(IAPTransStr.iapIngFlag == 1 && u8IAPOutFlag){
        if(LED_MOD.Mode == LED_ON){
            EasyUI_TransEnd(IAP_TRANS_FAILED);
        }else{
            EasyUI_TransEnd(IAP_TRANS_SUCCESS);
        }
        IAPTransStr.iapIngFlag = 0;
        StopTimer();
        memset(&IAPTransStr, 0, sizeof(TransStruct));
    }
}



/**
 * @brief  发送数据打包
 * @param  cmd：命令 type：类型 addr：地址 data：数据 length：数据长度 timeout：超时时间
 * @retval None
 */
uint8_t CommModemPackget(uint8_t cmd, uint8_t type, uint32_t addr, uint8_t* data, uint16_t length, uint32_t timeout){
    uint16_t index;
    uint16_t u16Head = FRAME_HEAD;
    uint16_t frameHeadLength = FRAME_SHELL_SIZE - 2;
    uint16_t controlLength = PACKET_INSTRUCT_SIZE;
    uint16_t packetHeadLength = controlLength + frameHeadLength;
    uint16_t totalLength = length + controlLength;
    uint16_t crc16;

    index = 0;
    if(cmd == PACKET_CMD_APP_UPGRADE){
        u8IAPTxBuffer[index++] = 0xAC;
        u8IAPTxBuffer[index++] = 0x02;
        u8IAPTxBuffer[index++] = 0x00;
        u8IAPTxBuffer[index++] = 0x00;
        u8IAPTxBuffer[index++] = 0xDD;
        u8IAPTxBuffer[index++] = 0xDD;
        u8IAPTxBuffer[index++] = 0xDD;
        u8IAPTxBuffer[index++] = (uint8_t)(addr & 0x000000FF);
        u8IAPTxBuffer[index++] = (uint8_t)((addr >> 8) & 0x000000FF);
        u8IAPTxBuffer[index++] = (uint8_t)((addr >> 16) & 0x000000FF);
        u8IAPTxBuffer[index++] = (uint8_t)((addr >> 24) & 0x000000FF);

        u8IAPTxBuffer[index++] = Cal_ADD8(u8IAPTxBuffer, 0 , 11);
    }else{
        u8IAPTxBuffer[index++] = (uint8_t)(u16Head & 0x00FF);
        u8IAPTxBuffer[index++] = (uint8_t)((u16Head >> 8) & 0x00FF);
        u8IAPTxBuffer[index++] = IAPTransStr.transNumber;
        u8IAPTxBuffer[index++] = (uint8_t)(IAPTransStr.transNumber ^ (uint8_t)FRAME_NUM_XOR_BYTE);
        u8IAPTxBuffer[index++] = (uint8_t)(totalLength & 0x00FF);
        u8IAPTxBuffer[index++] = (uint8_t)(totalLength >> 8);

        u8IAPTxBuffer[index++] = cmd;
        u8IAPTxBuffer[index++] = type;
        u8IAPTxBuffer[index++] = (uint8_t)(addr & 0x000000FF);
        u8IAPTxBuffer[index++] = (uint8_t)((addr >> 8) & 0x000000FF);
        u8IAPTxBuffer[index++] = (uint8_t)((addr >> 16) & 0x000000FF);
        u8IAPTxBuffer[index++] = (uint8_t)((addr >> 24) & 0x000000FF);

        for (int i = index; i < packetHeadLength; i++)
        {
            u8IAPTxBuffer[i] = 0x00;
        }

        index = packetHeadLength;
        if(length){
            memcpy(&u8IAPTxBuffer[index], data, length);
        }
        index += length;
        crc16 = Cal_CRC16(u8IAPTxBuffer, frameHeadLength, totalLength);
        u8IAPTxBuffer[index++] = (uint8_t)(crc16 & 0x00FF);
        u8IAPTxBuffer[index++] = (uint8_t)(crc16 >> 8);
    }
    return CommModemSendData(u8IAPTxBuffer, index, timeout);
}


/**
 * @brief  IAP发送
 * @param  pu8Data：数据 length：数据长度 timeout：超时时间
 * @retval None
 */
uint8_t CommModemSendData(uint8_t* pu8Data, uint16_t length, uint32_t timeout){
    uint16_t u16Index = 0;
    uint32_t u32TimeoutCnt = 0;
    uint16_t u16Data;
    for(u16Index = 0; u16Index < length; u16Index++){
        while(!usart_flag_get(UART_IAP, USART_TDBE_FLAG)){
        }
        UART_IAP->dt = (*pu8Data & 0x01FF);
        pu8Data++;
    }
    UART_IAP->sts = ~USART_TDC_FLAG;
    IAPTransStr.transStatus = TransBegin;
    IAPTimer.timerEn = 1;
    IAPTimer.timeout = timeout;
    IAPTimer.timerTick = 0;
    LED_MOD.Mode = LED_OFF;
    LED_DOWNLOAD.Mode = LED_FLASH;
    return 1;
}


/**
 * @brief  IAP参数初始化
 * @param  None
 * @retval None
 */
void IAPRam_Init(unsigned char index){
    uint8_t appflag = IAPTransStr.appIsRuning;
    memset(&IAPTransStr, 0, sizeof(TransStruct));
    if(appflag){
        IAPTransStr.appIsRuning = 1;
    }
    memset(&LoadingPm, 0, sizeof(LoadingPmStruct));
    u32AppFlashAddr = 0x5000;
    FileFlashAddr_Init(index - 1);
    iapFilePtr = (uint8_t*)(CurrentFile.FileAddrAppSt);
    uint32_t u32FirstStartAddr = *((uint32_t*)(CurrentFile.FileAddrAppSt + 4));
    if(u32FirstStartAddr > 0x8010000 && u32FirstStartAddr < 0x8010200){
        IAPTransStr.iapVerion = ZSERIES;
    }else if(u32FirstStartAddr == 0x519D || u32FirstStartAddr == 0x5165){
        IAPTransStr.iapVerion = XSERIES;
    }else{
        EasyUI_TransEnd(IAP_TRANS_FAILED);
        return;
    }
    IAPTransStr.iapIngFlag = 1;
    u8IAPOutFlag = 0;
    LoadingPm.transProcess = PACKET_CMD_APP_UPGRADE;
    LoadingPm.fileLength = *((uint32_t*)CurrentFile.FileAddrInfoSt);
    LoadingPm.transFileSize = LoadingPm.fileLength;
    LoadingPm.flashAddr = u32AppFlashAddr;
    
}



void FileFlashAddr_Init(uint8_t index){
    CurrentFile.FileAddrAppSt = FLASH_BINK2_BASE + (IAP_FILE_SIZE * index);
    CurrentFile.FileAddrAppEd = CurrentFile.FileAddrAppSt + IAP_FILE_APP_SIZE - 1;
    CurrentFile.FileAddrInfoSt = CurrentFile.FileAddrAppEd + 1;
    CurrentFile.FileAddrInfoEd = CurrentFile.FileAddrInfoSt + IAP_FILE_INFO_SIZE - 1;
}



/**
 * @brief  停止定时器
 * @param  None
 * @retval None
 */
void StopTimer(void){
    IAPTimer.timerEn = 0;
    IAPTimer.timerTick = 0;
}

/**
 * @brief  获取IAP传输结构体指针
 */
TransStruct* IAPGetTransStr(void){
    return &IAPTransStr;
}

unsigned char IAPIsRunning(void){
    return IAPTransStr.iapIngFlag;
}





