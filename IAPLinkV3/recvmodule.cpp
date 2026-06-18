#include "recvmodule.h"

RecvModule::RecvModule(QObject* parent) : QObject(parent)
{
    qDebug()<<"main tid:RecvModule"<< QThread::currentThreadId();

    this->frame_recv_status = FRAME_RECV_IDLE_STATUS;
    memset(&this->iap_trans, 0, sizeof(IAPTransStruct));
    this->iap_trans.is_app_program = 0;

    this->recv_timer.setTimerType(Qt::PreciseTimer);
    this->StopRecvTimer();
    connect(&this->recv_timer, &QTimer::timeout, this, &RecvModule::RecvTimerOut);

    this->trans_timer.setTimerType(Qt::PreciseTimer);
    this->StopTransTimer();
    connect(&this->trans_timer, &QTimer::timeout, this, &RecvModule::TransTimerOut);
}

void RecvModule::StartRecvTimer(void ){
    this->recv_timer_en = true;
    this->recv_timer.start(10);
}

void RecvModule::StopRecvTimer(void){
    this->recv_timer_en = false;
    if(this->recv_timer.isActive()){
        this->recv_timer.stop();
    }
}

void RecvModule::RecvTimerOut(void){
    if(this->m_serial->qSerial->isOpen()){
        this->recv_buffer.append(this->m_serial->qSerial->readAll());
        this->RecvFunc();
    }
    this->StopRecvTimer();
}


void RecvModule::RecvFunc(void){
    quint32 buffer_size = 0;
    quint32 index = 0;
    quint8 temp_data = 0;
    quint8 exit_recv = 0;
    quint8 first_check = 0,  second_check = 0;
    this->frame_recv_status = FRAME_RECV_IDLE_STATUS;
    quint8 emit_flag = 0;
    quint8 check_offset = 0;

    quint16 crc16 = 0;
    quint8 add8 = 0;


    buffer_size = this->recv_buffer.size();
    if(this->iap_trans.recv_type == RECV_NORMAL){
        exit_recv = 0;
        first_check = this->model_info->normal_rx_check_value.at(0);
        second_check = this->model_info->normal_rx_check_value.at(1);
        quint8 check_len = this->model_info->normal_rx_check_length;
        memset(this->comm_recv_buffer, 0, sizeof(this->comm_recv_buffer));
        while(index < buffer_size){
            temp_data = (quint8)this->recv_buffer[index];
            switch(this->frame_recv_status){
            case FRAME_RECV_IDLE_STATUS:
                if(temp_data == first_check){
                    memset(this->comm_recv_buffer, 0 ,check_len);
                    comm_recv_buffer[FRAME_HEAD_H_INDEX] = temp_data;
                    this->frame_recv_status = FRAME_RECV_HEADER_STATUS;
                }
                break;
            case FRAME_RECV_HEADER_STATUS:
                if(temp_data == second_check){
                    comm_recv_buffer[FRAME_HEAD_L_INDEX] = temp_data;
                    frame_data_index = 2;
                    this->frame_recv_status = FRAME_RECV_DATA_STATUS;
                    check_offset = index - 1;
                }else if(temp_data == first_check){
                    comm_recv_buffer[FRAME_HEAD_H_INDEX] = temp_data;
                    this->frame_recv_status = FRAME_RECV_HEADER_STATUS;
                }else{
                    this->frame_recv_status = FRAME_RECV_IDLE_STATUS;
                }
                break;
            case FRAME_RECV_DATA_STATUS:
                comm_recv_buffer[frame_data_index++] = temp_data;
                if(frame_data_index == check_len){
                    if(this->model_info->verify_mode == VERIFY_CRC16){

                    }else if(this->model_info->verify_mode == VERIFY_ADD8){
                        quint8 add8 = this->comm_recv_buffer[frame_data_index - 1];
                        if(add8 == Cal_ADD8(this->comm_recv_buffer, 0, check_len - 1)){
                            this->frame_recv_status = FRAME_RECV_PROC_STATUS;
                        }else{
                            this->frame_recv_status = FRAME_RECV_IDLE_STATUS;
                            exit_recv = 1;
                        }
                    }else if(this->model_info->verify_mode == VERIFY_ADD16){
                        quint16 add16 = (this->comm_recv_buffer[frame_data_index - 2] << 8) + this->comm_recv_buffer[frame_data_index - 1];
                        quint16 crc = Cal_ADD16(this->comm_recv_buffer, 1 , check_len - 3);
                        if(add16 == crc){
                            this->frame_recv_status = FRAME_RECV_PROC_STATUS;
                        }else{
                            this->frame_recv_status = FRAME_RECV_IDLE_STATUS;
                            exit_recv = 1;
                        }
                    }
                }
                break;
            default:
                break;
            }
            index++;
            if(exit_recv == 1){
                this->frame_recv_status = FRAME_RECV_IDLE_STATUS;
                break;
            }
        }
        if(this->frame_recv_status == FRAME_RECV_PROC_STATUS){
            this->iap_trans.is_app_program = 1;
            if(this->comm_recv_buffer[3] == 0x5A){
                if(this->comm_recv_buffer[4] == 0x5A){
                    if(this->comm_recv_buffer[5] == 0x5A){
                        this->model_info->verison = IAPLINK;
                    }
                }
            }

            if(this->model_info->verison == IAPLINK){
                if(this->comm_recv_buffer[3] == 0xDD){
                    if(this->comm_recv_buffer[4] == 0xDD){
                        if(this->comm_recv_buffer[5] == 0xEE){
                            this->iap_trans.trans_status = TransAddrError;
                        }else{
                            this->iap_trans.recv_type = RECV_XSERIES_IAP;
                            quint32 flashaddr = 0x08080000 + 51200 * comm_recv_buffer[5];
                            this->model_info->app_flash_st_addr = flashaddr;
                            if (this->iap_trans.trans_status == TransBegin){
                                this->StopTransTimer();
                                this->iap_trans.trans_status = TransFinished;
                            }
                        }
                    }
                }
            }else if(this->model_info->verison == XSERIES){
                QString tmpString;
                quint16 ret;
                if(this->iap_trans.is_upgrading == 1){
                    QByteArrayView view1(reinterpret_cast<const char*>(this->comm_recv_buffer), this->model_info->normal_rx_check_length);
                    if(view1 == this->model_info->handshake_check_buffer){
                        this->iap_trans.recv_type = RECV_XSERIES_IAP;
                        if(this->model_info->upgrade_baudrate != this->model_info->normal_rx_baudrate){
                            int bt_index = this->m_serial->combox_baudrate->findData(static_cast<int>(this->model_info->upgrade_baudrate));
                            if (bt_index != -1) {
                                this->m_serial->combox_baudrate->setCurrentIndex(bt_index);
                            }
                        }
                        if (this->iap_trans.trans_status == TransBegin){
                            this->StopTransTimer();
                            this->iap_trans.trans_status = TransFinished;
                        }
                    }
                }

                if(this->model_info->inquiry_enable == 1){
                    for(quint8 k = 0; k < 4; k++){
                        quint8 res_check_index = this->model_info->inquiry_res_check[k].at(0);
                        quint8 res_check_val = this->model_info->inquiry_res_check[k].at(1);
                        if(res_check_index < this->model_info->normal_rx_check_length){
                            if(this->comm_recv_buffer[res_check_index] == res_check_val){
                                switch(this->model_info->inquiry_res_check[k].at(2)){
                                case 0x00:
                                    // 直接使用
                                    if(this->model_info->inquiry_res_check[k].at(4) != 0){
                                        if(this->model_info->inquiry_res_check[k].at(3) !=0){
                                            tmpString.append(QString::number(this->comm_recv_buffer[4]));
                                            tmpString.append(" ");
                                        }
                                        tmpString.append(QString::number(this->comm_recv_buffer[5]));
                                    }else{
                                        if(this->model_info->inquiry_res_check[k].at(3) !=0){
                                            tmpString.append(QString::number(this->comm_recv_buffer[4]));
                                        }
                                    }
                                    break;
                                case 0x01:{
                                    // 低8位 + 高8位
                                    quint16 ret = this->comm_recv_buffer[4] + (this->comm_recv_buffer[5] << 8);
                                    tmpString = "0x" + QString::number(ret, 16).toUpper();
                                    break;
                                }
                                case 0x02:{
                                    quint16 ret = this->comm_recv_buffer[5]+ (this->comm_recv_buffer[4] << 8);
                                    tmpString = "0x" + QString::number(ret, 16).toUpper();
                                    // 高8位 + 低8位
                                    break;
                                }
                                case 0x03:{
                                    // 相加
                                    break;
                                }
                                case 0x0F:{
                                    // 自定义字符 + 数字
                                    tmpString = QString::number(this->comm_recv_buffer[4]) + this->model_info->inquiry_res_check[k].at(3) + " " +QString::number(this->comm_recv_buffer[5]) + this->model_info->inquiry_res_check[k].at(4);

                                    break;
                                }
                                }
                                emit this->InquiryUpdate(k, tmpString);
                            }
                        }

                    }
                }
            }

            this->frame_recv_status = FRAME_RECV_IDLE_STATUS;
            emit_flag = 1;
        }
    }else if(this->iap_trans.recv_type == RECV_ZSERIES_IAP){
        if(this->iap_trans.is_upgrading){
            switch (this->iap_trans.trans_process) {
            case PACKET_CMD_APP_UPGRADE:
                if(this->recv_buffer.at(0) == '\xb0' ){
                    this->iap_trans.trans_status = TransFinished;
                }
                break;
            case PACKET_CMD_HANDSHAKE:
                if(this->recv_buffer.at(0) == '\xac' ){
                    this->iap_trans.trans_status = TransFinished;
                }
                break;
            case PACKET_CMD_APP_DOWNLOAD:
                this->iap_trans.add8_flash += ((quint8)recv_buffer[0] << 24);
                this->iap_trans.add8_flash += ((quint8)recv_buffer[1] << 16);
                this->iap_trans.add8_flash += ((quint8)recv_buffer[2] << 8);
                this->iap_trans.add8_flash += ((quint8)recv_buffer[3]);
                this->iap_trans.trans_status = TransFinished;
                break;
            default:
                break;
            }
        }

    }else if(this->iap_trans.recv_type == RECV_XSERIES_IAP){
        exit_recv = 0;
        quint8 firstCheck = 0xAC;
        quint8 secondCheck = 0x6D;
        if(this->model_info->host_type >= HOST_BRUSH ){
            firstCheck = 0x6D;
            secondCheck = 0xAC;
        }
        qDebug()<<this->recv_buffer;
        while(index < buffer_size) {
            temp_data = (quint8)this->recv_buffer[index];
            switch(this->frame_recv_status){
            case FRAME_RECV_IDLE_STATUS:
                if(temp_data == firstCheck){
                    memset(iap_recv_buffer, 0, FRAME_MAX_SIZE);
                    iap_recv_buffer[FRAME_HEAD_INDEX] = temp_data;
                    this->frame_recv_status = FRAME_RECV_HEADER_STATUS;
                }
                break;
            case FRAME_RECV_HEADER_STATUS:
                if(temp_data == secondCheck){
                    iap_recv_buffer[FRAME_HEAD_L_INDEX] = temp_data;
                    frame_data_index = FRAME_NUM_INDEX;
                    this->frame_recv_status = FRAME_RECV_DATA_STATUS;
                }else if(temp_data == firstCheck){
                    iap_recv_buffer[FRAME_HEAD_H_INDEX] = temp_data;
                    this->frame_recv_status = FRAME_RECV_HEADER_STATUS;
                }else{
                    this->frame_recv_status = FRAME_RECV_IDLE_STATUS;
                }
                break;
            case FRAME_RECV_DATA_STATUS:
                iap_recv_buffer[frame_data_index++] = temp_data;
                if(frame_data_index == (FRAME_NUM_INDEX + 2)){
                    if((iap_recv_buffer[FRAME_NUM_INDEX] != (iap_recv_buffer[FRAME_XORNUM_INDEX] ^ FRAME_NUM_XOR_BYTE))){
                        exit_recv = 1;
                    }
                }else if(frame_data_index == (FRAME_LENGTH_INDEX + 2)){
                    frame_size = iap_recv_buffer[FRAME_LENGTH_INDEX] + (iap_recv_buffer[FRAME_LENGTH_INDEX + 1] << 8) + FRAME_SHELL_SIZE;
                    if((frame_size < FRAME_MIN_SIZE) || (frame_size > FRAME_MAX_SIZE)){
                        exit_recv = 1;
                    }
                }else if((frame_data_index > (FRAME_LENGTH_INDEX + 2)) && (frame_data_index == frame_size)){
                    crc16 = iap_recv_buffer[frame_data_index - 2] + (iap_recv_buffer[frame_data_index - 1] << 8);
                    // frame_recv_status = FRAME_RECV_PROC_STATUS;
                    if(Cal_CRC16(iap_recv_buffer, FRAME_PACKET_INDEX, (frame_size - FRAME_SHELL_SIZE)) == crc16){
                        if(iap_recv_buffer[PACKET_CMD_INDEX] == PACKET_CMD_FLASH_CRC){
                            this->iap_trans.crc_flash = iap_recv_buffer[PACKET_DATA_INDEX] + (iap_recv_buffer[PACKET_DATA_INDEX + 1] << 8);
                        }
                        this->frame_recv_status = FRAME_RECV_PROC_STATUS;
                    }else{
                        exit_recv = 1;
                    }
                }
                break;
            }
            index++;
            if(exit_recv == 1){
                this->frame_recv_status = FRAME_RECV_IDLE_STATUS;
                break;
            }
        }
        if(this->frame_recv_status == FRAME_RECV_PROC_STATUS){
            this->iap_trans.trans_numble++;
            if(this->iap_trans.trans_numble == 0){
                this->iap_trans.trans_numble = 1;
            }
            // memset(m_commRecvData + frameDataIndex - 2, 0, 2);
            this->CommModemHandler(this->iap_recv_buffer);
            this->frame_recv_status = FRAME_RECV_IDLE_STATUS;
            emit_flag = 2;
        }
    }
    if(emit_flag == 1){
        emit this->RecvDataSignal(QByteArray(reinterpret_cast<const char*>(comm_recv_buffer), this->model_info->normal_rx_check_length));
    }else if(emit_flag == 2){
        QByteArray data = QByteArray(reinterpret_cast<char*>(iap_recv_buffer), frame_data_index);
        emit this->RecvDataSignal(data);
    }
    this->recv_buffer.clear();
}



void RecvModule::CommModemHandler(quint8* frame_data){
    quint8 ret_status;
    quint8 trans_status;
    ret_status = frame_data[PACKET_RESULT_INDEX];
    if (this->iap_trans.trans_status == TransBegin){
        this->StopTransTimer();
        switch(ret_status)
        {
        case PACKET_ACK_OK:
            trans_status = TransFinished;
            break;
        case PACKET_ACK_ERROR:
            trans_status = TransFailed;
            break;
        case PACKET_ACK_ADDR_ERROR:
            trans_status = TransAddrError;
            break;
        default:
            trans_status = TransFailed;
            break;
        }
        this->iap_trans.trans_status = trans_status;
    }
}


void RecvModule::TransTimerOut(void){
    if (this->iap_trans.trans_status == TransBegin){
        // this->iap_trans.trans_status = TransTimeout;
    }
}


void RecvModule::StopTransTimer(void){
    this->trans_timer_en = false;
    if(this->trans_timer.isActive()){
        this->trans_timer.stop();
    }
}


void RecvModule::StartTransTimer(qint32 value){
    this->trans_timer_en = true;
    this->trans_timer.start(value);
}


void RecvModule::ConnectRx(void)
{
    connect(this->m_serial->qSerial, &QSerialPort::readyRead, this, &RecvModule::StartRecvTimer, Qt::BlockingQueuedConnection);
}


void RecvModule::DisconnectRx(void)
{
    disconnect(this->m_serial->qSerial, &QSerialPort::readyRead, this, &RecvModule::StartRecvTimer);
}


void RecvModule::ClearRecvBuffer(void){
    memset(this->iap_recv_buffer, 0, FRAME_MAX_SIZE);
}


void RecvModule::ResetIAPModule(void){
    memset(&this->iap_trans, 0, sizeof(IAPTransStruct));
}
