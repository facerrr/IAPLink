#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <qstandardpaths.h>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "modelcfgdialog.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle("IAPLink");
    // setWindowFlags(Qt::WindowCloseButtonHint);
    setFixedSize(QSize(715, 570));

    InitWindowElement();
    InitUsbDevice();
    InitRxPlainText();
    InitUartItem();
    InitModelInfo(0xff);
    UpdateSerial();
    InitUartSlotSignal();
    InitPlainTextTimer();
    InitSerialSelect();

    InitIAP();

}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::InitWindowElement(void){

    // Status Bar Init
    this->label_com_status = new QLabel("串口未连接",this);
    this->label_com_status->setMaximumWidth(250);
    this->label_com_status->setMinimumWidth(250);

    this->label_iap_status = new QLabel("IAP Status",this);
    this->label_iap_status->setMinimumWidth(400);
    this->label_iap_status->setMaximumWidth(400);

    ui->statusBar->addWidget(this->label_com_status);
    ui->statusBar->addWidget(this->label_iap_status);

    // Bottom Style

    ui->pushButton_clear->setStyleSheet("QPushButton:hover {background-color: #CECECE;}");
    ui->pushButton_of->setStyleSheet("QPushButton:hover {background-color: #CECECE;}");
    ui->pushButton_modelCfg->setStyleSheet("QPushButton:hover {background-color: #CECECE;}");

    ui->iapBar->setStyleSheet("QProgressBar{text-align: center;"
                              "background-color: #e2e3e4;"
                              "border: 0px solid #e2e3e4;"
                              "border-radius: 5px;}"
                              "QProgressBar::chunk{background-color: #00FF00; "
                              "border-radius: 5px;}");


    connect(ui->pushButton_of, &QPushButton::released, this, &MainWindow::OpenBinFile);
    connect(ui->pushButton_modelCfg, &QPushButton::released, this, &MainWindow::OpenModelCfgDialog);
    connect(ui->pushButton_appVer, &QCheckBox::released, this, &MainWindow::ShowUpdateHistory);
}



void MainWindow::InitUsbDevice(void){
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


void MainWindow::InitRxPlainText(void){
    connect(&this->xUart.recv_module, &RecvModule::RecvDataSignal, ui->plaintText_recv, &VPlainTextEdit::ReceivedData);
    ui->plaintText_recv->SetHexCfgAddr(&this->xUart.m_serial.rx_hex_en);
    ui->plaintText_recv->SetTimeStampAddr(&this->xUart.m_serial.rx_timestamp_en);
}


void MainWindow::InitUartItem(void){
    ui->comboBox_bt->addItem("115200",int(115200));
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

    ui->comboBox_hostType->addItem("手持", quint32(0));
    ui->comboBox_hostType->addItem("附件", quint32(1));
    ui->comboBox_hostType->addItem("底座", quint32(2));
    ui->comboBox_hostType->addItem("电池包", quint32(3));

    ui->comboBox_hostType->setCurrentIndex(0);

    this->xUart.m_serial.SetComBoxAddr(ui->comboBox_bt);

    //Slot Signal
    connect(ui->pushButton_clear, &QPushButton::released, ui->plaintText_recv, &VPlainTextEdit::ClearBuff);
    connect(ui->checkBox_hex, &QCheckBox::released, this, &MainWindow::RxHexEnableCfg);
    connect(ui->checkBox_time,&QCheckBox::released,this,&MainWindow::RxTimeStampCfg);

    //
}


void MainWindow::InitModelInfo(quint8 index)
{
    QString configFilePath = QCoreApplication::applicationDirPath() + "/model.json";
    QFile file(configFilePath);
    
    if (!file.exists() || !file.open(QIODevice::ReadOnly)) {
        qDebug() << "Model config file not found. Using default model info.";
        return;
    }

    QByteArray data = file.readAll();
    file.close();
    
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) return;
    
    QJsonObject root = doc.object();
    QJsonArray modelsArray = root["models"].toArray();

    int selectedIndex = root["selected_model_index"].toInt(0);

    if(index >= modelsArray.size()){
        if (selectedIndex < 0 || selectedIndex >= modelsArray.size()) {
            selectedIndex = 0;
        }
    }else{
        selectedIndex = index;
    }
    
    if (modelsArray.isEmpty()) return;

    QJsonObject obj = modelsArray[selectedIndex].toObject();
    
    this->model_info.verison = obj["verison"].toInt();
    this->model_info.host_type = obj["host_type"].toInt();
    this->model_info.normal_rx_baudrate = obj["normal_rx_baudrate"].toInt();
    this->model_info.upgrade_baudrate = obj["upgrade_baudrate"].toInt();
    this->model_info.verify_mode = obj["verify_mode"].toInt();
    
    this->model_info.upgrade_cmd = QByteArray::fromBase64(obj["upgrade_cmd"].toString().toUtf8());
    this->model_info.handshake_check_buffer = QByteArray::fromBase64(obj["handshake_check_buffer"].toString().toUtf8());
    this->model_info.normal_rx_check_buffer = QByteArray::fromBase64(obj["normal_rx_check_buffer"].toString().toUtf8());
    this->model_info.normal_rx_check_value = QByteArray::fromBase64(obj["normal_rx_check_value"].toString().toUtf8());
    
    this->model_info.normal_rx_check_length = obj["normal_rx_check_length"].toInt();
    this->model_info.inquiry_enable = obj["inquiry_enable"].toInt();
    
    QJsonArray cmdArr = obj["inquiry_cmd"].toArray();
    QJsonArray contentArr = obj["inquiry_content"].toArray();
    QJsonArray resCheckArr = obj["inquiry_res_check"].toArray();
    for (int i = 0; i < 4; ++i) {
        if (i < cmdArr.size()) {
            this->model_info.inquiry_cmd[i] = QByteArray::fromBase64(cmdArr[i].toString().toUtf8());
        }
        if (i < contentArr.size()) {
            this->model_info.inquiry_content[i] = contentArr[i].toString();
        }
        if (i < resCheckArr.size()) {
            this->model_info.inquiry_res_check[i] = QByteArray::fromBase64(resCheckArr[i].toString().toUtf8());
        }
    }
    
    this->model_info.app_flash_st_addr = obj["app_flash_st_addr"].toInt();

    this->xUart.ShareModelInfo(&this->model_info);
    ui->comboBox_iapV->setCurrentIndex(this->model_info.verison);

    int bt_index = ui->comboBox_bt->findData(static_cast<int>(this->model_info.normal_rx_baudrate));
    if (bt_index != -1) {
        ui->comboBox_bt->setCurrentIndex(bt_index);
    }

    ui->comboBox_hostType->setCurrentIndex(this->model_info.host_type);

    QPushButton* inquiry_btns[4] = {
        ui->pushButton_inquiry1,
        ui->pushButton_inquiry2,
        ui->pushButton_inquiry3,
        ui->pushButton_inquiry4
    };
    if(this->model_info.inquiry_enable == 1){
        for (int i = 0; i < 4; ++i) {
            inquiry_btns[i]->setEnabled(true);
            inquiry_btns[i]->setText(this->model_info.inquiry_content[i]);
            connect(inquiry_btns[i], &QPushButton::released, this, [this, i]() {
                if (!this->model_info.inquiry_cmd[i].isEmpty()) {
                    emit this->DataSendSignal(this->model_info.inquiry_cmd[i]);
                }
            });
        }
        // 查询label更新槽与信号
        connect(&this->xUart.recv_module, &RecvModule::InquiryUpdate, this, [=](quint8 type, const QString& value) {
            switch (type) {
            case 0: ui->lineEdit_inquiry1->setText(value); break;
            case 1: ui->lineEdit_inquiry2->setText(value); break;
            case 2: ui->lineEdit_inquiry3->setText(value); break;
            case 3: ui->lineEdit_inquiry4->setText(value); break;
            }
        });
    }else{
        for (int i = 0; i < 4; ++i) {
            inquiry_btns[i]->setEnabled(false);
            inquiry_btns[i]->setText("inquir disable");
        }
        ui->lineEdit_inquiry1->setText("");
        ui->lineEdit_inquiry2->setText("");
        ui->lineEdit_inquiry3->setText("");
        ui->lineEdit_inquiry4->setText("");
    }
}


void MainWindow::UpdateSerial(void){
    bool haveIt = false;
    static qint16 index;
    this->xUart.m_serial.SerialComScanf();
    ui->comboBox_com->clear();
    haveIt = false;
    index  = 0;

    if(this->xUart.m_serial.serial_port_cnt!=0){
        for(quint32 i=0; i < this->xUart.m_serial.serial_port_cnt;i++)
        {
            QString str ="("
                          +this->xUart.m_serial.serial_info[i].SerialName
                          +")"
                          +this->xUart.m_serial.serial_info[i].DescribeStr;
            ui->comboBox_com->addItem(str,
                                      this->xUart.m_serial.serial_info[i].SerialName);

            if(this->xUart.m_serial.serial_info[i].SerialName ==
                this->xUart.m_serial.serial_config->mSerialPortName)
            {
                haveIt = true;
                index = i;
            }
            //检测上次打开的串口是否存在
            if(haveIt != false)
            {
                ui->comboBox_com->setCurrentIndex(index);
            }
            else
            {
                // this->SerialClose();
                this->xUart.m_serial.serial_config->mSerialPortName =
                    ui->comboBox_com->currentData().toString();
            }
        }
    }
    else
    {
        // this->SerialClose();
        ui->comboBox_com->clear();
        this->xUart.m_serial.serial_config->mSerialPortName =" ";
    }

}


void MainWindow::InitUartSlotSignal(void){
    connect(ui->radioButton_cn, &QRadioButton::released, this, &MainWindow::SerialOpen);
    connect(this, &MainWindow::OpenSerialSignal, &this->xUart.m_serial, &MSerialPort::SerialOpen, Qt::BlockingQueuedConnection);
    connect(this, &MainWindow::CloseSerialSignal, &this->xUart.m_serial, &MSerialPort::SerialClose, Qt::QueuedConnection);

    this->xUart.m_serial.serial_config->mSerialBaudRate = ui->comboBox_bt->currentData().toInt();
    this->xUart.m_serial.serial_config->mSerialStopBits = QSerialPort::StopBits(ui->comboBox_stop->currentData().toInt());
    this->xUart.m_serial.serial_config->mSerialDataBits = QSerialPort::DataBits(ui->comboBox_bit->currentData().toInt());
    this->xUart.m_serial.serial_config->mSerialParrity  = QSerialPort::Parity(ui->comboBox_cs->currentData().toInt());

    connect(ui->comboBox_com, &QComboBox::currentTextChanged, this, [=]()
            {
                this->xUart.m_serial.serial_config->mSerialPortName = ui->comboBox_com->currentData().toString();
                this->SerialClose();
            }
            );

    connect(ui->comboBox_bt,&QComboBox::currentTextChanged,this, [=]()
            {
                this->xUart.m_serial.serial_config->mSerialBaudRate = ui->comboBox_bt->currentData().toInt();
                if(this->xUart.m_serial.qSerial->isOpen()){
                    this->xUart.m_serial.qSerial->setBaudRate(this->xUart.m_serial.serial_config->mSerialBaudRate);
                }
            });

    connect(ui->comboBox_stop,&QComboBox::currentTextChanged,this, [=]()
            {
                this->xUart.m_serial.serial_config->mSerialStopBits = QSerialPort::StopBits(ui->comboBox_stop->currentData().toInt());
                this->SerialClose();
            });

    connect(ui->comboBox_bit,&QComboBox::currentTextChanged,this, [=]()
            {
                this->xUart.m_serial.serial_config->mSerialDataBits = QSerialPort::DataBits(ui->comboBox_bit->currentData().toInt());
                this->SerialClose();
            });
    connect(ui->comboBox_cs,&QComboBox::currentTextChanged,this, [=]()
            {
                this->xUart.m_serial.serial_config->mSerialParrity  = QSerialPort::Parity(ui->comboBox_cs->currentData().toInt());
                this->SerialClose();
            });

    connect(this, &MainWindow::RxHexEnChanged, ui->plaintText_recv, &VPlainTextEdit::HexEnChanged);
    connect(this, &MainWindow::TimeStampChanged, ui->plaintText_recv, &VPlainTextEdit::TimeStampEnChanged);
    connect(this, &MainWindow::DataSendSignal, &this->xUart.m_serial, &MSerialPort::WriteData);

    connect(ui->pushButton_tx1,&QPushButton::released,this, [=]()
            {
                QString data = ui->lineEdit_tx1->text();
                QByteArray byteArray = QByteArray::fromHex(data.remove("0x").toUtf8());
                emit this->DataSendSignal(byteArray);
            });

    connect(ui->pushButton_tx2,&QPushButton::released,this, [=]()
            {
                QString data = ui->lineEdit_tx2->text();
                QByteArray byteArray = QByteArray::fromHex(data.remove("0x").toUtf8());
                emit this->DataSendSignal(byteArray);
            });

    connect(ui->comboBox_iapV,&QComboBox::currentTextChanged,this, [=]()
            {
                this->model_info.verison = ui->comboBox_iapV->currentData().toInt();
                ui->lineEdit_inquiry1->clear();
                ui->lineEdit_inquiry2->clear();
                ui->lineEdit_inquiry3->clear();
                ui->lineEdit_inquiry4->clear();
            });
    connect(ui->comboBox_hostType, &QComboBox::currentTextChanged, this, [=]()
            {
                this->model_info.host_type = ui->comboBox_hostType->currentData().toInt();
            });

    connect(ui->radioButton_re, &QRadioButton::released, this, [=]()
            {
                if(ui->radioButton_re->isChecked()){
                    this->xUart.iap_module.re_iap_flag = 1;
                }else{
                    this->xUart.iap_module.re_iap_flag = 0;
                }
            });

}


void MainWindow::InitPlainTextTimer(void)
{
    ui->plaintText_recv->TimerStop();

    ui->plaintText_recv->SetTimeCfg(100);

    ui->plaintText_recv->TimerStart();
}

void MainWindow::RxTimeStampCfg(void)
{
    if(ui->checkBox_time->isChecked())
    {
        this->xUart.m_serial.rx_timestamp_en = true;
    }
    else
    {
        this->xUart.m_serial.rx_timestamp_en = false;
    }
    emit this->TimeStampChanged();
}

void MainWindow::RxHexEnableCfg(void)
{
    if(ui->checkBox_hex->isChecked())
    {
        this->xUart.m_serial.rx_hex_en = true;
    }
    else
    {
        this->xUart.m_serial.rx_hex_en = false;
    }

    emit this->RxHexEnChanged();
}



void MainWindow::ReadSerialSelect(void)
{
    this->xUart.m_serial.serial_config->mSerialPortName = ui->comboBox_com->currentData().toString();
    this->xUart.m_serial.serial_config->mSerialBaudRate = ui->comboBox_bt->currentData().toInt();
    this->xUart.m_serial.serial_config->mSerialStopBits = QSerialPort::StopBits(ui->comboBox_stop->currentData().toInt());
    this->xUart.m_serial.serial_config->mSerialDataBits = QSerialPort::DataBits(ui->comboBox_bit->currentData().toInt());
    this->xUart.m_serial.serial_config->mSerialParrity  = QSerialPort::Parity(ui->comboBox_cs->currentData().toInt());
}


void MainWindow::InitSerialSelect(void)
{
    this->xUart.recv_module.DisconnectRx();
    this->xUart.recv_module.ConnectRx();
    this->ReadSerialSelect();
    this->RxTimeStampCfg();
    this->RxHexEnableCfg();
}


void MainWindow::SerialOpen(void)
{
    if(ui->radioButton_cn->isChecked())
    {
        this->ReadSerialSelect();
        bool is_open;
        emit OpenSerialSignal(is_open);
        if(is_open)
        {
            ui->radioButton_cn->setChecked(true);
            this->label_com_status->setText(this->xUart.m_serial.serial_config->mSerialPortName+"串口打开");
            ui->plaintText_recv->ClearBuff();
            if(this->model_info.host_type == HOST_HANDHELD){
                if(this->model_info.inquiry_enable == 1){
                    QByteArray inquiry_buf = QByteArray::fromHex("AC 02 00 00 DD DD E4 00 00 00 00 4C");
                    emit this->DataSendSignal(inquiry_buf);

                }
            }
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
    QString portName = this->xUart.m_serial.serial_config->mSerialPortName;
    Sleep(50);
    emit CloseSerialSignal();
    this->label_com_status->setText(portName + "串口断开");
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

    QFileInfo file_info(filename);
    QString short_file_name = file_info.completeBaseName();
    this->xUart.iap_module.firmware_name = short_file_name;

    QFile file(filename);

    if(!file.open(QIODevice::ReadOnly))
    {
        QMessageBox::information(this,"Error Message", "Error: Could not open file");
    }
    QByteArray file_data = file.readAll();
    file.close();
    qint32 file_length = file_data.length();

    quint32 checksum = 0;
    quint32 bin_start_addr = 0;
    for(int i=4; i<8;i++) {
        bin_start_addr += ((quint8)file_data[i] << (i - 4) * 8);
    }
    QString str = "0x"+ QString::number(bin_start_addr, 16).toUpper();
    ui->lineEdit_fsac->setText(str);
    qDebug()<<str;
    if(this->model_info.verison != IAPLINK){
        if(bin_start_addr > 0x8010000){
            ui->comboBox_iapV->setCurrentIndex(ZSERIES);
        }else if(bin_start_addr == 0x28D5 || bin_start_addr == 0x2981){
            InitModelInfo(HOST_BOTTOM);
        }else if(bin_start_addr == 0x51ED || bin_start_addr == 0x51E5 || bin_start_addr == 0x51B9){
            InitModelInfo(HOST_HANDHELD);
        }else if(bin_start_addr == 0x5165 || bin_start_addr == 0x519D){
            InitModelInfo(HOST_BRUSH);
        }else if(bin_start_addr == 0x165 || bin_start_addr == 0x2965){
            InitModelInfo(HOST_BRUSH);
        }else if(bin_start_addr == 0x29B9 || bin_start_addr == 0x299D){
            InitModelInfo(HOST_BATTERY);
        }else{
            QMessageBox::information(this, "IAP", "请选择有效的Bin文件");
            return;
        }
    }else{
        if(bin_start_addr < 0x8010000 && bin_start_addr != 0x519D && bin_start_addr != 0x5165 && bin_start_addr != 0x165){
            QMessageBox::information(this, "IAP", "请选择有效的Bin文件");
            return;
        }
    }

    this->xUart.iap_module.InitIAPFile(file_data, file_length);
    if(this->model_info.verison == ZSERIES){
        checksum = this->xUart.iap_module.add8_check;
    }else{
        checksum = this->xUart.iap_module.crc16_check;
    }

    str = "0x"+ QString::number(checksum, 16).toUpper();
    ui->lineEdit_bcs->setText(str);
    this->label_iap_status->setText("IAP Ready");
    ui->pushButton_iap->setEnabled(true);
}

void MainWindow::OpenModelCfgDialog(void)
{
    ModelCfgDialog dialog(this);

    if (dialog.exec() == QDialog::Accepted) {
        // Read the newly saved configuration
        InitModelInfo(0xff);
    }
}


void MainWindow::InitIAP(void)
{
    connect(ui->pushButton_iap, &QPushButton::released, this, &MainWindow::IAPUpgrade);
    connect(this, &MainWindow::IAPUpdateStart, &this->xUart.iap_module, &IAPModule::IAPUpgradeFunc);
    connect(&this->xUart.iap_module, &IAPModule::UpgradeBarSignal, this, [=](int a){ui->iapBar->setValue(a);});
    connect(&this->xUart.iap_module, &IAPModule::TransFinishSignal, this, &MainWindow::TransThreadFinish);

    ui->pushButton_iap->setEnabled(false);
}


void MainWindow::IAPUpgrade(void)
{
    QPalette palette;
    if(!this->xUart.iap_module.iap_trans->is_upgrading){
        if(this->xUart.m_serial.qSerial->isOpen()){
            palette.setColor(QPalette::Button, QColor(Qt::darkCyan));
            ui->pushButton_iap->setPalette(palette);
            ui->pushButton_iap->setText("停止");
            this->xUart.iap_module.iap_trans->trans_numble = 1;
            this->xUart.iap_module.iap_trans->trans_status = TransIdle;
            this->xUart.iap_module.iap_trans->is_upgrading = 1;
            this->label_iap_status->setText(QString("IAP 升级中, FlashAddr = %1").arg(this->model_info.app_flash_st_addr));
            ui->iapBar->setValue(0);
            emit this->IAPUpdateStart(this->model_info.verison);
        }else{
            QMessageBox::information(this, "IAP", "Please Open a valid com!");
        }
    }else{
        palette.setColor(QPalette::Button, QColor(Qt::white));
        ui->pushButton_iap->setPalette(palette);
        ui->pushButton_iap->setText("下载");
        this->xUart.iap_module.iap_trans->is_upgrading = 0;
        this->xUart.iap_module.iap_trans->trans_status = TransAbort;
        this->xUart.iap_module.iap_trans->is_app_program = 0;
        this->label_iap_status->setText("IAP 下载中断");
    }

}

void MainWindow::TransThreadFinish(quint8 thread_status, quint8 thread_cmd)
{
    if(this->model_info.verison == ZSERIES){
        if(thread_status == TransFinished){
            QMessageBox::information(this,"IAP Result", "Success: IAP Sucess");
            this->label_iap_status->setText("IAP Sucess");
        }else{
            QMessageBox::warning(this, "IAP Result", "IAP Failed!");
            this->label_iap_status->setText("IAP Fail");
        }
    }else{
        QString title = NULL, command = NULL;
        title = "IAP";
        QString content;
        switch (thread_cmd){
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

        switch (thread_status){
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
        if(this->model_info.verison == HOST_HANDHELD){
            if(this->model_info.inquiry_enable == 1){
                QByteArray byteArray = QByteArray::fromHex("AC 02 00 00 DD DD E4 00 00 00 00 4C");
                emit this->DataSendSignal(byteArray);
            }
        }
        this->xUart.recv_module.ResetIAPModule();
        QMessageBox::information(this, "IAP", content);
    }
    if(this->model_info.upgrade_baudrate != this->model_info.normal_rx_baudrate){
        // int index = ui->comboBox_bt->findText(QString::number(this->model_info.normal_rx_baudrate));
        int index = ui->comboBox_bt->findData(static_cast<int>(this->model_info.normal_rx_baudrate));
        if (index != -1) {
            ui->comboBox_bt->setCurrentIndex(index);
        }
    }
    this->xUart.recv_module.ResetIAPModule();
    this->xUart.recv_module.iap_trans.recv_type = RECV_NORMAL;
    this->label_iap_status->setText("IAP Ready");
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
                            "     - 修复接收窗口显示Bug\n"
                            "3.0 - 增加机型自定义功能";

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
                this->UpdateSerial();
            }
            break;
        case DBT_DEVICEREMOVECOMPLETE:
            if(lpdb->dbch_devicetype == DBT_DEVTYP_PORT)
            {
                if(ui->radioButton_cn->isChecked())
                {
                    ui->radioButton_cn->setChecked(false);
                }
                PDEV_BROADCAST_DEVICEINTERFACE pDevInf  = PDEV_BROADCAST_DEVICEINTERFACE(lpdb);
                Q_UNUSED(pDevInf)
                this->UpdateSerial();
            }
            break;
        }
    }
    return false;
}




