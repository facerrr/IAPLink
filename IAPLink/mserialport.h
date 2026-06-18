#ifndef MSERIALPORT_H
#define MSERIALPORT_H

#include <QObject>
#include <QThread>
#include <QDebug>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QTimer>
#include <QComboBox>
#include "config.h"

//获取串口设备信息列表，以便于识别正确设备
typedef struct
{
    QString SerialName;
    qint16  ProductCode;
    QString SyestemPosition;
    QString SerialNumStr;
    QString DescribeStr;
    QString Manufacturer;
    QString SupplierCode;
}mSerialComInfoStruct;

//串口操作选中对象及初始化赋值
typedef struct
{
    QString                  mSerialPortName;
    qint32                   mSerialBaudRate;
    QSerialPort::StopBits    mSerialStopBits;
    QSerialPort::DataBits    mSerialDataBits;
    QSerialPort::Parity      mSerialParrity;
    QSerialPort::FlowControl mSerialFlowControl;
}mSerialConfigStruct;


class MSerialPort : public QObject
{
    Q_OBJECT
public:
    explicit MSerialPort(QObject *parent = nullptr);
    ~MSerialPort(void);
    QSerialPort * qSerial = nullptr;
    mSerialComInfoStruct* mSerialComInfo = nullptr;
    mSerialConfigStruct* mSerialConfig = nullptr;
    quint32 mSerialPortCnt;

    bool rxHexEnable = true;
    bool rxTimeStampEnable = true;
    quint32 iapVersion = 0;
    volatile quint32 m_appFlashAddr;
    quint8 wireType = 0;
    quint32 hostType = HOST_HANDHELD;

    QComboBox* combox_version = nullptr;
    QComboBox* combox_hostType = nullptr;
    QComboBox* combox_baudrate = nullptr;

    void SetAppFlashAddr(const quint32 addr);

    void SetComBoxAddr(QComboBox* addrVer, QComboBox* addrHost, QComboBox* addrBaud){
        this->combox_version = addrVer;
        this->combox_hostType = addrHost;
        this->combox_baudrate = addrBaud;
    }
    void SetIAPVersion(quint8 ver){
        this->iapVersion = ver;
        this->combox_version->setCurrentIndex(ver);
    }
    void SetHostType(quint8 type){
        this->hostType = type;
        this->combox_hostType->setCurrentIndex(type);
    }
    void SetBaudRate(qint32 baudrate){
        // this->qSerial->setBaudRate(baudrate);
        // quint8 val = 0;
        // if(baudrate == 115200) val = 0;
        // if(baudrate == 9600) val = 1;
        // this->combox_baudrate->setCurrentIndex(val);
    }

private:

public slots:
    void SerialOpen(bool & isOpen);
    void SerialClose(void);
    void SerialComScanf(void);
    void WriteData(const QByteArray &data);

signals:
    void txError(void);
    void closeRx(void);
    void openRx(void);
};



class MSerialThread : public QThread
{
    Q_OBJECT
public:
    explicit MSerialThread(QObject *parent = nullptr);
public:

    void run(void)
    {
        qDebug()<<"main tid:MSerialThread run"<< QThread::currentThreadId();
        exec();
    }
signals:
public slots:

};


#endif // MSERIALPORT_H
