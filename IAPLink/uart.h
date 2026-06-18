#ifndef UART_H
#define UART_H


#include <QObject>
#include "mserialport.h"
#include "iapmodule.h"
#include "recvthread.h"

#include <QObject>

class Uart : public QObject
{
    Q_OBJECT
public:
    explicit Uart(QObject *parent = nullptr);
    ~Uart(void);
    MSerialPort m_serial;
    MSerialThread m_comThread;
    IAPModule m_iapModule;
    IAPThread m_iapThread;
    RecvThread m_recvThread;
    RecvModule m_recvModule;
};

#endif // UART_H
