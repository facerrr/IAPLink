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
#include <qabstractnativeeventfilter.h>
#include <windows.h>

#include "uart.h"
#include "config.h"

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

    QLabel *label_com_status;
    QLabel *label_iap_status;

    void InitWindowElement(void);
    void InitUsbDevice(void);
    void InitRxPlainText(void);
    void InitUartItem(void);
    void UpdateSerial(void);
    void InitUartSlotSignal(void);
    void InitPlainTextTimer(void);
    void ReadSerialSelect(void);
    void InitSerialSelect(void);
    void InitModelInfo(quint8 idnex);
    void InitIAP(void);

public slots:
    void SerialOpen(void);
    void SerialClose(void);
    void RxTimeStampCfg(void);
    void RxHexEnableCfg(void);
    void OpenBinFile(void);
    void IAPUpgrade(void);
    void TransThreadFinish(quint8 threadSta, quint8 threadCmd);
    void OpenModelCfgDialog(void);
    void ShowUpdateHistory(void);

signals:
    void OpenSerialSignal(bool & is_open);
    void CloseSerialSignal(void);
    void RxHexEnChanged(void);
    void TimeStampChanged(void);
    void IAPUpdateStart(quint8 version);
    void DataSendSignal(const QByteArray &array);

protected:
    bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result);
    void doWarning(const QString &str);

private:
    Ui::MainWindow *ui;
    ModelInfoStruct model_info;
};
#endif // MAINWINDOW_H
