#include "recvthread.h"

RecvModule::RecvModule(QObject* parent) : QObject(parent)
{
    qDebug()<<"main tid:RecvModule"<< QThread::currentThreadId();

    this->frameRecvStatus = FRAME_RECV_IDLE_STATUS;
    memset(&this->m_transStr, 0, sizeof(mTransStruct));
    this->m_transStr.m_appIsRuning = 0;

    this->recvTimer.setTimerType(Qt::PreciseTimer);
    this->StopRecvTimer();
    connect(&this->recvTimer, &QTimer::timeout, this, &RecvModule::RecvTimerOut);

    this->transTimer.setTimerType(Qt::PreciseTimer);
    this->StopTransTimer();
    connect(&this->transTimer, &QTimer::timeout, this, &RecvModule::TransTimerOut);

}

void RecvModule::StartRecvTimer(void ){
    this->recvTimerEnable = true;
    this->recvTimer.start(10);
}

void RecvModule::StopRecvTimer(void){
    this->recvTimerEnable = false;
    if(this->recvTimer.isActive()){
        this->recvTimer.stop();
    }
}

void RecvModule::RecvTimerOut(void){
    if(this->m_serial->qSerial->isOpen()){
        this->recvBuffer.append(this->m_serial->qSerial->readAll());
        this->RecvFunc();
    }
    this->StopRecvTimer();
}


void RecvModule::RecvFunc(void){
    quint32 bufferSize = 0;
    quint32 index = 0;
    quint8 tempData;
    quint16 crc16 = 0;
    quint8 add8 = 0;
    quint8 exitRecvLoop;
    quint8 firstCheck;
    quint8 secondCheck;
    this->frameRecvStatus = FRAME_RECV_IDLE_STATUS;
    quint8 emitflag = 0;

    bufferSize = this->recvBuffer.size();
    if(this->m_transStr.m_recvType == RECV_NORMAL){
        exitRecvLoop = 0;
        if(bufferSize == 7){
            firstCheck = 0xAC;
            secondCheck = 0x00;
        }else if(bufferSize == 10){
            firstCheck = 0xAC;
            if(this->m_serial->hostType == HOST_BOTTOM){
                secondCheck = 0x06;
            }else{
                secondCheck = 0x05;
            }
        }else if(bufferSize == 8){
            firstCheck = 0xAC;
            secondCheck = 0xEF;
        }else if(bufferSize == 16 && this->m_serial->hostType == HOST_BOTTOM){
            this->recvBuffer.remove(0, 6);
            bufferSize = 10;
            firstCheck = 0xAC;
            secondCheck = 0x06;
        }else{
            exitRecvLoop = 1;
        }
        while(index < bufferSize) {
            if(bufferSize > 32){
                break;
            }
            tempData = (quint8)this->recvBuffer[index];
            switch(this->frameRecvStatus){
            case FRAME_RECV_IDLE_STATUS:
                if(tempData == firstCheck){
                    memset(recvDataFromHost, 0, bufferSize);
                    recvDataFromHost[FRAME_HEAD_H_INDEX] = tempData;
                    this->frameRecvStatus = FRAME_RECV_HEADER_STATUS;
                }else{
                    exitRecvLoop = 1;
                }
                break;
            case FRAME_RECV_HEADER_STATUS:
                if(tempData == secondCheck){
                    recvDataFromHost[FRAME_HEAD_L_INDEX] = tempData;
                    frameDataIndex = 2;
                    this->frameRecvStatus = FRAME_RECV_DATA_STATUS;
                }else if(tempData == firstCheck){
                    recvDataFromHost[FRAME_HEAD_H_INDEX] = tempData;
                    this->frameRecvStatus = FRAME_RECV_HEADER_STATUS;
                }else{
                    this->frameRecvStatus = FRAME_RECV_IDLE_STATUS;
                    exitRecvLoop = 1;
                }
                break;
            case FRAME_RECV_DATA_STATUS:
                recvDataFromHost[frameDataIndex++] = tempData;
                if(frameDataIndex == bufferSize){
                    if(bufferSize == 8){
                        quint16 crc = 0, res = 0;
                        res = (recvDataFromHost[frameDataIndex - 2] << 8) + recvDataFromHost[frameDataIndex - 1];
                        for(quint8 i = 1; i < 6; i++){
                            crc += recvDataFromHost[i];
                        }
                        if(crc == res){
                            this->frameRecvStatus = FRAME_RECV_PROC_STATUS;
                        }else{
                            this->frameRecvStatus = FRAME_RECV_IDLE_STATUS;
                            exitRecvLoop = 1;
                        }
                    }else{
                        add8 = recvDataFromHost[frameDataIndex - 1];
                        if(add8 == Cal_ADD8(recvDataFromHost, 0, bufferSize - 1)){
                            this->frameRecvStatus = FRAME_RECV_PROC_STATUS;
                        }else{
                            this->frameRecvStatus = FRAME_RECV_IDLE_STATUS;
                            exitRecvLoop = 1;
                        }
                    }
                }
                break;
            default:

                break;
            }
            index++;
            if(exitRecvLoop == 1){
                this->frameRecvStatus = FRAME_RECV_IDLE_STATUS;
                break;
            }
        }
        if(this->frameRecvStatus == FRAME_RECV_PROC_STATUS){
            this->m_transStr.m_appIsRuning = 1;
            if(this->recvDataFromHost[1] == 0x00 && this->recvDataFromHost[2] == 0x02){
                this->m_serial->SetHostType(HOST_HANDHELD);
            }else if(this->recvDataFromHost[1] == 0x05 && this->recvDataFromHost[2] == 0x00){
                this->m_serial->SetHostType(HOST_BRUSH);
            }else if(this->recvDataFromHost[0] == 0xAC && this->recvDataFromHost[1] == 0xEF){
                this->m_serial->SetHostType(HOST_BATTERY);
            }else if(this->recvDataFromHost[1] == 0x06 && this->recvDataFromHost[2] == 0x00){
                this->m_serial->SetHostType(HOST_BOTTOM);
            }
            if(recvDataFromHost[3] == 0x5A){
                if(recvDataFromHost[4] == 0x5A){
                    if(recvDataFromHost[5] == 0x5A){
                        this->m_serial->SetIAPVersion(IAPLINK);
                    }
                }
            }
            if(this->m_serial->iapVersion == IAPLINK){
                if(recvDataFromHost[3] == 0xDD){
                    if(recvDataFromHost[4] == 0xDD){
                        if(recvDataFromHost[5] == 0xEE){
                            this->m_transStr.m_transStatus = TransAddrError;
                        }else{
                            this->m_transStr.m_recvType = RECV_XSERIES_IAP;
                            quint32 flashaddr = 0x08080000 + 51200 * recvDataFromHost[5];
                            this->m_serial->SetAppFlashAddr(flashaddr);
                            if (this->m_transStr.m_transStatus == TransBegin){
                                this->StopTransTimer();
                                this->m_transStr.m_transStatus = TransFinished;
                            }
                        }
                    }
                }
            }else if(this->m_serial->iapVersion == XSERIES){
                QString tmpString;
                if(recvDataFromHost[3] == 0xDD){
                    if(recvDataFromHost[4] == 0xDD){
                        if(recvDataFromHost[5] == 0xEE){
                            this->m_transStr.m_transStatus = TransAddrError;
                        }else{
                            this->m_transStr.m_recvType = RECV_XSERIES_IAP;
                            if(this->m_serial->hostType == HOST_BATTERY){
                                this->m_serial->combox_baudrate->setCurrentIndex(0);
                            }
                            if (this->m_transStr.m_transStatus == TransBegin){
                                this->StopTransTimer();
                                this->m_transStr.m_transStatus = TransFinished;
                            }
                        }
                    }
                }else if(recvDataFromHost[3] == 0xE0){
                    if(recvDataFromHost[5] == 1){
                        tmpString = "V2524A";
                    }else if(recvDataFromHost[5] == 2){
                        tmpString = "V2525A";
                    }else if(recvDataFromHost[5] == 3){
                        tmpString = "V2524C";
                    }else if(recvDataFromHost[5] == 4){
                        tmpString = "V2525B";
                    }else if(recvDataFromHost[5] == 5){
                        tmpString = "V2529";
                    }else if(recvDataFromHost[5] == 6){
                        tmpString = "V2524E";
                    }else if(recvDataFromHost[5] == 7){
                        tmpString = "V2525C";
                    }else if(recvDataFromHost[5] == 8){
                        tmpString = "V2525D";
                    }else if(recvDataFromHost[5] == 9){
                        tmpString = "V2555";
                    }else if(recvDataFromHost[5] == 10){
                        tmpString = "V2555A";
                    }else if(recvDataFromHost[5] == 11){
                        tmpString = "V2558";
                    }else if(recvDataFromHost[5] == 12){
                        tmpString = "V2559";
                    }else if(recvDataFromHost[5] == 13){
                        tmpString = "V2550";
                    }else if(recvDataFromHost[5] == 14){
                        tmpString = "V2593";
                    }
                    emit updateVer(tmpString);
                }else if(recvDataFromHost[3] == 0xE1){
                    tmpString = QString::number(recvDataFromHost[4]) + "D " + QString::number(recvDataFromHost[5]) + "H";
                    emit updateFilter(tmpString);
                }else if(recvDataFromHost[3] == 0xE2){
                    quint16 ret = recvDataFromHost[4] + (recvDataFromHost[5] << 8);
                    tmpString = "0x" + QString::number(ret, 16).toUpper();
                    emit updateCrc(tmpString);
                }else if(recvDataFromHost[3] == 0xE3){
                    if(recvDataFromHost[5]){
                        tmpString = "高海拔模式";
                    }else{
                        tmpString = "正常海拔模式";
                    }
                    emit updateHaiBa(tmpString);
                }
            }
            this->frameRecvStatus = FRAME_RECV_IDLE_STATUS;
            emitflag = 1;
        }
    }else if(this->m_transStr.m_recvType == RECV_ZSERIES_IAP){
        if(this->m_transStr.m_iapIngFlag){
            switch (this->m_transStr.m_transProcess) {
            case PACKET_CMD_APP_UPGRADE:
                if(this->recvBuffer.at(0) == '\xb0' ){
                    this->m_transStr.m_transStatus = TransFinished;
                }
                break;
            case PACKET_CMD_HANDSHAKE:
                if(this->recvBuffer.at(0) == '\xac' ){
                    this->m_transStr.m_transStatus = TransFinished;
                }
                break;
            case PACKET_CMD_APP_DOWNLOAD:
                this->m_transStr.m_sum8Check += ((quint8)recvBuffer[0] << 24);
                this->m_transStr.m_sum8Check += ((quint8)recvBuffer[1] << 16);
                this->m_transStr.m_sum8Check += ((quint8)recvBuffer[2] << 8);
                this->m_transStr.m_sum8Check += ((quint8)recvBuffer[3]);
                this->m_transStr.m_transStatus = TransFinished;
                break;
            default:
                break;
            }
        }
    }else if(this->m_transStr.m_recvType == RECV_XSERIES_IAP){
        exitRecvLoop = 0;
        quint8 firstCheck = 0xAC;
        quint8 secondCheck = 0x6D;
        if(this->m_serial->hostType >= HOST_BRUSH ){
            firstCheck = 0x6D;
            secondCheck = 0xAC;
        }
        qDebug()<<this->recvBuffer;
        while(index < bufferSize) {
            tempData = (quint8)this->recvBuffer[index];
            switch(this->frameRecvStatus){
            case FRAME_RECV_IDLE_STATUS:
                if(tempData == firstCheck){
                    memset(m_commRecvData, 0, FRAME_MAX_SIZE);
                    m_commRecvData[FRAME_HEAD_INDEX] = tempData;
                    this->frameRecvStatus = FRAME_RECV_HEADER_STATUS;
                }
                break;
            case FRAME_RECV_HEADER_STATUS:
                if(tempData == secondCheck){
                    m_commRecvData[FRAME_HEAD_L_INDEX] = tempData;
                    frameDataIndex = FRAME_NUM_INDEX;
                    this->frameRecvStatus = FRAME_RECV_DATA_STATUS;
                }else if(tempData == firstCheck){
                    m_commRecvData[FRAME_HEAD_H_INDEX] = tempData;
                    this->frameRecvStatus = FRAME_RECV_HEADER_STATUS;
                }else{
                    this->frameRecvStatus = FRAME_RECV_IDLE_STATUS;
                }
                break;
            case FRAME_RECV_DATA_STATUS:
                m_commRecvData[frameDataIndex++] = tempData;
                if(frameDataIndex == (FRAME_NUM_INDEX + 2)){
                    if((m_commRecvData[FRAME_NUM_INDEX] != (m_commRecvData[FRAME_XORNUM_INDEX] ^ FRAME_NUM_XOR_BYTE))){
                        exitRecvLoop = 1;
                    }
                }else if(frameDataIndex == (FRAME_LENGTH_INDEX + 2)){
                    frameSize = m_commRecvData[FRAME_LENGTH_INDEX] + (m_commRecvData[FRAME_LENGTH_INDEX + 1] << 8) + FRAME_SHELL_SIZE;
                    if((frameSize < FRAME_MIN_SIZE) || (frameSize > FRAME_MAX_SIZE)){
                        exitRecvLoop = 1;
                    }
                }else if((frameDataIndex > (FRAME_LENGTH_INDEX + 2)) && (frameDataIndex == frameSize)){
                    crc16 = m_commRecvData[frameDataIndex - 2] + (m_commRecvData[frameDataIndex - 1] << 8);
                    // frameRecvStatus = FRAME_RECV_PROC_STATUS;
                    if(Cal_CRC16(m_commRecvData, FRAME_PACKET_INDEX, (frameSize - FRAME_SHELL_SIZE)) == crc16){
                        if(m_commRecvData[PACKET_CMD_INDEX] == PACKET_CMD_FLASH_CRC){
                            this->m_transStr.m_crcFlash = m_commRecvData[PACKET_DATA_INDEX] + (m_commRecvData[PACKET_DATA_INDEX + 1] << 8);
                        }
                        this->frameRecvStatus = FRAME_RECV_PROC_STATUS;
                    }else{
                        exitRecvLoop = 1;
                    }
                }
                break;
            }
            index++;
            if(exitRecvLoop == 1){
                this->frameRecvStatus = FRAME_RECV_IDLE_STATUS;
                break;
            }
        }
        if(this->frameRecvStatus == FRAME_RECV_PROC_STATUS){
            this->m_transStr.m_transNumber++;
            if(this->m_transStr.m_transNumber == 0){
                this->m_transStr.m_transNumber = 1;
            }
            // memset(m_commRecvData + frameDataIndex - 2, 0, 2);
            this->CommModemHandler(m_commRecvData);
            this->frameRecvStatus = FRAME_RECV_IDLE_STATUS;
            emitflag = 2;
        }
    // }else if(this->m_transStr.m_recvType == RECV_ONEWIRE_IAP){
    //     exitRecvLoop = 0;
    //     while(index < bufferSize) {
    //         tempData = (quint8)this->recvBuffer[index];
    //         switch(frameRecvStatus){
    //         case FRAME_RECV_IDLE_STATUS:
    //             if(tempData == 0x6D){
    //                 memset(m_commRecvData, 0, FRAME_MAX_SIZE);
    //                 m_commRecvData[FRAME_HEAD_INDEX] = tempData;
    //                 frameRecvStatus = FRAME_RECV_HEADER_STATUS;
    //             }
    //             break;
    //         case FRAME_RECV_HEADER_STATUS:
    //             if(tempData == 0xAC){
    //                 m_commRecvData[FRAME_HEAD_L_INDEX] = tempData;
    //                 frameDataIndex = FRAME_NUM_INDEX;
    //                 frameRecvStatus = FRAME_RECV_DATA_STATUS;
    //             }else if(tempData == 0x6D){
    //                 m_commRecvData[FRAME_HEAD_H_INDEX] = tempData;
    //                 frameRecvStatus = FRAME_RECV_HEADER_STATUS;
    //             }else{
    //                 frameRecvStatus = FRAME_RECV_IDLE_STATUS;
    //             }
    //             break;
    //         }

    //     }

    }
    if(emitflag == 1){
        emit this->recvSignal(this->recvBuffer);
    }else if(emitflag == 2){
        QByteArray data = QByteArray(reinterpret_cast<char*>(m_commRecvData), frameDataIndex);
        emit this->recvSignal(data);
    }

    this->recvBuffer.clear();
}


void RecvModule::CommModemHandler(quint8* frameData){
    quint8 retSta;
    quint8 transSta;
    retSta = frameData[PACKET_RESULT_INDEX];
    if (this->m_transStr.m_transStatus == TransBegin){
        this->StopTransTimer();
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
        this->m_transStr.m_transStatus = transSta;
    }
}


void RecvModule::TransTimerOut(void){
    if (this->m_transStr.m_transStatus == TransBegin){
        // this->m_transStr.m_transStatus = TransTimeout;
    }
}


void RecvModule::StopTransTimer(void){
    this->transTimerEnable = false;
    if(this->transTimer.isActive()){
        this->transTimer.stop();
    }
}


void RecvModule::StartTransTimer(qint32 value){
    this->transTimerEnable = true;
    this->transTimer.start(value);
}



void RecvModule::ConnectRx(void)
{
    connect(this->m_serial->qSerial, &QSerialPort::readyRead, this, &RecvModule::StartRecvTimer, Qt::BlockingQueuedConnection);
}


void RecvModule::DisConnectRx(void)
{
    disconnect(this->m_serial->qSerial, &QSerialPort::readyRead, this, &RecvModule::StartRecvTimer);
}


void RecvModule::ClearRecvBuffer(void){
    memset(this->m_commRecvData, 0, FRAME_MAX_SIZE);
}


void RecvModule::ResetIAPModule(void){
    memset(&this->m_transStr, 0, sizeof(mTransStruct));
}

