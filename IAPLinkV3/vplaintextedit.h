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

class VPlainTextEdit : public QTextEdit
{
    Q_OBJECT
public:
    explicit VPlainTextEdit(QWidget *parent = nullptr);
    ~VPlainTextEdit(void);
    QByteArray show_buffer;
    QByteArray data;
    QScrollBar * bar = this->verticalScrollBar();
    bool * hex_enflag = nullptr;
    bool * timestamp_enflag = nullptr;
    QTimer timer_ctr;
    bool timer_enflag;
    qint32 timer_cfg;
    qint64 show_pos_min = 0,show_pos_max=0;
    qint32 index_of_line;
    void SetHexCfgAddr(bool * addr);
    void SetTimeStampAddr(bool * addr);
    void SetTimeCfg(qint32 out_time);
    void UpdateShow(void);
    void TimerStart(void);
    void TimerStop(void);

public slots:
    void AutoScroll(int action);
    void ClearBuff(void);
    void VTimerOut(void);
    void ReceivedData(const QByteArray &buffer);
    void HexEnChanged(void);
    void TimeStampEnChanged(void);
signals:

};

#endif // VPLAINTEXTEDIT_H
