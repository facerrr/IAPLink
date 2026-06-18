#ifndef MSERIALPORT_H
#define MSERIALPORT_H

#include <QObject>
#include <QThread>
#include <QDebug>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QTimer>
#include <QComboBox>


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
    mSerialComInfoStruct* serial_info = nullptr;
    mSerialConfigStruct* serial_config = nullptr;
    quint32 serial_port_cnt;

    QComboBox* combox_baudrate = nullptr;

    void SetComBoxAddr(QComboBox* addrBaud){
        this->combox_baudrate = addrBaud;
    }

    bool rx_hex_en = true;
    bool rx_timestamp_en = true;

private:

public slots:
    void SerialOpen(bool & isOpen);
    void SerialClose(void);
    void SerialComScanf(void);
    void WriteData(const QByteArray &data);

signals:
    void TxError(void);
    void CloseRecive(void);
    void OpenRecive(void);
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
