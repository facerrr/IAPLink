#include "uart.h"

Uart::Uart(QObject *parent) : QObject(parent)
{
    this->recv_module.SetSerialAddr(&this->m_serial);
    this->iap_module.SetSerialAddr(&this->m_serial);
    this->iap_module.SetTransStrAddr(&this->recv_module.iap_trans);

    connect(&this->iap_module, &IAPModule::TimerStartSignal, &this->recv_module, &RecvModule::StartTransTimer);
    connect(&this->iap_module, &IAPModule::TimerStopSignal, &this->recv_module, &RecvModule::StopTransTimer);
    connect(&this->iap_module, &IAPModule::TransDataSignal, &this->m_serial, &MSerialPort::WriteData);

    this->m_serial.setObjectName("MSerial");
    this->m_serial.moveToThread(&this->serial_thread);
    this->m_serial.qSerial->moveToThread(&this->serial_thread);


    this->recv_module.setObjectName("RecvModule");
    this->recv_module.moveToThread(&this->recv_thread);
    this->recv_module.recv_timer.moveToThread(&this->recv_thread);
    this->recv_module.trans_timer.moveToThread(&this->recv_thread);

    this->iap_module.setObjectName("IAPModule");
    this->iap_module.moveToThread(&this->iap_thread);


    this->serial_thread.start();
    this->recv_thread.start();
    this->iap_thread.start();
}

void Uart::ShareModelInfo(ModelInfoStruct * addr){
    this->recv_module.SetModeInfoAddr(addr);
    this->iap_module.SetModelInforAddr(addr);
}


Uart::~Uart(void)
{
    this->serial_thread.exit();
    this->serial_thread.wait();

    this->recv_thread.exit();
    this->recv_thread.wait();

    this->iap_thread.exit();
    this->iap_thread.wait();
}
