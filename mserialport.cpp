   #include "mserialport.h"

MSerialPort::MSerialPort(QObject *parent) : QObject(parent)
{
    qDebug()<<"main tid:MSerialPort"<< QThread::currentThreadId();
    this->qSerial        = new QSerialPort;
    this->mSerialConfig  = new mSerialConfigStruct;
    this->m_appFlashAddr = 0;
}


MSerialPort::~MSerialPort(void)
{
    delete this->qSerial;
    delete this->mSerialConfig;
    delete [] this->mSerialComInfo;
};


void MSerialPort::SerialComScanf(void)
{
    qint32 SerialComCnt  = 0;
    this->mSerialPortCnt = 0;

    if(this->mSerialComInfo!=nullptr)
    {
        delete [] this->mSerialComInfo;
    }

    this->mSerialPortCnt = QSerialPortInfo::availablePorts().length();
    this->mSerialComInfo = new mSerialComInfoStruct[this->mSerialPortCnt];
    SerialComCnt = 0;
    foreach(const QSerialPortInfo &port, QSerialPortInfo::availablePorts())
    {
        this->mSerialComInfo[SerialComCnt].SerialName      = port.portName();
        this->mSerialComInfo[SerialComCnt].ProductCode     = port.productIdentifier();
        this->mSerialComInfo[SerialComCnt].SyestemPosition = port.systemLocation();
        this->mSerialComInfo[SerialComCnt].SerialNumStr    = port.serialNumber();
        this->mSerialComInfo[SerialComCnt].DescribeStr     = port.description();
        this->mSerialComInfo[SerialComCnt].Manufacturer    = port.manufacturer();
        this->mSerialComInfo[SerialComCnt].SupplierCode    = port.vendorIdentifier();
        SerialComCnt++;
    }
}

void MSerialPort::SerialOpen(bool & isOpen)
{
    this->SerialClose();
    this->qSerial->setPortName(this->mSerialConfig->mSerialPortName);
    this->qSerial->setBaudRate(this->mSerialConfig->mSerialBaudRate);
    this->qSerial->setStopBits(this->mSerialConfig->mSerialStopBits);
    this->qSerial->setDataBits(this->mSerialConfig->mSerialDataBits);
    this->qSerial->setParity(this->mSerialConfig->mSerialParrity);
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

void MSerialPort::SetAppFlashAddr(const quint32 addr){
    this->m_appFlashAddr = addr;
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
