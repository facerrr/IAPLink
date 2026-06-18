#include "iapmodule.h"

IAPModule::IAPModule(QObject *parent) : QObject(parent)
{
    qDebug()<<"main tid:IAPModule"<< QThread::currentThreadId();
    this->m_reIapFlag = 0;
    this->txByte1 = QByteArray::fromHex("AC 02 00 00 00 00 00 DD DD DD 00 45");
    this->txByte2 = QByteArray::fromHex("A5 5A");
    this->txByte3 = QByteArray::fromHex("59");
    this->txByte4 = QByteArray::fromHex("4E");
}

IAPModule::~IAPModule(void)
{
};

void IAPModule::IAPLoop_ZSeries(void){
    quint8 threadSta = TransIdle;
    qint32 fileLength = 0, fileIndex = 0;
    qint32 downTotalEntries = 0, downProgressVal = 0;
    quint8 transBuffer[PACKET_DATA_SIZE];
    QByteArray chunk;

    fileLength = m_iapFileLength;
    if(fileLength == 0){
        this->m_transStr->m_iapIngFlag = 0;
        threadSta = TransFileInvalid;
    }else{
        downTotalEntries = fileLength / ZSERIES_IAP_SIZE;
        if ((fileLength % ZSERIES_IAP_SIZE) != 0){ downTotalEntries += 1; }
    }
    this->m_transStr->m_transProcess = PACKET_CMD_APP_UPGRADE;
    this->m_transStr->m_recvType = RECV_ZSERIES_IAP;
    while (this->m_transStr->m_iapIngFlag){
        switch(this->m_transStr->m_transProcess){
        case PACKET_CMD_APP_UPGRADE:
            if(this->m_transStr->m_appIsRuning){
                emit this->transSend(this->txByte1);
                this->m_transStr->m_transStatus = TransBegin;
                while(this->m_transStr->m_transStatus == TransBegin);
                if(this->m_transStr->m_transStatus == TransFinished){
                    this->m_transStr->m_transProcess = PACKET_CMD_HANDSHAKE;
                }else{
                    this->m_transStr->m_transStatus = TransFailed;
                    emit this->timerStop();
                }
            }else{
                this->m_transStr->m_transProcess = PACKET_CMD_HANDSHAKE;
            }
            break;
        case PACKET_CMD_HANDSHAKE:
            emit this->transSend(this->txByte2);
            this->m_transStr->m_transStatus = TransBegin;
            while(this->m_transStr->m_transStatus == TransBegin);
            if(this->m_transStr->m_transStatus == TransFinished){
                this->m_transStr->m_transProcess = PACKET_CMD_APP_DOWNLOAD;
            }else{
                this->m_transStr->m_transStatus = TransFailed;
                emit this->timerStop();
            }
            break;
        case PACKET_CMD_APP_DOWNLOAD:
            if (fileLength > ZSERIES_IAP_SIZE){
                memcpy(transBuffer, m_iapFilePtr + fileIndex, ZSERIES_IAP_SIZE);
                CommModemSendData(transBuffer, ZSERIES_IAP_SIZE, 3000);
            }else{
                memcpy(transBuffer, m_iapFilePtr + fileIndex, fileLength);
                CommModemSendData(transBuffer, fileLength, 2000);
            }

            if (fileLength > ZSERIES_IAP_SIZE){
                fileIndex += ZSERIES_IAP_SIZE;
                fileLength -= ZSERIES_IAP_SIZE;
                downProgressVal++;
                QThread::msleep(50);
                CommProgress(downTotalEntries, downProgressVal);

            }else{
                this->m_transStr->m_transStatus = TransBegin;
                while(this->m_transStr->m_transStatus == TransBegin);
                if(this->m_transStr->m_transStatus == TransFinished){
                    this->m_transStr->m_transProcess = PACKET_CMD_JUMP_TO_APP;
                }else{
                    this->m_transStr->m_transStatus = TransFailed;
                    emit this->timerStop();
                }
            }
            break;
        case PACKET_CMD_JUMP_TO_APP:
            if(this->m_transStr->m_sum8Check == this->sum8Check){
                emit this->transSend(this->txByte3);
                threadSta = TransFinished;
                CommProgress(downTotalEntries, downTotalEntries);
            }else{
                emit this->transSend(this->txByte4);
                threadSta = TransFailed;
                CommProgress(downTotalEntries, 0);
            }
            break;
        default:
            break;
        }
        if ((this->m_transStr->m_transStatus == TransTimeout)   ||
            (this->m_transStr->m_transStatus == TransFailed)    ||
            (this->m_transStr->m_transStatus == TransAbort)     ||
            (this->m_transStr->m_transStatus == TransAddrError))
        {
            threadSta = this->m_transStr->m_transStatus;
            break;
        }
        if(threadSta != TransIdle){
            break;
        }
    }
    emit this->timerStop();
    emit this->transFinish(threadSta, 0);
    this->m_transStr->m_iapIngFlag = 0;
}

void IAPModule::IAPLoop_XSeries(void){
    quint32 flashAddr = this->m_serial->m_appFlashAddr;
    quint8 threadSta = TransIdle;
    quint8 transBuffer[PACKET_DATA_SIZE];
    qint32 fileLength = 0, fileIndex = 0, transFileSize = 0;
    qint32 downTotalEntries = 0, downProgressVal = 0;
    bool txResult = false;
    quint16 crc16 = 0;
    QByteArray fileNameData = m_firmwareFileName.toUtf8();
    quint16 fileNameLen = fileNameData.size() < PACKET_DATA_SIZE ? fileNameData.size() : PACKET_DATA_SIZE;

    fileLength = m_iapFileLength;
    if(fileLength == 0){
        this->m_transStr->m_iapIngFlag = 0;
        threadSta = TransFileInvalid;
    }else{
        transFileSize = fileLength;
        downTotalEntries = fileLength / PACKET_DATA_SIZE;
        if ((fileLength % PACKET_DATA_SIZE) != 0){ downTotalEntries += 1; }
    }

    this->m_transStr->m_transProcess = PACKET_CMD_APP_UPGRADE;

    while(this->m_transStr->m_iapIngFlag){
        switch(this->m_transStr->m_transProcess){
        case PACKET_CMD_HANDSHAKE:
            memcpy(transBuffer, fileNameData.constData(), fileNameLen);
            txResult = CommModemPackget(PACKET_CMD_HANDSHAKE, PACKET_TYPE_CONTROL, 0, transBuffer, fileNameLen, 1000);
            if(txResult){
                while(this->m_transStr->m_transStatus == TransBegin);
                if(this->m_transStr->m_transStatus == TransFinished){
                    this->m_transStr->m_transProcess = PACKET_CMD_ERASE_FLASH;
                }
            }else{
                this->m_transStr->m_transStatus = TransFailed;
                emit this->timerStop();
            }
            break;
        case PACKET_CMD_ERASE_FLASH:
            transBuffer[0] = (quint8)(transFileSize & 0x000000ff);
            transBuffer[1] = (quint8)((transFileSize >> 8) & 0x000000ff);
            transBuffer[2] = (quint8)((transFileSize >> 16) & 0x000000ff);
            transBuffer[3] = (quint8)((transFileSize >> 24) & 0x000000ff);
            txResult = CommModemPackget(PACKET_CMD_ERASE_FLASH, PACKET_TYPE_DATA, this->m_serial->m_appFlashAddr, transBuffer, 4, 5000);
            if(txResult){
                while (this->m_transStr->m_transStatus == TransBegin) ;
                if (this->m_transStr->m_transStatus == TransFinished){
                    this->m_transStr->m_transProcess = PACKET_CMD_APP_DOWNLOAD;
                    downProgressVal = 0;
                    CommProgress(downTotalEntries, downProgressVal);
                    fileIndex = 0;
                }
            }else{
                this->m_transStr->m_transStatus = TransFailed;
                emit this->timerStop();
            }
            break;
        case PACKET_CMD_APP_DOWNLOAD:
            if (fileLength > PACKET_DATA_SIZE){
                memcpy(transBuffer, m_iapFilePtr + fileIndex, PACKET_DATA_SIZE);
                txResult = CommModemPackget(PACKET_CMD_APP_DOWNLOAD, PACKET_TYPE_DATA, flashAddr, transBuffer, PACKET_DATA_SIZE, 2000);
            }else{
                memcpy(transBuffer, m_iapFilePtr + fileIndex, fileLength);
                txResult = CommModemPackget(PACKET_CMD_APP_DOWNLOAD, PACKET_TYPE_DATA, flashAddr, transBuffer, fileLength, 2000);
            }
            if(txResult){
                while(this->m_transStr->m_transStatus == TransBegin);
                if(this->m_transStr->m_transStatus == TransFinished){
                    if (fileLength > PACKET_DATA_SIZE){
                        fileIndex += PACKET_DATA_SIZE;
                        fileLength -= PACKET_DATA_SIZE;
                        flashAddr += PACKET_DATA_SIZE;
                        downProgressVal++;
                        CommProgress(downTotalEntries, downProgressVal);
                    }else{
                        // m_iapFileLength = 0;
                        this->m_transStr->m_transProcess = PACKET_CMD_FLASH_CRC;
                    }
                }
            }else{
                this->m_transStr->m_transStatus = TransFailed;
                emit this->timerStop();
            }
            break;
        case PACKET_CMD_JUMP_TO_APP:
            txResult = CommModemPackget(PACKET_CMD_JUMP_TO_APP, PACKET_TYPE_CONTROL, 0, NULL, 0, 1000);
            if (txResult){
                while (this->m_transStr->m_transStatus == TransBegin) ;
                if (this->m_transStr->m_transStatus == TransFinished)
                {
                    CommProgress(downTotalEntries, downTotalEntries);
                    threadSta = TransFinished;
                }
            }else{
                this->m_transStr->m_transStatus = TransFailed;
                emit this->timerStop();
            }
            break;
        case PACKET_CMD_FLASH_CRC:
            transBuffer[0] = (quint8)(transFileSize & 0x000000ff);
            transBuffer[1] = (quint8)((transFileSize >> 8) & 0x000000ff);
            transBuffer[2] = (quint8)((transFileSize >> 16) & 0x000000ff);
            transBuffer[3] = (quint8)((transFileSize >> 24) & 0x000000ff);
            txResult = CommModemPackget(PACKET_CMD_FLASH_CRC, PACKET_TYPE_DATA,  this->m_serial->m_appFlashAddr, transBuffer, 4, 3000);
            if(txResult){
                while (this->m_transStr->m_transStatus == TransBegin) ;
                if (this->m_transStr->m_transStatus == TransFinished){
                    crc16 = this->m_transStr->m_crcFlash;
                    if (crc16 == Cal_CRC16(m_iapFilePtr, 0, (quint32)transFileSize)){
                        this->m_transStr->m_transProcess = PACKET_CMD_JUMP_TO_APP;
                    }else{
                        this->m_transStr->m_transStatus = TransFailed;
                    }
                }
            }else{
                this->m_transStr->m_transStatus = TransFailed;
                emit this->timerStop();
            }
            break;
        case PACKET_CMD_APP_UPGRADE:
            if((this->m_transStr->m_appIsRuning || this->m_serial->iapVersion == IAPLINK || this->m_serial->hostType == HOST_BOTTOM) && this->m_reIapFlag == 0){
                txResult = CommModemPackget(PACKET_CMD_APP_UPGRADE, PACKET_TYPE_CONTROL, this->m_serial->m_appFlashAddr, NULL, 0, 1000);
                if (txResult){
                    while (this->m_transStr->m_transStatus == TransBegin);
                    if (this->m_transStr->m_transStatus == TransFinished)
                    {
                        this->m_transStr->m_transProcess = PACKET_CMD_HANDSHAKE;
                        /* Wait for MCU reset */
                        QThread::msleep(200);
                        this->m_transStr->m_recvType = RECV_XSERIES_IAP;
                        flashAddr = this->m_serial->m_appFlashAddr;
                    }
                }else{
                    this->m_transStr->m_transStatus = TransFailed;
                    emit this->timerStop();
                }
            }else{
                this->m_transStr->m_transProcess = PACKET_CMD_HANDSHAKE;
                this->m_transStr->m_recvType = RECV_XSERIES_IAP;
            }
            break;
        default:
            break;

        }
        if ((this->m_transStr->m_transStatus == TransTimeout)   ||
            (this->m_transStr->m_transStatus == TransFailed)    ||
            (this->m_transStr->m_transStatus == TransAbort)     ||
            (this->m_transStr->m_transStatus == TransAddrError))
        {
            threadSta = this->m_transStr->m_transStatus;
            break;
        }
        // finished
        if (threadSta == TransFinished)
        {
            break;
        }
    }
    emit this->timerStop();
    this->m_transStr->m_iapIngFlag = 0;
    emit this->transFinish(threadSta, this->m_transStr->m_transProcess);
}


void IAPModule::IAP_Func(quint8 version){
    if(version == 0){
        this->IAPLoop_ZSeries();
    }else{
        this->IAPLoop_XSeries();
    }
}


bool IAPModule::CommModemPackget(quint8 cmd, quint8 type, quint32 addr, quint8* data, quint16 length, qint32 timeout){
    quint16 index;
    quint8 txData[FRAME_MAX_SIZE] = {0};
    quint16 u16Head = (quint16)FRAME_HEAD;
    // if(this->m_serial->hostType == HOST_BRUSH){
    //     u16Head = 0xAC6D;
    // }
    quint16 frameHeadLength = (quint16)FRAME_SHELL_SIZE - 2;
    quint16 controlLength = (quint16)PACKET_INSTRUCT_SIZE;
    quint16 packetHeadLength = (quint16)(controlLength + frameHeadLength);
    quint16 totalLength = (quint16)(length + controlLength);
    quint16 crc16;

    index = 0;
    if(cmd == PACKET_CMD_APP_UPGRADE){
        if(this->m_serial->hostType == HOST_HANDHELD){
            txData[index++] = 0xAC;
            txData[index++] = 0x02;
            txData[index++] = 0x00;
            txData[index++] = 0x00;
            txData[index++] = 0xDD;
            txData[index++] = 0xDD;
            txData[index++] = 0xDD;
            txData[index++] = (quint8)(addr & 0x000000FF);
            txData[index++] = (quint8)((addr >> 8) & 0x000000FF);
            txData[index++] = (quint8)((addr >> 16) & 0x000000FF);
            txData[index++] = (quint8)((addr >> 24) & 0x000000FF);

            txData[index++] = Cal_ADD8(txData, 0 , 11);
        }else if(this->m_serial->hostType == HOST_BRUSH){
            txData[index++] = 0xAC;
            txData[index++] = 0x00;
            txData[index++] = 0x05;
            txData[index++] = 0xDD;
            txData[index++] = 0xDD;
            txData[index++] = Cal_ADD8(txData, 0 , 5);
        }else if(this->m_serial->hostType == HOST_BOTTOM){
            txData[index++] = 0xAC;
            txData[index++] = 0x05;
            txData[index++] = 0x06;
            txData[index++] = 0xDD;
            txData[index++] = 0xDD;
            txData[index++] = Cal_ADD8(txData, 0 , 5);
        }else if(this->m_serial->hostType == HOST_BATTERY){
            txData[index++] = 0xAC;
            txData[index++] = 0xFE;
            txData[index++] = 0x01;
            txData[index++] = 0xFE;
            txData[index++] = 0x01;
            txData[index++] = 0xFD;
        }
        emit this->clearRecvData();
    }else{
        txData[index++] = (quint8)(u16Head & 0x00FF);
        txData[index++] = (quint8)((u16Head >> 8) & 0x00FF);
        txData[index++] = this->m_transStr->m_transNumber;
        txData[index++] = (quint8)(this->m_transStr->m_transNumber ^ (quint8)FRAME_NUM_XOR_BYTE);
        txData[index++] = (quint8)(totalLength & 0x00FF);
        txData[index++] = (quint8)(totalLength >> 8);

        txData[index++] = cmd;
        txData[index++] = type;
        txData[index++] = (quint8)(addr & 0x000000FF);
        txData[index++] = (quint8)((addr >> 8) & 0x000000FF);
        txData[index++] = (quint8)((addr >> 16) & 0x000000FF);
        txData[index++] = (quint8)((addr >> 24) & 0x000000FF);

        for (int i = index; i < packetHeadLength; i++)
        {
            txData[i] = 0x00;
        }

        index = packetHeadLength;
        if(length){
            memcpy(&txData[index], data, length);
        }
        index += length;
        crc16 = Cal_CRC16(txData, frameHeadLength, totalLength);
        txData[index++] = (quint8)(crc16 & 0x00FF);
        txData[index++] = (quint8)(crc16 >> 8);
        emit this->clearRecvData();
        // memset(this->m_transStr->data, 0 , FRAME_MAX_SIZE);
    }

    return CommModemSendData(txData, index, timeout);
}


bool IAPModule::CommModemSendData(quint8* transStr, quint16 length, qint32 timeout){
    QByteArray txData;
    for(quint16 i = 0; i < length; i++){
        txData.append((char)transStr[i]);
    }
    qDebug()<<txData;
    emit this->transSend(txData);
    emit this->timerStart(timeout);
    this->m_transStr->m_transStatus = TransBegin;
    return true;
}


void IAPModule::IAPFile_Init(const QByteArray& fileData, qint32 fileLength){
    this->sum8Check = 0;
    this->m_iapFilePtr = new quint8[fileLength];
    for(qint32 i = 0; i < fileLength; i++){
        m_iapFilePtr[i] = (quint8)(fileData[i]);
        this->sum8Check += m_iapFilePtr[i];
    }

    this->crc16 = Cal_CRC16(m_iapFilePtr, 0, fileLength);
    this->m_iapFileLength = fileLength;
}


void IAPModule::CommProgress(int total, int currval){
    int perValue = (int)(currval * 100 / total);
    emit this->upgradeBar(perValue);
}

IAPThread::IAPThread(QObject *parent) : QThread(parent)
{

}
