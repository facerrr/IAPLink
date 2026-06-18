#ifndef RECVTHREAD_H
#define RECVTHREAD_H

#include <QObject>
#include <QThread>
#include <QTimer>
#include "config.h"
#include "mserialport.h"
#include <QComboBox>

class RecvModule : public QObject
{
    Q_OBJECT
public:
    explicit RecvModule(QObject *parent = nullptr);
    MSerialPort* m_serial = nullptr;
    QTimer recvTimer;
    quint8 recvTimerEnable;
    QTimer transTimer;
    quint8 transTimerEnable;
    qint32 period;
    QByteArray recvBuffer;

    mTransStruct m_transStr;

    quint8 m_commRecvData[FRAME_MAX_SIZE];
    quint8 recvDataFromHost[32];
    quint8 frameRecvStatus;
    quint32 frameDataIndex;
    quint32 frameSize;



    void ConnectRx(void);
    void DisConnectRx(void);
    void ClearRecvBuffer(void);
    void StopRecvTimer(void);
    void CommModemHandler(quint8* frameData);
    void ResetIAPModule(void);

    void SetSerialAddr(MSerialPort* Serial_t){
        this->m_serial = Serial_t;
    }

public slots:
    void RecvTimerOut(void);
    void TransTimerOut(void);
    void StartRecvTimer(void);
    void StopTransTimer(void);
    void StartTransTimer(int value);
    void RecvFunc(void);

signals:
    void recvSignal(const QByteArray& array);
    void updateFilter(const QString &time);
    void updateVer(const QString &ver);
    void updateCrc(const QString &crc);
    void updateHaiBa(const QString &crc);
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

#endif // RECVTHREAD_H
