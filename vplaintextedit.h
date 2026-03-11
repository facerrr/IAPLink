#ifndef VPLAINTEXTEDIT_H
#define VPLAINTEXTEDIT_H

#include <QTextEdit>
#include <QTimer>
#include <QScrollBar>

typedef enum
{
    SerialCodeNo = 0,
    SerialCodeUtf8,     //Utf8编码格式
    SerialCodeUtf16,    //Utf16编码格式
    SerialCodeGB18030,  //GBK编码格式、兼容GBK18030
    SerialCodeISO8859,  //IOS8859-1
    SerialCodeBig5,     //Big5
    SerialShiftJIS,
}vSerialCodeModeEnum;

QByteArray CodeDecoding(QByteArray const &qByteArr);

class vPlainTextEdit : public QTextEdit
{
    Q_OBJECT
public:
    explicit vPlainTextEdit(QWidget *parent = nullptr);
    ~vPlainTextEdit(void);
    QByteArray vShowBuff;
    QByteArray data;
    QScrollBar * bar = this->verticalScrollBar();
    bool       * hexEnable = nullptr;
    bool       * timeStampEnable = nullptr;
    QTimer TimerCtr;
    bool   TimerEnable;
    qint32 TimerCfg;
    qint64 showPosMin=0,showPosMax=0;
    qint32 index_of_line;
    void setHexEnableAddr(bool * addr);
    void setTimeStampEnableAddr(bool * addr);
    void setTimerCfg(qint32 outtime);
    void vUpdataShow(void);
    void TimerStart(void);
    void TimerStop(void);

public slots:
    void autoScroll(int action);
    void clearBuff(void);
    void vTimerOut(void);
    void receivedData(const QByteArray &buffer);
    void hexEnableChanged(void);
    void timeStampEnableChanged(void);
signals:

};

#endif // VPLAINTEXTEDIT_H
