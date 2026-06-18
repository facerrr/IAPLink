#include "iapmodule.h"

IAPModule::IAPModule(QObject *parent) : QObject(parent)
{
    qDebug()<<"main tid:IAPModule"<< QThread::currentThreadId();
    this->re_iap_flag = 0;
    // this->txByte1 = QByteArray::fromHex("AC 02 00 00 00 00 00 DD DD DD 00 45");
    // this->txByte2 = QByteArray::fromHex("A5 5A");
    // this->txByte3 = QByteArray::fromHex("59");
    // this->txByte4 = QByteArray::fromHex("4E");
}


IAPModule::~IAPModule(void)
{
};



#define ZSERIES_IAP_SIZE        256
void IAPModule::IAPZSeriesLoop(void){
    quint8 threadStatus = TransIdle;
    qint32 fileLength = 0, fileIndex = 0;
    qint32 downTotalEntries = 0, downProgressVal = 0;
    quint8 transBuffer[PACKET_DATA_SIZE];
    QByteArray chunk;

    fileLength = this->iap_file_len;
    if(fileLength == 0){
        this->iap_trans->is_upgrading = 0;
        threadStatus = TransFileInvalid;
    }else{
        downTotalEntries = fileLength / ZSERIES_IAP_SIZE;
        if ((fileLength % ZSERIES_IAP_SIZE) != 0){ downTotalEntries += 1; }
    }

    QByteArray tx;

    this->iap_trans->trans_process = PACKET_CMD_APP_UPGRADE;
    this->iap_trans->recv_type = RECV_ZSERIES_IAP;
    while(this->iap_trans->is_upgrading){
        switch(this->iap_trans->trans_process){
        case PACKET_CMD_APP_UPGRADE:
            if(this->iap_trans->is_app_program){
                tx = QByteArray::fromHex("AC 02 00 00 00 00 00 DD DD DD 00 45");
                emit this->TransDataSignal(tx);
                this->iap_trans->trans_status = TransBegin;
                while(this->iap_trans->trans_status == TransBegin);
                if(this->iap_trans->trans_status == TransFinished){
                    this->iap_trans->trans_process = PACKET_CMD_HANDSHAKE;
                }else{
                    this->iap_trans->trans_status = TransFailed;
                    emit this->TimerStopSignal();
                }
            }else{
                this->iap_trans->trans_process = PACKET_CMD_HANDSHAKE;
            }
            break;
        case PACKET_CMD_HANDSHAKE:
            tx = QByteArray::fromHex("A5 5A");
            emit this->TransDataSignal(tx);
            this->iap_trans->trans_status = TransBegin;
            while(this->iap_trans->trans_status == TransBegin);
            if(this->iap_trans->trans_status == TransFinished){
                this->iap_trans->trans_process = PACKET_CMD_APP_DOWNLOAD;
            }else{
                this->iap_trans->trans_status = TransFailed;
                emit this->TimerStopSignal();
            }
            break;
        case PACKET_CMD_APP_DOWNLOAD:
            if (fileLength > ZSERIES_IAP_SIZE){
                memcpy(transBuffer, iap_file_ptr + fileIndex, ZSERIES_IAP_SIZE);
                CommModemSendData(transBuffer, ZSERIES_IAP_SIZE, 3000);
            }else{
                memcpy(transBuffer, iap_file_ptr + fileIndex, fileLength);
                CommModemSendData(transBuffer, fileLength, 2000);
            }

            if (fileLength > ZSERIES_IAP_SIZE){
                fileIndex += ZSERIES_IAP_SIZE;
                fileLength -= ZSERIES_IAP_SIZE;
                downProgressVal++;
                QThread::msleep(50);
                CommProgress(downTotalEntries, downProgressVal);

            }else{
                this->iap_trans->trans_status = TransBegin;
                while(this->iap_trans->trans_status == TransBegin);
                if(this->iap_trans->trans_status == TransFinished){
                    this->iap_trans->trans_process = PACKET_CMD_JUMP_TO_APP;
                }else{
                    this->iap_trans->trans_status = TransFailed;
                    emit this->TimerStopSignal();
                }
            }
            break;
        case PACKET_CMD_JUMP_TO_APP:
            if(this->iap_trans->add8_flash == this->add8_check){
                tx = QByteArray::fromHex("59");
                emit this->TransDataSignal(tx);
                threadStatus = TransFinished;
                CommProgress(downTotalEntries, downTotalEntries);
            }else{
                tx = QByteArray::fromHex("4E");
                emit this->TransDataSignal(tx);
                threadStatus = TransFailed;
                CommProgress(downTotalEntries, 0);
            }
            break;
        default:
            break;
        }
        if ((this->iap_trans->trans_status == TransTimeout)   ||
            (this->iap_trans->trans_status == TransFailed)    ||
            (this->iap_trans->trans_status == TransAbort)     ||
            (this->iap_trans->trans_status == TransAddrError))
        {
            threadStatus = this->iap_trans->trans_status;
            break;
        }
        if(threadStatus != TransIdle){
            break;
        }
    }
    emit this->TimerStopSignal();
    emit this->TransFinishSignal(threadStatus, 0);
    this->iap_trans->is_upgrading = 0;
}


void IAPModule::IAPXSeriesLoop(){
    quint32 flashAddr = this->model_info->app_flash_st_addr;
    quint8 threadSta = TransIdle;
    quint8 transBuffer[PACKET_DATA_SIZE];
    qint32 fileLength = 0, fileIndex = 0, transFileSize = 0;
    qint32 downTotalEntries = 0, downProgressVal = 0;
    bool txResult = false;
    quint16 crc16 = 0;
    QByteArray fileNameData = this->firmware_name.toUtf8();
    quint16 fileNameLen = fileNameData.size() < PACKET_DATA_SIZE ? fileNameData.size() : PACKET_DATA_SIZE;

    fileLength = this->iap_file_len;
    if(fileLength == 0){
        this->iap_trans->is_upgrading = 0;
        threadSta = TransFileInvalid;
    }else{
        transFileSize = fileLength;
        downTotalEntries = fileLength / PACKET_DATA_SIZE;
        if ((fileLength % PACKET_DATA_SIZE) != 0){ downTotalEntries += 1; }
    }

    this->iap_trans->trans_process = PACKET_CMD_APP_UPGRADE;

    while(this->iap_trans->is_upgrading){
        switch(this->iap_trans->trans_process){
        case PACKET_CMD_HANDSHAKE:
            memcpy(transBuffer, fileNameData.constData(), fileNameLen);
            txResult = CommModemPackget(PACKET_CMD_HANDSHAKE, PACKET_TYPE_CONTROL, 0, transBuffer, fileNameLen, 1000);
            if(txResult){
                while(this->iap_trans->trans_status == TransBegin);
                if(this->iap_trans->trans_status == TransFinished){
                    this->iap_trans->trans_process = PACKET_CMD_ERASE_FLASH;
                }
            }else{
                this->iap_trans->trans_status = TransFailed;
                emit this->TimerStopSignal();
            }
            break;
        case PACKET_CMD_ERASE_FLASH:
            transBuffer[0] = (quint8)(transFileSize & 0x000000ff);
            transBuffer[1] = (quint8)((transFileSize >> 8) & 0x000000ff);
            transBuffer[2] = (quint8)((transFileSize >> 16) & 0x000000ff);
            transBuffer[3] = (quint8)((transFileSize >> 24) & 0x000000ff);
            txResult = CommModemPackget(PACKET_CMD_ERASE_FLASH, PACKET_TYPE_DATA, this->model_info->app_flash_st_addr, transBuffer, 4, 5000);
            if(txResult){
                while (this->iap_trans->trans_status == TransBegin) ;
                if (this->iap_trans->trans_status == TransFinished){
                    this->iap_trans->trans_process = PACKET_CMD_APP_DOWNLOAD;
                    downProgressVal = 0;
                    CommProgress(downTotalEntries, downProgressVal);
                    fileIndex = 0;
                    qDebug()<<"PACKET_CMD_ERASE_FLASH";
                }
            }else{
                this->iap_trans->trans_status = TransFailed;
                emit this->TimerStopSignal();
            }
            break;
        case PACKET_CMD_APP_DOWNLOAD:
            if (fileLength > PACKET_DATA_SIZE){
                memcpy(transBuffer, this->iap_file_ptr + fileIndex, PACKET_DATA_SIZE);
                txResult = CommModemPackget(PACKET_CMD_APP_DOWNLOAD, PACKET_TYPE_DATA, flashAddr, transBuffer, PACKET_DATA_SIZE, 2000);
            }else{
                memcpy(transBuffer, this->iap_file_ptr + fileIndex, fileLength);
                txResult = CommModemPackget(PACKET_CMD_APP_DOWNLOAD, PACKET_TYPE_DATA, flashAddr, transBuffer, fileLength, 2000);
            }
            if(txResult){
                while(this->iap_trans->trans_status == TransBegin);
                if(this->iap_trans->trans_status == TransFinished){
                    if (fileLength > PACKET_DATA_SIZE){
                        fileIndex += PACKET_DATA_SIZE;
                        fileLength -= PACKET_DATA_SIZE;
                        flashAddr += PACKET_DATA_SIZE;
                        downProgressVal++;
                        CommProgress(downTotalEntries, downProgressVal);
                    }else{
                        this->iap_trans->trans_process = PACKET_CMD_FLASH_CRC;
                    }
                }
            }else{
                this->iap_trans->trans_status = TransFailed;
                emit this->TimerStopSignal();
            }
            break;
        case PACKET_CMD_JUMP_TO_APP:
            txResult = CommModemPackget(PACKET_CMD_JUMP_TO_APP, PACKET_TYPE_CONTROL, 0, NULL, 0, 1000);
            if (txResult){
                while (this->iap_trans->trans_status == TransBegin) ;
                if (this->iap_trans->trans_status == TransFinished){
                    CommProgress(downTotalEntries, downTotalEntries);
                    threadSta = TransFinished;
                }
            }else{
                this->iap_trans->trans_status = TransFailed;
                emit this->TimerStopSignal();
            }
            break;
        case PACKET_CMD_FLASH_CRC:
            transBuffer[0] = (quint8)(transFileSize & 0x000000ff);
            transBuffer[1] = (quint8)((transFileSize >> 8) & 0x000000ff);
            transBuffer[2] = (quint8)((transFileSize >> 16) & 0x000000ff);
            transBuffer[3] = (quint8)((transFileSize >> 24) & 0x000000ff);
            txResult = CommModemPackget(PACKET_CMD_FLASH_CRC, PACKET_TYPE_DATA,  this->model_info->app_flash_st_addr, transBuffer, 4, 3000);
            if(txResult){
                while (this->iap_trans->trans_status == TransBegin) ;
                if (this->iap_trans->trans_status == TransFinished){
                    crc16 = this->iap_trans->crc_flash;
                    if (crc16 == Cal_CRC16(this->iap_file_ptr, 0, (quint32)transFileSize)){
                        this->iap_trans->trans_process = PACKET_CMD_JUMP_TO_APP;
                    }else{
                        this->iap_trans->trans_status = TransFailed;
                    }
                }
            }else{
                this->iap_trans->trans_status = TransFailed;
                emit this->TimerStopSignal();
            }
            break;
        case PACKET_CMD_APP_UPGRADE:
            if((this->iap_trans->is_app_program || this->model_info->verison == IAPLINK || this->model_info->host_type == HOST_BOTTOM) && this->re_iap_flag == 0){
                txResult = CommModemPackget(PACKET_CMD_APP_UPGRADE, PACKET_TYPE_CONTROL, this->model_info->app_flash_st_addr, NULL, 0, 1000);
                if (txResult){
                    while (this->iap_trans->trans_status == TransBegin);
                    if (this->iap_trans->trans_status == TransFinished)
                    {
                        this->iap_trans->trans_process = PACKET_CMD_HANDSHAKE;
                        /* Wait for MCU reset */
                        QThread::msleep(200);
                        this->iap_trans->recv_type = RECV_XSERIES_IAP;
                        flashAddr = this->model_info->app_flash_st_addr;
                    }
                }else{
                    this->iap_trans->trans_status = TransFailed;
                    emit this->TimerStopSignal();
                }
            }else{
                this->iap_trans->trans_process = PACKET_CMD_HANDSHAKE;
                this->iap_trans->recv_type = RECV_XSERIES_IAP;
            }
            break;
        default:
            break;
        }
        if ((this->iap_trans->trans_status == TransTimeout)   ||
            (this->iap_trans->trans_status == TransFailed)    ||
            (this->iap_trans->trans_status == TransAbort)     ||
            (this->iap_trans->trans_status == TransAddrError))
        {
            threadSta = this->iap_trans->trans_status;
            break;
        }
        // finished
        if (threadSta == TransFinished)
        {
            break;
        }
    }
    emit this->TimerStopSignal();
    this->iap_trans->is_upgrading = 0;
    emit this->TransFinishSignal(threadSta, this->iap_trans->trans_process);
}


void IAPModule::IAPUpgradeFunc(quint8 version){
    if(version == 0){
        this->IAPZSeriesLoop();
    }else{
        this->IAPXSeriesLoop();
    }
}


bool IAPModule::CommModemPackget(quint8 cmd, quint8 type, quint32 addr, quint8* data, quint16 length, qint32 timeout){
    quint16 index;
    quint8 txData[FRAME_MAX_SIZE] = {0};
    quint16 u16Head = (quint16)FRAME_HEAD;

    quint16 frameHeadLength = (quint16)FRAME_SHELL_SIZE - 2;
    quint16 controlLength = (quint16)PACKET_INSTRUCT_SIZE;
    quint16 packetHeadLength = (quint16)(controlLength + frameHeadLength);
    quint16 totalLength = (quint16)(length + controlLength);
    quint16 crc16;

    index = 0;
    if(cmd == PACKET_CMD_APP_UPGRADE){
        quint8 size = this->model_info->upgrade_cmd.size();
        for(quint8 i = 0; i < size; i++){
            txData[index++] = this->model_info->upgrade_cmd.at(i);
        }
        qDebug()<<"model info"<<this->model_info->upgrade_cmd.toHex();
    }else{
        qDebug()<<"aaa"<<this->model_info->upgrade_cmd.toHex();
        txData[index++] = (quint8)(u16Head & 0x00FF);
        txData[index++] = (quint8)((u16Head >> 8) & 0x00FF);
        txData[index++] = this->iap_trans->trans_numble;
        txData[index++] = (quint8)(this->iap_trans->trans_numble ^ (quint8)FRAME_NUM_XOR_BYTE);
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
    }

    return CommModemSendData(txData, index, timeout);
}


bool IAPModule::CommModemSendData(quint8* transStr, quint16 length, qint32 timeout){
    QByteArray txData;
    for(quint16 i = 0; i < length; i++){
        uint8_t data = transStr[i];
        if(length == 526){
            if(i > 5){
                data = 0x01;
            }
        }
        txData.append(data);
    }
    // qDebug()<<"length"<<length;
    qDebug()<<"txdata:"<<txData;
    emit this->TransDataSignal(txData);
    emit this->TimerStartSignal(timeout);
    this->iap_trans->trans_status = TransBegin;
    return true;
}


void IAPModule::InitIAPFile(const QByteArray& fileData, qint32 fileLength){
    this->add8_check = 0;
    this->iap_file_ptr = new quint8[fileLength];
    for(qint32 i = 0; i < fileLength; i++){
        iap_file_ptr[i] = (quint8)(fileData[i]);
        this->add8_check += iap_file_ptr[i];
    }

    this->crc16_check = Cal_CRC16(iap_file_ptr, 0, fileLength);
    this->iap_file_len = fileLength;
}



void IAPModule::CommProgress(int total, int currval){
    int perValue = (int)(currval * 100 / total);
    emit this->UpgradeBarSignal(perValue);
}



IAPThread::IAPThread(QObject *parent) : QThread(parent)
{

}





