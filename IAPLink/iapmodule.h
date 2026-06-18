#ifndef IAPMODULE_H
#define IAPMODULE_H

#include <QObject>
#include <QThread>
#include <QDebug>
#include <QTimer>
#include "mserialport.h"
#include "config.h"


#define ZSERIES_IAP_SIZE        256

class IAPModule : public QObject
{
    Q_OBJECT
public:
    explicit IAPModule(QObject *parent = nullptr);
    ~IAPModule(void);
    MSerialPort* m_serial = nullptr;
    quint8* m_iapFilePtr = nullptr;
    qint32 m_iapFileLength;
    QString m_firmwareFileName;

    volatile mTransStruct* m_transStr;

    QByteArray txByte1;
    QByteArray txByte2;
    QByteArray txByte3;
    QByteArray txByte4;

    quint32 sum8Check;
    quint16 crc16;

    quint8 m_reIapFlag;


    bool CommModemPackget(quint8 cmd, quint8 type, quint32 addr, quint8* data, quint16 length, qint32 timeout);
    bool CommModemSendData(quint8* tansStr, quint16 length, qint32 timeout);

    void IAPFile_Init(const QByteArray& fileData, qint32 fileLength);
    void CommProgress(int total, int currval);

    void SetSerialAddr(MSerialPort* Serial_t){
        this->m_serial = Serial_t;
    }

    void SetTransStrAddr(mTransStruct* addr){
        this->m_transStr = addr;
    }

public slots:
    void IAPLoop_ZSeries(void);
    void IAPLoop_XSeries(void);
    void IAP_Func(quint8 version);

signals:
    void transFinish(quint8 transSta, quint8 cmd);
    void upgradeBar(int a);
    void timerStart(int a);
    void timerStop(void);
    void transSend(const QByteArray &array);
    void clearRecvData(void);
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
