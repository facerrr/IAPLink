#ifndef RECVMODULE_H
#define RECVMODULE_H

#include <QObject>
#include <QThread>
#include <QTimer>
#include <QComboBox>
#include "config.h"
#include "mserialport.h"


class RecvModule : public QObject
{
    Q_OBJECT
public:
    explicit RecvModule(QObject *parent = nullptr);
    MSerialPort* m_serial = nullptr;
    ModelInfoStruct* model_info;
    QTimer recv_timer;
    quint8 recv_timer_en;
    QTimer trans_timer;
    quint8 trans_timer_en;
    qint32 period;
    QByteArray recv_buffer;
    IAPTransStruct iap_trans;

    quint8 iap_recv_buffer[FRAME_MAX_SIZE];
    quint8 comm_recv_buffer[32];

    quint8 frame_recv_status;
    quint32 frame_data_index;
    quint32 frame_size;

    void ConnectRx(void);
    void DisconnectRx(void);
    void ClearRecvBuffer(void);
    void StopRecvTimer(void);
    void CommModemHandler(quint8* frame_data);
    void ResetIAPModule(void);

    void SetSerialAddr(MSerialPort* serial_addr){
        this->m_serial = serial_addr;
    }

    void SetModeInfoAddr(ModelInfoStruct* info_addr){
        this->model_info = info_addr;
    }

public slots:
    void RecvTimerOut(void);
    void TransTimerOut(void);
    void StartRecvTimer(void);
    void StopTransTimer(void);
    void StartTransTimer(int value);
    void RecvFunc(void);

signals:
    void RecvDataSignal(const QByteArray& data);
    void InquiryUpdate(quint8 index, const QString& value);
};


class RecvThread : public QThread
{
    Q_OBJECT
public:
    void run(void)
    {
        qDebug()<<"main tid:RecvThread run"<< QThread::currentThreadId();
        exec();
    }
signals:
public slots:

};


#endif // RECVMODULE_H
