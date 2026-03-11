#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>
#include <QMessageBox>
#include <windows.h>
#include <dbt.h>
#include <devguid.h>
#include <setupapi.h>
#include <initguid.h>
#include "uart.h"
#include <qabstractnativeeventfilter.h>
#include <windows.h>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow, public QAbstractNativeEventFilter
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    Uart xUart;
    QLabel *labelCom;
    QLabel *labelIAP;

    uint8_t errFlag = 0;

    uint8_t mcu_type = 0;

    void StatusBarInit(void);
    void Base_Init(void);
    void PlainTextInit(void);
    void UartItemInit(void);
    void UpdateComInfo(void);
    void ReadSerialSelect(void);
    void UpdateSerial(void);
    void RxSlotSignalInit(void);
    void UartSlotSignalInit(void);
    void BinSlotSignalInit(void);
    void PlainTextTimerCfg(void);
    void IAPInit(void);
    void VersionSelect(void);

    void ButtonInit(void);

public slots:
    void UpdateShow(void);
    void SerialOpen(void);
    void SerialClose(void);
    void RxTimeStampCfg(void);
    void RxHexEnableCfg(void);
    void OpenBinFile(void);
    void IAPUpgrade(void);
    void TransThreadFinish(quint8 threadSta, quint8 threadCmd);
    void ShowUpdateHistory(void);

signals:
    void openSerial(bool & isOpen);
    void closeSerial(void);
    void rxHexEnableChanged(void);
    void timeStampEnableChanged(void);
    void iapStart(quint8 version);
    void commonSend(const QByteArray &array);

protected:
    bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result);
    void doWarning(const QString &str);

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
