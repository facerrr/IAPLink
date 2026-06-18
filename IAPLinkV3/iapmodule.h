#ifndef IAPMODULE_H
#define IAPMODULE_H

#include <QObject>
#include <QThread>
#include <QDebug>
#include <QTimer>
#include "mserialport.h"
#include "config.h"


class IAPModule : public QObject
{
    Q_OBJECT
public:
    explicit IAPModule(QObject *parent = nullptr);
    ~IAPModule(void);
    MSerialPort* m_serial = nullptr;
    ModelInfoStruct* model_info = nullptr;
    quint8* iap_file_ptr = nullptr;
    qint32 iap_file_len;
    QString firmware_name;

    volatile IAPTransStruct* iap_trans;
    quint32 add8_check;
    quint16 crc16_check;
    quint8 re_iap_flag;


    bool CommModemPackget(quint8 cmd, quint8 type, quint32 addr, quint8* data, quint16 length, qint32 timeout);
    bool CommModemSendData(quint8* tansStr, quint16 length, qint32 timeout);

    void InitIAPFile(const QByteArray& fileData, qint32 fileLength);
    void CommProgress(int total, int currval);

    void IAPZSeriesLoop(void);
    void IAPXSeriesLoop(void);

    void SetSerialAddr(MSerialPort* Serial_t){
        this->m_serial = Serial_t;
    }

    void SetTransStrAddr(IAPTransStruct* addr){
        this->iap_trans = addr;
    }

    void SetModelInforAddr(ModelInfoStruct* addr){
        this->model_info = addr;
    }

public slots:
    void IAPUpgradeFunc(quint8 version);

signals:
    void TransFinishSignal(quint8 trans_status, quint8 cmd);
    void UpgradeBarSignal(int a);
    void TimerStartSignal(int timeout_val);
    void TimerStopSignal(void);
    void TransDataSignal(const QByteArray & array);
};


class IAPThread : public QThread
{
    Q_OBJECT
public:
    explicit IAPThread(QObject *parent = nullptr);
public:
    void run(void)
    {
        qDebug()<<"main tid:IAPThread run"<< QThread::currentThreadId();
        exec();
    }
signals:
public slots:

};





#endif // IAPMODULE_H
