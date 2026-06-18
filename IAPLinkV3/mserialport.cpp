#include "mserialport.h"

MSerialPort::MSerialPort(QObject *parent) : QObject(parent)
{
    qDebug()<<"main tid:MSerialPort"<< QThread::currentThreadId();
    this->qSerial        = new QSerialPort;
    this->serial_config  = new mSerialConfigStruct;
}


MSerialPort::~MSerialPort(void)
{
    delete this->qSerial;
    delete this->serial_config;
    delete [] this->serial_info;
};


void MSerialPort::SerialComScanf(void){
    qint32 com_cnt = 0;
    this->serial_port_cnt = 0;
    if(this->serial_info != nullptr){
        delete [] this->serial_info;
    }

    this->serial_port_cnt = QSerialPortInfo::availablePorts().length();
    this->serial_info = new mSerialComInfoStruct[this->serial_port_cnt];
    com_cnt = 0;
    foreach(const QSerialPortInfo &port, QSerialPortInfo::availablePorts()){
        this->serial_info[com_cnt].SerialName      = port.portName();
        this->serial_info[com_cnt].ProductCode     = port.productIdentifier();
        this->serial_info[com_cnt].SyestemPosition = port.systemLocation();
        this->serial_info[com_cnt].SerialNumStr    = port.serialNumber();
        this->serial_info[com_cnt].DescribeStr     = port.description();
        this->serial_info[com_cnt].Manufacturer    = port.manufacturer();
        this->serial_info[com_cnt].SupplierCode    = port.vendorIdentifier();
        com_cnt++;
    }
}

void MSerialPort::SerialOpen(bool & isOpen)
{
    this->SerialClose();
    this->qSerial->setPortName(this->serial_config->mSerialPortName);
    this->qSerial->setBaudRate(this->serial_config->mSerialBaudRate);
    this->qSerial->setStopBits(this->serial_config->mSerialStopBits);
    this->qSerial->setDataBits(this->serial_config->mSerialDataBits);
    this->qSerial->setParity(this->serial_config->mSerialParrity);
    try {
        isOpen =  (this->qSerial->open(QIODevice::ReadWrite));
    } catch (const char * msg) {
        qDebug()<<msg;
    }
}



void MSerialPort::SerialClose(void)
{
    if(this->qSerial->isOpen())
    {
        this->qSerial->close();
    }
}



///*同一时间只允许一个线程调用*/
void MSerialPort::WriteData(const QByteArray &str)
{
    this->qSerial->write(str);
}


//串口线程
MSerialThread::MSerialThread(QObject *parent) : QThread(parent)
{

}
