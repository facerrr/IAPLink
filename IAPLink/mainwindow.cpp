#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <qstandardpaths.h>
#include "config.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle("IAPLink");
    // setWindowFlags(Qt::WindowCloseButtonHint);
    setFixedSize(QSize(715, 570));
    StatusBarInit();
    Base_Init();
    PlainTextInit();
    UartItemInit();
    UpdateSerial();
    RxSlotSignalInit();
    UartSlotSignalInit();
    PlainTextTimerCfg();
    BinSlotSignalInit();
    IAPInit();
    UpdateShow();
    ButtonInit();
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::StatusBarInit(void)
{
    this->labelCom = new QLabel("串口未连接",this);
    this->labelCom->setMaximumWidth(250);
    this->labelCom->setMinimumWidth(250);

    this->labelIAP=new QLabel("IAP Status",this);
    this->labelIAP->setMinimumWidth(400);
    this->labelIAP->setMaximumWidth(400);

    ui->statusBar->addWidget(this->labelCom);
    ui->statusBar->addWidget(this->labelIAP);

}

void MainWindow::Base_Init(void)
{
    static const GUID GUID_DEVINTERFACE_LIST[] =
        {
         {0xA5DCBF10, 0x6530, 0x11D2, { 0x90, 0x1F, 0x00, 0xC0, 0x4F, 0xB9, 0x51, 0xED } },
         };

    HDEVNOTIFY hDevNotify;
    DEV_BROADCAST_DEVICEINTERFACE NotifacationFiler;
    ZeroMemory(&NotifacationFiler,sizeof(DEV_BROADCAST_DEVICEINTERFACE));
    NotifacationFiler.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
    NotifacationFiler.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;

    for(unsigned int i=0;i<sizeof(GUID_DEVINTERFACE_LIST)/sizeof(GUID);i++)
    {
        NotifacationFiler.dbcc_classguid = GUID_DEVINTERFACE_LIST[i];
        //GetCurrentUSBGUID();
        hDevNotify = RegisterDeviceNotification(HANDLE(this->winId()),&NotifacationFiler,DEVICE_NOTIFY_WINDOW_HANDLE);
        if(!hDevNotify)
        {
            GetLastError();
        }
    }
}

void MainWindow::ButtonInit(void){
    ui->pushButton_clear->setStyleSheet("QPushButton:hover {background-color: #CECECE;}");
    ui->pushButton_of->setStyleSheet("QPushButton:hover {background-color: #CECECE;}");
    ui->pushButton_appVer->setStyleSheet("QPushButton:hover {background-color: #CECECE;}");
}

void MainWindow::PlainTextInit(void)
{
    connect(&this->xUart.m_recvModule, &RecvModule::recvSignal, ui->plainTextRx, &vPlainTextEdit::receivedData);
    ui->plainTextRx->setHexEnableAddr(&this->xUart.m_serial.rxHexEnable);
    ui->plainTextRx->setTimeStampEnableAddr(&this->xUart.m_serial.rxTimeStampEnable);
}


void MainWindow::UartItemInit(void)
{
    // ui->comboBox_bt->addItem("921600",int(921600));
    // ui->comboBox_bt->addItem("460800",int(460800));
    // ui->comboBox_bt->addItem("256000",int(256000));
    // ui->comboBox_bt->addItem("230400",int(230400));
    // ui->comboBox_bt->addItem("128000",int(128000));
    ui->comboBox_bt->addItem("115200",int(115200));
    // ui->comboBox_bt->addItem("57600",int(57600));
    // ui->comboBox_bt->addItem("43000",int(43000));
    // ui->comboBox_bt->addItem("38400",int(38400));
    // ui->comboBox_bt->addItem("19200",int(19200));
    ui->comboBox_bt->addItem("9600",int(9600));
    ui->comboBox_bt->addItem("4800",int(4800));
    ui->comboBox_bt->setCurrentIndex(0);

    ui->comboBox_stop->addItem("1",QSerialPort::StopBits(QSerialPort::OneStop));
    ui->comboBox_stop->addItem("1.5",QSerialPort::StopBits(QSerialPort::OneAndHalfStop));
    ui->comboBox_stop->addItem("2",QSerialPort::StopBits(QSerialPort::TwoStop));

    ui->comboBox_bit->addItem("8",QSerialPort::DataBits(QSerialPort::Data8));
    ui->comboBox_bit->addItem("7",QSerialPort::DataBits(QSerialPort::Data7));
    ui->comboBox_bit->addItem("6",QSerialPort::DataBits(QSerialPort::Data6));
    ui->comboBox_bit->addItem("5",QSerialPort::DataBits(QSerialPort::Data5));

    ui->comboBox_cs->addItem(QString::fromLocal8Bit("None"),QSerialPort::Parity(QSerialPort::NoParity));
    ui->comboBox_cs->addItem(QString::fromLocal8Bit("EvenParity"),QSerialPort::Parity(QSerialPort::EvenParity));
    ui->comboBox_cs->addItem(QString::fromLocal8Bit("OddParity"),QSerialPort::Parity(QSerialPort::OddParity));

    ui->comboBox_iapV->addItem("ZSeries",int(0));
    ui->comboBox_iapV->addItem("XSeries",int(1));
    ui->comboBox_iapV->addItem("IAPLINK",int(2));

    ui->comboBox_iapV->setCurrentIndex(0);
    this->xUart.m_serial.SetComBoxAddr(ui->comboBox_iapV, ui->comboBox_hostType, ui->comboBox_bt);

    ui->comboBox_hostType->addItem("手持", quint32(0));
    ui->comboBox_hostType->addItem("附件", quint32(1));
    ui->comboBox_hostType->addItem("底座", quint32(2));
    ui->comboBox_hostType->addItem("电池包", quint32(3));

    ui->comboBox_hostType->setCurrentIndex(0);

}

void MainWindow::UpdateSerial(void)
{
    bool haveIt = false;
    static qint16 index;
    this->xUart.m_serial.SerialComScanf();
    ui->comboBox_com->clear();
    haveIt = false;
    index  = 0;
    if(this->xUart.m_serial.mSerialPortCnt!=0)
    {
        for(quint32 i=0; i < this->xUart.m_serial.mSerialPortCnt;i++)
        {
            QString str ="("
                          +this->xUart.m_serial.mSerialComInfo[i].SerialName
                          +")"
                          +this->xUart.m_serial.mSerialComInfo[i].DescribeStr;
            ui->comboBox_com->addItem(str,
                                      this->xUart.m_serial.mSerialComInfo[i].SerialName);

            if(this->xUart.m_serial.mSerialComInfo[i].SerialName ==
                this->xUart.m_serial.mSerialConfig->mSerialPortName)
            {
                haveIt = true;
                index = i;
            }
        }
        //检测上次打开的串口是否存在
        if(haveIt != false)
        {
            ui->comboBox_com->setCurrentIndex(index);
        }
        else
        {
            this->SerialClose();
            this->xUart.m_serial.mSerialConfig->mSerialPortName =
                ui->comboBox_com->currentData().toString();
        }
    }
    else
    {
        this->SerialClose();
        ui->comboBox_com->clear();
        this->xUart.m_serial.mSerialConfig->mSerialPortName =" ";
    }
}


void MainWindow::RxSlotSignalInit(void)
{
    connect(ui->pushButton_clear,&QPushButton::released,ui->plainTextRx,&vPlainTextEdit::clearBuff);
    connect(ui->checkBox_hex,&QCheckBox::released,this,&MainWindow::RxHexEnableCfg);
    connect(ui->checkBox_time,&QCheckBox::released,this,&MainWindow::RxTimeStampCfg);
    connect(ui->pushButton_appVer,&QCheckBox::released,this,&MainWindow::ShowUpdateHistory);
}

void MainWindow::UartSlotSignalInit(void)
{
    connect(ui->radioButton_cn, &QRadioButton::released, this, &MainWindow::SerialOpen);
    connect(this,&MainWindow::openSerial,&this->xUart.m_serial,&MSerialPort::SerialOpen,Qt::BlockingQueuedConnection);
    connect(this,&MainWindow::closeSerial,&this->xUart.m_serial,&MSerialPort::SerialClose,Qt::QueuedConnection);

    this->xUart.m_serial.mSerialConfig->mSerialBaudRate = ui->comboBox_bt->currentData().toInt();
    this->xUart.m_serial.mSerialConfig->mSerialStopBits = QSerialPort::StopBits(ui->comboBox_stop->currentData().toInt());
    this->xUart.m_serial.mSerialConfig->mSerialDataBits = QSerialPort::DataBits(ui->comboBox_bit->currentData().toInt());
    this->xUart.m_serial.mSerialConfig->mSerialParrity  = QSerialPort::Parity(ui->comboBox_cs->currentData().toInt());

    connect(ui->comboBox_com, &QComboBox::currentTextChanged, this, [=]()
            {
                this->xUart.m_serial.mSerialConfig->mSerialPortName = ui->comboBox_com->currentData().toString();
                qDebug()<<"this com change";
                this->SerialClose();
            }
            );
    connect(ui->comboBox_bt,&QComboBox::currentTextChanged,this, [=]()
            {
                this->xUart.m_serial.mSerialConfig->mSerialBaudRate = ui->comboBox_bt->currentData().toInt();
                qDebug()<<"this com change";
                if(this->xUart.m_serial.qSerial->isOpen()){
                    this->xUart.m_serial.qSerial->setBaudRate(this->xUart.m_serial.mSerialConfig->mSerialBaudRate);
                }
                // this->SerialClose();
            });
    connect(ui->comboBox_stop,&QComboBox::currentTextChanged,this, [=]()
            {
                this->xUart.m_serial.mSerialConfig->mSerialStopBits = QSerialPort::StopBits(ui->comboBox_stop->currentData().toInt());
                this->SerialClose();
            });
    connect(ui->comboBox_bit,&QComboBox::currentTextChanged,this, [=]()
            {
                this->xUart.m_serial.mSerialConfig->mSerialDataBits = QSerialPort::DataBits(ui->comboBox_bit->currentData().toInt());
                this->SerialClose();
            });
    connect(ui->comboBox_cs,&QComboBox::currentTextChanged,this, [=]()
            {
                this->xUart.m_serial.mSerialConfig->mSerialParrity  = QSerialPort::Parity(ui->comboBox_cs->currentData().toInt());
                this->SerialClose();
            });

    connect(this, &MainWindow::rxHexEnableChanged, ui->plainTextRx, &vPlainTextEdit::hexEnableChanged);
    connect(this, &MainWindow::timeStampEnableChanged, ui->plainTextRx, &vPlainTextEdit::timeStampEnableChanged);
    connect(this, &MainWindow::commonSend, &this->xUart.m_serial, &MSerialPort::WriteData);
    connect(ui->pushButton_tx1,&QPushButton::released,this, [=]()
            {
                QString data = ui->lineEdit_tx1->text();
                QByteArray byteArray = QByteArray::fromHex(data.remove("0x").toUtf8());
                emit this->commonSend(byteArray);
            });

    connect(ui->pushButton_tx2,&QPushButton::released,this, [=]()
            {
                QString data = ui->lineEdit_tx2->text();
                QByteArray byteArray = QByteArray::fromHex(data.remove("0x").toUtf8());
                emit this->commonSend(byteArray);
            });

    connect(ui->pushButton_haiba,&QPushButton::released,this, [=]()
            {
                QByteArray byteArray = QByteArray::fromHex("AC 02 00 00 DD DD E3 00 00 00 00 4B");
                emit this->commonSend(byteArray);
            });

    connect(ui->pushButton_crc16,&QPushButton::released,this, [=]()
            {
                if(this->xUart.m_serial.hostType == HOST_BRUSH){
                    QByteArray byteArray = QByteArray::fromHex("AC 00 05 DD E2 70");
                    emit this->commonSend(byteArray);
                }else if(this->xUart.m_serial.hostType == HOST_HANDHELD){
                    QByteArray byteArray = QByteArray::fromHex("AC 02 00 00 DD DD E2 00 00 00 00 4A");
                    emit this->commonSend(byteArray);
                }
            });

    connect(ui->pushButton_filter,&QPushButton::released,this, [=]()
            {
                QByteArray byteArray = QByteArray::fromHex("AC 02 00 00 DD DD E1 00 00 00 00 49");
                emit this->commonSend(byteArray);
            });

    connect(ui->pushButton_ver,&QPushButton::released,this, [=]()
            {
                QByteArray byteArray = QByteArray::fromHex("AC 02 00 00 DD DD E0 00 00 00 00 48");
                emit this->commonSend(byteArray);
            });

    connect(&this->xUart.m_recvModule, &RecvModule::updateCrc, ui->lineEdit_crc16, &QLineEdit::setText);
    connect(&this->xUart.m_recvModule, &RecvModule::updateVer, ui->lineEdit_ver, &QLineEdit::setText);
    connect(&this->xUart.m_recvModule, &RecvModule::updateFilter, ui->lineEdit_filter, &QLineEdit::setText);
    connect(&this->xUart.m_recvModule, &RecvModule::updateHaiBa, ui->lineEdit_haiba, &QLineEdit::setText);

    connect(ui->comboBox_hostType, &QComboBox::currentTextChanged, this, [=]()
            {
        this->xUart.m_serial.hostType = ui->comboBox_hostType->currentData().toInt();
            });

    connect(ui->comboBox_iapV,&QComboBox::currentTextChanged,this, [=]()
            {
                this->xUart.m_serial.iapVersion  = ui->comboBox_iapV->currentData().toInt();
                ui->lineEdit_crc16->clear();
                ui->lineEdit_filter->clear();
                ui->lineEdit_haiba->clear();
                ui->lineEdit_ver->clear();
            });

    connect(ui->radioButton_re, &QRadioButton::released, this, [=]()
            {
                if(ui->radioButton_re->isChecked()){
                    this->xUart.m_iapModule.m_reIapFlag = 1;
                }else{
                    this->xUart.m_iapModule.m_reIapFlag = 0;
                }
            });
}


void MainWindow::BinSlotSignalInit(void)
{
    connect(ui->pushButton_of, &QPushButton::released, this, &MainWindow::OpenBinFile);
}

void MainWindow::PlainTextTimerCfg(void)
{
    ui->plainTextRx->TimerStop();

    ui->plainTextRx->setTimerCfg(100);

    ui->plainTextRx->TimerStart();
}

void MainWindow::RxTimeStampCfg(void)
{
    if(ui->checkBox_time->isChecked())
    {
        this->xUart.m_serial.rxTimeStampEnable = true;
    }
    else
    {
        this->xUart.m_serial.rxTimeStampEnable = false;
    }
    emit timeStampEnableChanged();
}

void MainWindow::RxHexEnableCfg(void)
{
    if(ui->checkBox_hex->isChecked())
    {
        this->xUart.m_serial.rxHexEnable = true;
    }
    else
    {
        this->xUart.m_serial.rxHexEnable = false;
    }

    emit rxHexEnableChanged();
}


void MainWindow::ReadSerialSelect(void)
{
    this->xUart.m_serial.mSerialConfig->mSerialPortName = ui->comboBox_com->currentData().toString();
    this->xUart.m_serial.mSerialConfig->mSerialBaudRate = ui->comboBox_bt->currentData().toInt();
    this->xUart.m_serial.mSerialConfig->mSerialStopBits = QSerialPort::StopBits(ui->comboBox_stop->currentData().toInt());
    this->xUart.m_serial.mSerialConfig->mSerialDataBits = QSerialPort::DataBits(ui->comboBox_bit->currentData().toInt());
    this->xUart.m_serial.mSerialConfig->mSerialParrity  = QSerialPort::Parity(ui->comboBox_cs->currentData().toInt());

    this->xUart.m_serial.iapVersion = ui->comboBox_iapV->currentData().toInt();
}

void MainWindow::UpdateShow(void)
{
    this->xUart.m_recvModule.DisConnectRx();
    this->xUart.m_recvModule.ConnectRx();
    ReadSerialSelect();
    RxTimeStampCfg();
    RxHexEnableCfg();
}


void MainWindow::SerialOpen(void)
{
    if(ui->radioButton_cn->isChecked())
    {
        //读取选择的串口配置
        ReadSerialSelect();
        bool isOpen;
        emit openSerial(isOpen);
        if(isOpen)
        {
            ui->radioButton_cn->setChecked(true);
            this->labelCom->setText(this->xUart.m_serial.mSerialConfig->mSerialPortName+"串口打开");
            this->xUart.m_serial.qSerial->clear();
            ui->plainTextRx->clearBuff();
            QByteArray byteArray = QByteArray::fromHex("AC 02 00 00 DD DD E4 00 00 00 00 4C");
            emit this->commonSend(byteArray);
        }
        else
        {
            this->doWarning(QString::fromLocal8Bit("Opening the serial port failed, please check whether the serial port is inserted!"));
            this->SerialClose();
        }
    }
    else
    {
        this->SerialClose();
    }
}

void MainWindow::SerialClose(void)
{
    ui->radioButton_cn->setChecked(false);
    QString portName = this->xUart.m_serial.mSerialConfig->mSerialPortName;
    Sleep(50);
    emit closeSerial();
    this->labelCom->setText(portName + "串口断开");
}


void MainWindow::OpenBinFile(void)
{
    QString desktop_path = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    QString filename = QFileDialog::getOpenFileName(this,"Open File",desktop_path,"Binary Files (*.bin)");

    ui->lineEdit_bin->setText(filename);
    if(filename.isEmpty())
    {
        return;
    }
    QFileInfo fileInfo(filename);
    QString shortFileName = fileInfo.completeBaseName();
    this->xUart.m_iapModule.m_firmwareFileName = shortFileName;
    
    QFile file(filename);

    if(!file.open(QIODevice::ReadOnly))
    {
        QMessageBox::information(this,"Error Message", "Error: Could not open file");
    }
    QByteArray fileData = file.readAll();
    file.close();
    qint32 fileLength = fileData.length();

    quint32 checksum = 0;
    quint32 binStartAddr = 0;
    for(int i=4; i<8;i++) {
        binStartAddr += ((quint8)fileData[i] << (i - 4) * 8);
    }
    QString str = "0x"+ QString::number(binStartAddr, 16).toUpper();
    ui->lineEdit_fsac->setText(str);
    qDebug()<<str;
    if(this->xUart.m_serial.iapVersion != IAPLINK){
        if(binStartAddr > 0x8010000){
            this->xUart.m_serial.SetIAPVersion(ZSERIES);
        }else if(binStartAddr == 0x28D5 || binStartAddr == 0x2981){
            this->xUart.m_serial.SetHostType(HOST_BOTTOM);
            this->xUart.m_serial.SetIAPVersion(XSERIES);
        }else if(binStartAddr == 0x519D || binStartAddr == 0x5165 || binStartAddr == 0x165 || binStartAddr == 0x2965 || binStartAddr == 0x51ED ){
            if(binStartAddr == 0x2965){
                this->mcu_type = MCU_021;
                this->xUart.m_serial.SetHostType(HOST_BRUSH);
            }else{
                this->mcu_type = MCU_052;
            }
            this->xUart.m_serial.SetIAPVersion(XSERIES);
        }else if(binStartAddr == 0x29B9 || binStartAddr == 0x299D){
            this->xUart.m_serial.SetHostType(HOST_BATTERY);
            this->xUart.m_serial.SetIAPVersion(XSERIES);
            this->xUart.m_serial.combox_baudrate->setCurrentIndex(1);
        }else{
            QMessageBox::information(this, "IAP", "请选择有效的Bin文件");
            return;
        }
    }else{
        if(binStartAddr < 0x8010000 && binStartAddr != 0x519D && binStartAddr != 0x5165 && binStartAddr != 0x165){
            QMessageBox::information(this, "IAP", "请选择有效的Bin文件");
            return;
        }
    }

    this->xUart.m_iapModule.IAPFile_Init(fileData, fileLength);
    if(this->xUart.m_serial.iapVersion == ZSERIES){
        checksum = this->xUart.m_iapModule.sum8Check;
    }else{
        checksum = this->xUart.m_iapModule.crc16;
    }
    str = "0x"+ QString::number(checksum, 16).toUpper();
    ui->lineEdit_bcs->setText(str);
    this->labelIAP->setText("IAP Ready");
    ui->pushButton_iap->setEnabled(true);
}


void MainWindow::IAPInit(void)
{
    this->xUart.m_serial.iapVersion = ui->comboBox_iapV->currentData().toInt();

    connect(ui->pushButton_iap, &QPushButton::released, this, &MainWindow::IAPUpgrade);
    connect(this, &MainWindow::iapStart, &this->xUart.m_iapModule, &IAPModule::IAP_Func);
    connect(&this->xUart.m_iapModule, &IAPModule::upgradeBar, this, [=](int a){ui->iapBar->setValue(a);});
    connect(&this->xUart.m_iapModule, &IAPModule::transFinish, this, &MainWindow::TransThreadFinish);

    ui->pushButton_iap->setEnabled(false);
    ui->iapBar->setStyleSheet("QProgressBar{text-align: center;"
                              "background-color: #e2e3e4;"
                              "border: 0px solid #e2e3e4;"
                              "border-radius: 5px;}"
                              "QProgressBar::chunk{background-color: #00FF00; "
                              "border-radius: 5px;}");
}

void MainWindow::IAPUpgrade(void)
{
    QPalette palette;
    if(!this->xUart.m_iapModule.m_transStr->m_iapIngFlag){
        if(this->xUart.m_serial.qSerial->isOpen()){
            palette.setColor(QPalette::Button, QColor(Qt::darkCyan));
            ui->pushButton_iap->setPalette(palette);
            ui->pushButton_iap->setText("停止");
            this->xUart.m_iapModule.m_transStr->m_transNumber = 1;
            this->xUart.m_iapModule.m_transStr->m_transStatus = TransIdle;
            this->xUart.m_iapModule.m_transStr->m_iapIngFlag = 1;
            if(this->xUart.m_serial.iapVersion == XSERIES){
                if(this->xUart.m_serial.hostType == HOST_BOTTOM){
                    this->xUart.m_serial.SetAppFlashAddr(0x2800);
                }else if(this->xUart.m_serial.hostType == HOST_BATTERY){
                    this->xUart.m_serial.SetAppFlashAddr(0x2800);
                }else{
                    if(this->mcu_type == MCU_021){
                        this->xUart.m_serial.SetAppFlashAddr(0x2800);
                    }else{
                        this->xUart.m_serial.SetAppFlashAddr(0x5000);
                    }
                }
            }
            this->labelIAP->setText(QString("IAP 升级中, FlashAddr = %1").arg(this->xUart.m_serial.m_appFlashAddr));
            ui->iapBar->setValue(0);
            emit this->iapStart(this->xUart.m_serial.iapVersion);
        }else{
            QMessageBox::information(this, "IAP", "Please Open a valid com!");
        }
    }else{
        palette.setColor(QPalette::Button, QColor(Qt::white));
        ui->pushButton_iap->setPalette(palette);
        ui->pushButton_iap->setText("下载");
        this->xUart.m_iapModule.m_transStr->m_iapIngFlag = 0;
        this->xUart.m_iapModule.m_transStr->m_transStatus = TransAbort;
        this->xUart.m_iapModule.m_transStr->m_appIsRuning = 0;
        this->labelIAP->setText("IAP 下载中断");
    }
}


void MainWindow::TransThreadFinish(quint8 threadSta, quint8 threadCmd){
    if(this->xUart.m_serial.iapVersion == 0){
        if(threadSta == TransFinished){
            QMessageBox::information(this,"IAP Result", "Success: IAP Sucess");
            this->labelIAP->setText("IAP Sucess");
        }else{
            QMessageBox::warning(this, "IAP Result", "IAP Failed!");
            this->labelIAP->setText("IAP Fail");
        }
    }else{
        QString title = NULL, command = NULL;
        title = "IAP";
        QString content;
        switch (threadCmd){
        case PACKET_CMD_HANDSHAKE:
            command = "握手";
            break;
        case PACKET_CMD_JUMP_TO_APP:
            command = "跳转";
            break;
        case PACKET_CMD_APP_DOWNLOAD:
            command = "下载";
            break;
        case PACKET_CMD_APP_UPLOAD:
            command = "上传";
            break;
        case PACKET_CMD_ERASE_FLASH:
            command = "擦除Flash";
            break;
        case PACKET_CMD_FLASH_CRC:
            command = "Flash校验";
            break;
        case PACKET_CMD_APP_UPGRADE:
            command = "APP升级";
            break;
        default:
            break;
        }

        switch (threadSta){
        case TransFinished:
            content = title + "升级/上传完成！";
            break;
        case TransTimeout:
            content = (title + "程序" + command + "超时,"+ "请检查设备及连接线是否正常!");
            break;
        case TransFileInvalid:
            content = (title + "程序" + "请选择有效的文件!");
            break;
        case TransAddrError:
            content = (title + "程序" + command + "地址错误，" + "请输入有效地址值!");
            break;
        case TransFailed:
            content = (title + "程序" + command + "失败，" + "请检查参数是否合法!");
            break;
        case TransAbort:
            content = (title + "程序" + "终止!");
            break;
        default:
            break;
        }
        QByteArray byteArray = QByteArray::fromHex("AC 02 00 00 DD DD E4 00 00 00 00 4C");
        emit this->commonSend(byteArray);
        memset(&this->xUart.m_recvModule.m_transStr, 0, 7);
        QMessageBox::information(this, "IAP", content);

    }
    if(this->xUart.m_serial.hostType == HOST_BATTERY){
        this->xUart.m_serial.combox_baudrate->setCurrentIndex(1);
    }
    this->xUart.m_recvModule.ResetIAPModule();
    this->xUart.m_recvModule.m_transStr.m_recvType = RECV_NORMAL;
    this->labelIAP->setText("IAP Ready");
    QPalette palette;
    palette.setColor(QPalette::Button, QColor(Qt::white));
    ui->pushButton_iap->setPalette(palette);
    ui->pushButton_iap->setText("下载");

}

void MainWindow::ShowUpdateHistory(void){
    QString updateHistory = "版本更新履历：\n"
                            "1.0 - 初始版本\n"
                            "1.1 - 串口性能优化\n"
                            "1.2 - 改善串口热插拔问题\n"
                            "1.3 - 增加Bin文件检查\n"
                            "1.4 - 添加死机后二次烧录功能\n"
                            "1.5 - 修复二次烧录Bug\n"
                            "2.0 - 合并X系列和Z系列烧录功能\n"
                            "     - 支持升级失败后的二次烧录\n"
                            "     - 可根据选择的Bin文件自动选择机型\n"
                            "     - 增添Bin文件校验\n"
                            "     - 修复接收窗口显示Bug";

    QMessageBox::information(this, "版本更新履历", updateHistory);
}

void MainWindow::doWarning(const QString &str)
{
    QMessageBox::warning(this,QString::fromLocal8Bit("Warning"),str);

}


bool MainWindow::nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result){
    Q_UNUSED(eventType);
    Q_UNUSED(result);
    MSG* msg = reinterpret_cast<MSG*>(message);
    UINT msgType = msg->message;
    if(msgType==WM_DEVICECHANGE)
    {

        PDEV_BROADCAST_HDR lpdb = PDEV_BROADCAST_HDR(msg->lParam);
        switch (msg->wParam) {
        case DBT_DEVICEARRIVAL:
            if(lpdb->dbch_devicetype == DBT_DEVTYP_PORT)
            {
                PDEV_BROADCAST_DEVICEINTERFACE pDevInf  = PDEV_BROADCAST_DEVICEINTERFACE(lpdb);
                Q_UNUSED(pDevInf)
                UpdateSerial();
            }
            break;
        case DBT_DEVICEREMOVECOMPLETE:
            if(lpdb->dbch_devicetype == DBT_DEVTYP_PORT)
            {
                if(ui->radioButton_cn->isChecked())
                {
                    ui->radioButton_cn->setChecked(false);
                    this->SerialClose();
                }
                PDEV_BROADCAST_DEVICEINTERFACE pDevInf  = PDEV_BROADCAST_DEVICEINTERFACE(lpdb);
                Q_UNUSED(pDevInf)
                UpdateSerial();
            }
            break;
        }
    }
    return false;
}




