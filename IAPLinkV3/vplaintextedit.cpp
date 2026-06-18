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
VPlainTextEdit::VPlainTextEdit(QWidget *parent) : QTextEdit(parent)
{
    this->timer_cfg = 100;  //默认100，后续外层通过setTimerCfg设定时间，需要重启
    this->timer_ctr.setTimerType(Qt::PreciseTimer);
    this->TimerStop();
    connect(&this->timer_ctr,&QTimer::timeout,
            this,&VPlainTextEdit::VTimerOut);
    connect(this->verticalScrollBar(),&QAbstractSlider::actionTriggered,
            this,&VPlainTextEdit::AutoScroll);
    this->setReadOnly(true);
    this->setAcceptRichText(true);
    this->clear();

    const int tab_stop = 4;  // 4 characters

    QFontMetrics metrics(this->font());
    this->setTabStopDistance(tab_stop * metrics.horizontalAdvance(' '));
    //默认颜色
//    this->setStyleSheet("color:#0000DF");
//  this->TimerStart();
}


VPlainTextEdit::~VPlainTextEdit(void)
{
    this->TimerStop();
    this->show_buffer = nullptr;
}
void VPlainTextEdit::SetHexCfgAddr(bool * addr)
{
    this->hex_enflag = addr;
}

void VPlainTextEdit::SetTimeStampAddr(bool *addr)
{
    this->timestamp_enflag = addr;
}

void VPlainTextEdit::SetTimeCfg(qint32 outtime)
{
    this->timer_cfg = outtime;
}

void VPlainTextEdit::TimerStart(void)
{
    this->timer_enflag = true;
    if(!this->timer_ctr.isActive())
    {
        this->timer_ctr.start(this->timer_cfg);
    }
}
void VPlainTextEdit::TimerStop(void)
{
    this->timer_enflag = false;
    if(this->timer_ctr.isActive())
    {
        this->timer_ctr.stop();
    }
}
//定时器的开启仅用于追加时的实时刷新
void VPlainTextEdit::VTimerOut(void)
{
    qint64 showBuffMaxPos;
    showBuffMaxPos = this->show_buffer.size();
    if(this->show_pos_max>showBuffMaxPos)
    {
        this->show_pos_max = showBuffMaxPos;
        this->show_pos_min = 0;
    }
    if((this->show_pos_max<=showBuffMaxPos)&&(this->timer_enflag))
    {
        this->show_pos_max = showBuffMaxPos;
        if((this->show_pos_max-this->show_pos_min)>MAXRANGESIZE)
        {
            //不能够在容量之内显示
            qint32 indexOf1 =
                    this->show_buffer.indexOf("<br/>",this->show_pos_max-MAXRANGESIZE);
            qint32 indexOf2 =
                    this->show_buffer.indexOf("\n",this->show_pos_max-MAXRANGESIZE);
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
                this->show_pos_min = index_of_line;
            }
            else
            {
                this->show_pos_min = this->show_pos_max-MAXRANGESIZE;
            }
        }
        this->UpdateShow();//更新显示
        this->moveCursor(QTextCursor::End);
    }
}
void VPlainTextEdit::AutoScroll(int action)
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
       if((!this->timer_enflag)&&(value<barMax*0.1)&&this->show_pos_min)
       {
           if(this->show_pos_max-this->show_pos_min<this->show_pos_max/2)
           {
               qint32 indexOf1 =
                       this->show_buffer.indexOf("<br/>",this->show_pos_min*0.5);
               qint32 indexOf2 =
                       this->show_buffer.indexOf("\n",this->show_pos_min*0.5);
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
                   this->show_pos_min = index_of_line;
               }
               else
               {
                   this->show_pos_min = this->show_pos_min*0.5;
               }
           }
           else
           {
               this->show_pos_min = 0;
           }
           this->UpdateShow();
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
//               &&(this->show_buffer.size()-this->show_pos_max>0))
//       {
//           TimerStart();
//       }
   }
}

void VPlainTextEdit::ReceivedData(const QByteArray &buffer)
{
    data.append(buffer);
    if(*this->timestamp_enflag)
    {
        QString timeString;
        timeString = QDateTime::currentDateTime().toString("[hh:mm:ss.zzz]");
        this->show_buffer.append(timeString.toUtf8());
    }
    if(!(*this->hex_enflag))
    {
        this->show_buffer.append(QString(CodeDecoding(buffer)).toUtf8());
    }else
    {
        this->show_buffer.append((buffer).toHex(' ').toUpper()+' ');
    }
    if(*this->timestamp_enflag)
    {
        this->show_buffer.append("\r\n");
    }
}

void VPlainTextEdit::UpdateShow(void)
{
    if(this->hex_enflag==nullptr)return;
    if((this->show_pos_max<=this->show_buffer.size())&&(this->show_pos_min<=this->show_pos_max))
    {

        this->setPlainText(this->show_buffer.mid(this->show_pos_min,
                                                this->show_pos_max));
    }
}

void VPlainTextEdit::ClearBuff(void)
{
    TimerStop();
    this->show_pos_min = 0;
    this->show_pos_max = 0;
    this->show_buffer.clear();
    this->clear();
    TimerStart();
}
//根据hex标志刷新窗口
void VPlainTextEdit::HexEnChanged(void)
{
    //非文本输入切换操作
    if(this->hex_enflag==nullptr)return;
    if(this->show_buffer==nullptr)return;
    if(this->show_pos_min<=this->show_pos_max)
    {
        this->show_pos_min = 0;
        this->show_pos_max = 0;
        this->show_buffer.clear();

        if(!(*this->hex_enflag))
        {
            this->show_buffer.append(this->data);

        }
        else
        {
            this->show_buffer.append(this->data.toHex(' ').toUpper());
        }

        TimerStart();
    }
    this->TimeStampEnChanged();
}

void VPlainTextEdit::TimeStampEnChanged(void)
{
    if(*this->timestamp_enflag)
    {
        this->show_buffer.append("\r\n");
    }
}
