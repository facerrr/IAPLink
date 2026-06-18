#include "vplaintextedit.h"
#include <QDebug>
#include <QDateTime>
#include <QTextCodec>
//自定义的可以刷新大量数据的QPlainTextEdit显示窗口，定时器开启自动刷新


vSerialCodeModeEnum vSerialCodeMode = SerialCodeUtf8;
//实际为编码格式解码
QByteArray CodeDecoding(QByteArray const &qByteArr)
{
    QString tmpQStr;
    switch (vSerialCodeMode)
    {
        case SerialCodeNo     :tmpQStr = qByteArr;break;
        case SerialCodeUtf8   :tmpQStr = QTextCodec::codecForName("UTF-8")->toUnicode(qByteArr);break;
        case SerialCodeUtf16  :tmpQStr = QTextCodec::codecForName("UTF-16")->toUnicode(qByteArr);break;
        case SerialCodeGB18030:tmpQStr = QTextCodec::codecForName("GB18030")->toUnicode(qByteArr);break;
        case SerialCodeISO8859:tmpQStr = QTextCodec::codecForName("ISO 8859-1")->toUnicode(qByteArr);break;
        case SerialCodeBig5   :tmpQStr = QTextCodec::codecForName("Big5")->toUnicode(qByteArr);break;
        case SerialShiftJIS   :tmpQStr = QTextCodec::codecForName("Shift-JIS")->toUnicode(qByteArr);
        default:;break;
    }
    return tmpQStr.toUtf8();
};

static int32_t MAXRANGESIZE = 8192;
vPlainTextEdit::vPlainTextEdit(QWidget *parent) : QTextEdit(parent)
{
    this->TimerCfg = 100;//默认100，后续外层通过setTimerCfg设定时间，需要重启
    this->TimerCtr.setTimerType(Qt::PreciseTimer);
    this->TimerStop();
    connect(&this->TimerCtr,&QTimer::timeout,
            this,&vPlainTextEdit::vTimerOut);
    connect(this->verticalScrollBar(),&QAbstractSlider::actionTriggered,
            this,&vPlainTextEdit::autoScroll);
    this->setReadOnly(true);
    this->setAcceptRichText(true);
    this->clear();

    const int tabStop = 4;  // 4 characters

    QFontMetrics metrics(this->font());
    this->setTabStopDistance(tabStop * metrics.horizontalAdvance(' '));
    //默认颜色
//    this->setStyleSheet("color:#0000DF");
//  this->TimerStart();
}


vPlainTextEdit::~vPlainTextEdit(void)
{
    this->TimerStop();
    this->vShowBuff  = nullptr;
}
void vPlainTextEdit::setHexEnableAddr(bool * addr)
{
    this->hexEnable = addr;
}

void vPlainTextEdit::setTimeStampEnableAddr(bool *addr)
{
    this->timeStampEnable = addr;
}

void vPlainTextEdit::setTimerCfg(qint32 outtime)
{
    this->TimerCfg = outtime;
}
void vPlainTextEdit::TimerStart(void)
{
    this->TimerEnable = true;
    if(!this->TimerCtr.isActive())
    {
        this->TimerCtr.start(this->TimerCfg);
    }
}
void vPlainTextEdit::TimerStop(void)
{
    this->TimerEnable = false;
    if(this->TimerCtr.isActive())
    {
        this->TimerCtr.stop();
    }
}
//定时器的开启仅用于追加时的实时刷新
void vPlainTextEdit::vTimerOut(void)
{
    qint64 showBuffMaxPos;
    showBuffMaxPos = this->vShowBuff.size();
    if(this->showPosMax>showBuffMaxPos)
    {
        this->showPosMax = showBuffMaxPos;
        this->showPosMin = 0;
    }
    if((this->showPosMax<=showBuffMaxPos)&&(this->TimerEnable))
    {
        this->showPosMax = showBuffMaxPos;
        if((this->showPosMax-this->showPosMin)>MAXRANGESIZE)
        {
            //不能够在容量之内显示
            qint32 indexOf1 =
                    this->vShowBuff.indexOf("<br/>",this->showPosMax-MAXRANGESIZE);
            qint32 indexOf2 =
                    this->vShowBuff.indexOf("\n",this->showPosMax-MAXRANGESIZE);
            if(indexOf1>=0)
            {
                index_of_line = indexOf1;
            }
            else if(indexOf2>=0)
            {
                index_of_line = indexOf2;
            }else
            {
                index_of_line = -1;
            }
            if(index_of_line>=0)
            {
                this->showPosMin = index_of_line;
            }
            else
            {
                this->showPosMin = this->showPosMax-MAXRANGESIZE;
            }
        }
        this->vUpdataShow();//更新显示
        this->moveCursor(QTextCursor::End);
    }
}
void vPlainTextEdit::autoScroll(int action)
{
   if(bar->maximum() == 0)
        return;
   if(action == QAbstractSlider::SliderSingleStepAdd ||
      action == QAbstractSlider::SliderSingleStepSub||
      action == QAbstractSlider::SliderPageStepAdd||
      action == QAbstractSlider::SliderPageStepSub||
      action == QAbstractSlider::SliderMove)
   {
       qint32 value  = bar->value();
       qint32 barMax = bar->maximum();
       if((!this->TimerEnable)&&(value<barMax*0.1)&&this->showPosMin)
       {
           if(this->showPosMax-this->showPosMin<this->showPosMax/2)
           {
               qint32 indexOf1 =
                       this->vShowBuff.indexOf("<br/>",this->showPosMin*0.5);
               qint32 indexOf2 =
                       this->vShowBuff.indexOf("\n",this->showPosMin*0.5);
               if(indexOf1>=0)
               {
                   index_of_line = indexOf1;
               }
               else if(indexOf2>=0)
               {
                   index_of_line = indexOf2;
               }else
               {
                   index_of_line = -1;
               }
               if(index_of_line>=0)
               {
                   this->showPosMin = index_of_line;
               }
               else
               {
                   this->showPosMin = this->showPosMin*0.5;
               }
           }
           else
           {
               this->showPosMin = 0;
           }
           this->vUpdataShow();
           bar->setValue(bar->maximum()-barMax);
       }

       //是否开启定时器刷新
       if(value!=barMax)
       {
           TimerStop();
       }else
       {
           TimerStart();

       }
//       else if((this->TimerEnable==false)
//               &&((value==barMax))
//               &&(this->vShowBuff.size()-this->showPosMax>0))
//       {
//           TimerStart();
//       }
   }
}

void vPlainTextEdit::receivedData(const QByteArray &buffer)
{
    data.append(buffer);
    if(*this->timeStampEnable)
    {
        QString timeString;
        timeString = QDateTime::currentDateTime().toString("[hh:mm:ss.zzz]");
        this->vShowBuff.append(timeString.toUtf8());
    }
    if(!(*this->hexEnable))
    {
        this->vShowBuff.append(QString(CodeDecoding(buffer)).toUtf8());
    }else
    {
        this->vShowBuff.append((buffer).toHex(' ').toUpper()+' ');
    }
    if(*this->timeStampEnable)
    {
        this->vShowBuff.append("\r\n");
    }
}

void vPlainTextEdit::vUpdataShow(void)
{
    if(this->hexEnable==nullptr)return;
    if((this->showPosMax<=this->vShowBuff.size())&&(this->showPosMin<=this->showPosMax))
    {

        this->setPlainText(this->vShowBuff.mid(this->showPosMin,
                                                this->showPosMax));

    }
}

void vPlainTextEdit::clearBuff(void)
{
    TimerStop();
    this->showPosMin = 0;
    this->showPosMax = 0;
    this->vShowBuff.clear();
    this->clear();
    TimerStart();
}
//根据hex标志刷新窗口
void vPlainTextEdit::hexEnableChanged(void)
{
    //非文本输入切换操作
    if(this->hexEnable==nullptr)return;
    if(this->vShowBuff==nullptr)return;
    if(this->showPosMin<=this->showPosMax)
    {
        this->showPosMin = 0;
        this->showPosMax = 0;
        this->vShowBuff.clear();

        if(!(*this->hexEnable))
        {
            this->vShowBuff.append(this->data);

        }
        else
        {
            this->vShowBuff.append(this->data.toHex(' ').toUpper());
        }

        TimerStart();
    }
    this->timeStampEnableChanged();
}

void vPlainTextEdit::timeStampEnableChanged(void)
{
    if(*this->timeStampEnable)
    {
        this->vShowBuff.append("\r\n");
    }
}
