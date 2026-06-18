#include "uart.h"

Uart::Uart(QObject *parent) : QObject(parent)
{
    this->m_iapModule.SetSerialAddr(&this->m_serial);
    this->m_recvModule.SetSerialAddr(&this->m_serial);
    this->m_iapModule.SetTransStrAddr(&this->m_recvModule.m_transStr);

    connect(&this->m_iapModule, &IAPModule::timerStart, &this->m_recvModule, &RecvModule::StartTransTimer);
    connect(&this->m_iapModule, &IAPModule::timerStop, &this->m_recvModule, &RecvModule::StopTransTimer);
    connect(&this->m_iapModule, &IAPModule::transSend, &this->m_serial, &MSerialPort::WriteData);

    this->m_serial.setObjectName("MSerial");
    this->m_serial.moveToThread(&this->m_comThread);
    this->m_serial.qSerial->moveToThread(&this->m_comThread);

    this->m_iapModule.setObjectName("IAPModule");
    this->m_iapModule.moveToThread(&this->m_iapThread);


    this->m_recvModule.setObjectName("RecvModule");
    this->m_recvModule.moveToThread(&this->m_recvThread);
    this->m_recvModule.recvTimer.moveToThread(&this->m_recvThread);
    this->m_recvModule.transTimer.moveToThread(&this->m_recvThread);

    this->m_comThread.start();
    this->m_iapThread.start();
    this->m_recvThread.start();
}

Uart::~Uart(void)
{
    this->m_comThread.exit();
    this->m_comThread.wait();
    this->m_iapThread.exit();
    this->m_iapThread.wait();
    this->m_recvThread.exit();
    this->m_recvThread.wait();

}





