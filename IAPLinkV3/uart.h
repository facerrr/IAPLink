#ifndef UART_H
#define UART_H

#include <QObject>
#include "mserialport.h"
#include "iapmodule.h"
#include "recvmodule.h"

class Uart : public QObject
{
    Q_OBJECT
public:
    explicit Uart(QObject *parent = nullptr);
    ~Uart(void);
    MSerialPort m_serial;
    MSerialThread serial_thread;

    RecvModule recv_module;
    RecvThread recv_thread;

    IAPModule iap_module;
    IAPThread iap_thread;

    void ShareModelInfo(ModelInfoStruct * addr);


};

#endif // UART_H
