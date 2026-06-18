#include "modelcfgdialog.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QJsonDocument>
#include <QInputDialog>
#include <QMessageBox>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>

namespace {
constexpr const char* kBuiltInModelKey = "is_builtin_model";
}


ModelCfgDialog::ModelCfgDialog(QWidget *parent)
    : QDialog(parent)
{
    m_configFilePath = QCoreApplication::applicationDirPath() + "/model.json";
    
    setupUi();
    loadConfig();
}

void ModelCfgDialog::setupUi()
{
    setWindowTitle(tr("机型配置 / Model Configuration"));
    
    // 设置与主 UI 完全一致的尺寸，并移动到相同位置
    if (parentWidget()) {
        setFixedSize(parentWidget()->size());
        move(parentWidget()->pos());
    } else {
        setFixedSize(715, 570);
    }

    // 禁用改变大小，营造覆盖替代页面的感觉
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // 1. 顶部工具栏 (Select, Create, Delete, Save, Apply)
    QHBoxLayout *topLayout = new QHBoxLayout();
    cbModelSelect = new QComboBox(this);
    cbModelSelect->setMinimumHeight(28);
    cbModelSelect->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    btnCreate = new QPushButton(tr("创建机型"), this);
    btnCreate->setMinimumHeight(28);
    btnDelete = new QPushButton(tr("删除机型"), this);
    btnDelete->setMinimumHeight(28);
    btnSave = new QPushButton(tr("保存修改"), this);
    btnSave->setMinimumHeight(28);
    btnApply = new QPushButton(tr("应用此机型"), this);
    btnApply->setMinimumHeight(28);

    topLayout->addWidget(cbModelSelect, 1);
    topLayout->addWidget(btnCreate);
    topLayout->addWidget(btnDelete);
    topLayout->addWidget(btnSave);
    topLayout->addWidget(btnApply);
    mainLayout->addLayout(topLayout);

    connect(cbModelSelect, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ModelCfgDialog::onModelChanged);
    connect(btnCreate, &QPushButton::clicked, this, &ModelCfgDialog::onCreateClicked);
    connect(btnDelete, &QPushButton::clicked, this, &ModelCfgDialog::onDeleteClicked);
    connect(btnSave, &QPushButton::clicked, this, &ModelCfgDialog::onSaveClicked);
    connect(btnApply, &QPushButton::clicked, this, &ModelCfgDialog::onApplyClicked);

    // 2. 表单配置区域
    QGroupBox *grpConfig = new QGroupBox(tr("机型参数"), this);
    QVBoxLayout *grpLayout = new QVBoxLayout(grpConfig);
    grpLayout->setSpacing(8); // 缩小控件之间的留白
    grpLayout->setContentsMargins(10, 10, 10, 10);

    auto createHBox = []() -> QHBoxLayout* {
        QHBoxLayout *hLayout = new QHBoxLayout();
        hLayout->setSpacing(10);
        hLayout->setContentsMargins(0, 0, 0, 0);
        return hLayout;
    };

    // Style helper for consistent height
    auto applyStyle = [](QWidget* w) {
        w->setMinimumHeight(28);
        w->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    };

    // 统一设置固定宽度的 Label 让并排的两列文本起点完全对齐
    auto createLabel = [](const QString& text) -> QLabel* {
        QLabel* lbl = new QLabel(text);
        lbl->setMinimumWidth(125); // 控制相同宽度
        lbl->setMaximumWidth(125);
        lbl->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        return lbl;
    };

    // 第一行: Version 和 Host Type 并排
    QHBoxLayout *row1 = createHBox();
    
    cbVersion = new QComboBox(this);
    cbVersion->addItem("ZSERIES", ZSERIES);
    cbVersion->addItem("XSERIES", XSERIES);
    cbVersion->addItem("IAPLINK", IAPLINK);
    applyStyle(cbVersion);
    row1->addWidget(createLabel(tr("Version:")));
    row1->addWidget(cbVersion, 1);

    cbHostType = new QComboBox(this);
    cbHostType->addItem("HOST_HANDHELD", HOST_HANDHELD);
    cbHostType->addItem("HOST_BRUSH", HOST_BRUSH);
    cbHostType->addItem("HOST_BOTTOM", HOST_BOTTOM);
    cbHostType->addItem("HOST_BATTERY", HOST_BATTERY);
    cbHostType->addItem("HOST_SELFDEFINE", HOST_SELFDEFINE);
    applyStyle(cbHostType);
    row1->addWidget(createLabel(tr("Host Type:")));
    row1->addWidget(cbHostType, 1);
    
    grpLayout->addLayout(row1);

    // 第二行: Baudrates 并排
    QHBoxLayout *row2 = createHBox();
    
    cbNormalRxBaudrate = new QComboBox(this);
    cbNormalRxBaudrate->setEditable(true);
    cbNormalRxBaudrate->addItems({"9600", "19200", "38400", "57600", "115200", "460800", "921600"});
    applyStyle(cbNormalRxBaudrate);
    row2->addWidget(createLabel(tr("NormRX Baudrate:")));
    row2->addWidget(cbNormalRxBaudrate, 1);

    cbUpgradeBaudrate = new QComboBox(this);
    cbUpgradeBaudrate->setEditable(true);
    cbUpgradeBaudrate->addItems({"9600", "19200", "38400", "57600", "115200", "460800", "921600"});
    applyStyle(cbUpgradeBaudrate);
    row2->addWidget(createLabel(tr("Upgrade Baudrate:")));
    row2->addWidget(cbUpgradeBaudrate, 1);
    
    grpLayout->addLayout(row2);

    // 第三行: RX Check Len 和 Verify Mode 并排
    QHBoxLayout *row3 = createHBox();
    
    sbNormalRxCheckLength = new QSpinBox(this);
    sbNormalRxCheckLength->setRange(0, 255);
    applyStyle(sbNormalRxCheckLength);
    row3->addWidget(createLabel(tr("NormRX Check Len:")));
    row3->addWidget(sbNormalRxCheckLength, 1);

    cbVerifyMode = new QComboBox(this);
    cbVerifyMode->addItem("VERIFY_CRC16", VERIFY_CRC16);
    cbVerifyMode->addItem("VERIFY_ADD8", VERIFY_ADD8);
    cbVerifyMode->addItem("VERIFY_ADD16", VERIFY_ADD16);
    applyStyle(cbVerifyMode);
    row3->addWidget(createLabel(tr("Verify Mode:")));
    row3->addWidget(cbVerifyMode, 1);
    
    grpLayout->addLayout(row3);

    // 第四行: Upgrade Cmd 独占一行
    QHBoxLayout *row4 = createHBox();
    leUpgradeCmd = new QLineEdit(this);
    applyStyle(leUpgradeCmd);
    row4->addWidget(createLabel(tr("Upgrade Cmd:")));
    row4->addWidget(leUpgradeCmd, 1);
    grpLayout->addLayout(row4);

    // 第四行.5: Handshake check buffer 独占一行
    QHBoxLayout *row4_5 = createHBox();
    leHandshakeCheckBuffer = new QLineEdit(this);
    applyStyle(leHandshakeCheckBuffer);
    row4_5->addWidget(createLabel(tr("Handshake Check:")));
    row4_5->addWidget(leHandshakeCheckBuffer, 1);
    grpLayout->addLayout(row4_5);

    // 第五行: Check Buffer 独占一行
    QHBoxLayout *row5_1 = createHBox();
    leNormalRxCheckBuffer = new QLineEdit(this);
    applyStyle(leNormalRxCheckBuffer);
    row5_1->addWidget(createLabel(tr("RX Check Pos:")));
    row5_1->addWidget(leNormalRxCheckBuffer, 1);
    grpLayout->addLayout(row5_1);

    // 第五行.5: Check Value 独占一行
    QHBoxLayout *row5_2 = createHBox();
    leNormalRxCheckValue = new QLineEdit(this);
    applyStyle(leNormalRxCheckValue);
    row5_2->addWidget(createLabel(tr("RX Check Val:")));
    row5_2->addWidget(leNormalRxCheckValue, 1);
    grpLayout->addLayout(row5_2);

    // 剩下的 Inquiry 以及最后的 Address 项仍然可以使用 FormLayout 或者 QVBoxLayout
    QFormLayout *formLayout = new QFormLayout();
    formLayout->setSpacing(8);
    formLayout->setContentsMargins(0, 0, 0, 0);

    // 对于下方比较宽的项目保留长 Label 宽度防止挤压
    auto createWideRowLabel = [](const QString& text) -> QLabel* {
        QLabel* lbl = new QLabel(text);
        lbl->setMinimumWidth(150);
        return lbl;
    };

    chkInquiryEnable = new QCheckBox(tr("Enable Inquiry"), this);
    applyStyle(chkInquiryEnable);
    formLayout->addRow(chkInquiryEnable);

    // Inquiry Table
    tbInquiry = new QTableWidget(4, 3, this);
    tbInquiry->setHorizontalHeaderLabels({tr("Content"), tr("Cmd (Hex)"), tr("Inquiry Res Check (Hex)")});
    tbInquiry->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    tbInquiry->horizontalHeader()->setStretchLastSection(false);
    tbInquiry->verticalHeader()->setVisible(false);
    tbInquiry->setMinimumHeight(150);
    // Initialize items to avoid null pointers later
    for (int i = 0; i < 4; ++i) {
        tbInquiry->setItem(i, 0, new QTableWidgetItem(""));
        tbInquiry->setItem(i, 1, new QTableWidgetItem(""));
        tbInquiry->setItem(i, 2, new QTableWidgetItem(""));
    }
    formLayout->addRow(tbInquiry);

    // Connect checkbox toggle to table enable/disable state
    connect(chkInquiryEnable, &QCheckBox::toggled, tbInquiry, &QWidget::setEnabled);
    connect(tbInquiry->horizontalHeader(), &QHeaderView::sectionResized, this,
            [this](int logicalIndex, int oldSize, int newSize) {
                if (m_restoringInquiryColumnWidths || logicalIndex < 0 || logicalIndex >= 3) {
                    return;
                }

                int delta = newSize - oldSize;
                if (delta == 0) {
                    return;
                }

                int adjustIndex = (logicalIndex == 2) ? 1 : 2;
                if (adjustIndex < 0 || adjustIndex >= 3) {
                    return;
                }

                const int minWidth = 40;
                int currentAdjust = tbInquiry->columnWidth(adjustIndex);
                int targetAdjust = currentAdjust - delta;

                m_restoringInquiryColumnWidths = true;
                if (targetAdjust < minWidth) {
                    int lack = minWidth - targetAdjust;
                    targetAdjust = minWidth;

                    int currentResized = tbInquiry->columnWidth(logicalIndex);
                    int targetResized = currentResized - lack;
                    if (targetResized < minWidth) {
                        targetResized = minWidth;
                    }
                    tbInquiry->setColumnWidth(logicalIndex, targetResized);
                }

                tbInquiry->setColumnWidth(adjustIndex, targetAdjust);
                m_restoringInquiryColumnWidths = false;
            });
    // Initialize state
    tbInquiry->setEnabled(false);

    // 第六行：将 App Flash Start Addr 和第一地址校验 并排
    QHBoxLayout *row6 = createHBox();
    leAppFlashStAddr = new QLineEdit(this); // Hex format address
    applyStyle(leAppFlashStAddr);
    row6->addWidget(createLabel(tr("Flash Start Addr (Hex):")));
    row6->addWidget(leAppFlashStAddr, 1);

    leFirstAddrCheck = new QLineEdit(this); // Hex format address check
    applyStyle(leFirstAddrCheck);
    row6->addWidget(createLabel(tr("第一地址校验:")));
    row6->addWidget(leFirstAddrCheck, 1);

    // 我们把行6直接加到外层的大 Layout 里
    grpLayout->addLayout(formLayout);
    grpLayout->addLayout(row6);
    
    mainLayout->addWidget(grpConfig);
}

void ModelCfgDialog::createDefaultModels()
{
    m_modelsArray = QJsonArray();

    auto appendModel = [this](const QString& name, const ModelInfoStruct& info, const QVector<int>& widths) {
        QJsonObject obj = structToJson(info);
        obj["model_name"] = name;
        obj[kBuiltInModelKey] = true;
        QJsonArray widthArr;
        widthArr.append(widths.value(0, 223));
        widthArr.append(widths.value(1, 223));
        widthArr.append(widths.value(2, 223));
        obj["inquiry_column_widths"] = widthArr;
        m_modelsArray.append(obj);
    };

    ModelInfoStruct handheld;
    handheld.verison = XSERIES;
    handheld.host_type = HOST_HANDHELD;
    handheld.normal_rx_baudrate = 115200;
    handheld.upgrade_baudrate = 115200;
    handheld.verify_mode = VERIFY_ADD8;
    handheld.upgrade_cmd = QByteArray::fromBase64("rAIAAN3d3QBQAACV");
    handheld.handshake_check_buffer = QByteArray::fromBase64("rAAC3d3dRQ==");
    handheld.normal_rx_check_buffer = QByteArray::fromBase64("AQEBAAAAAg==");
    handheld.normal_rx_check_value = QByteArray::fromBase64("rAACAAAAAA==");
    handheld.normal_rx_check_length = 7;
    handheld.inquiry_enable = 1;
    handheld.inquiry_cmd[0] = QByteArray::fromBase64("rAIAAN3d4AAAAABI");
    handheld.inquiry_cmd[1] = QByteArray::fromBase64("rAIAAN3d4gAAAABK");
    handheld.inquiry_cmd[2] = QByteArray::fromBase64("rAIAAN3d4gAAAABK");
    handheld.inquiry_cmd[3] = QByteArray::fromBase64("rAIAAN3d4QAAAABJ");
    handheld.inquiry_content[0] = tr("软件版本");
    handheld.inquiry_content[1] = "CRC16";
    handheld.inquiry_content[2] = tr("海拔模式");
    handheld.inquiry_content[3] = tr("滤芯使用时间");
    handheld.inquiry_res_check[0] = QByteArray::fromBase64("A+AAAAE=");
    handheld.inquiry_res_check[1] = QByteArray::fromBase64("A+IBAQg=");
    handheld.inquiry_res_check[2] = QByteArray::fromBase64("A+MAAAE=");
    handheld.inquiry_res_check[3] = QByteArray::fromBase64("A+EPREg=");
    handheld.app_flash_st_addr = 0x5000;
    handheld.first_addr_check = 0;
    appendModel(tr("手持"), handheld, {208, 264, 197});

    ModelInfoStruct brush;
    brush.verison = XSERIES;
    brush.host_type = HOST_BRUSH;
    brush.normal_rx_baudrate = 115200;
    brush.upgrade_baudrate = 115200;
    brush.verify_mode = VERIFY_ADD8;
    brush.upgrade_cmd.clear();
    brush.handshake_check_buffer = QByteArray::fromBase64("rAUA3d3dAAAASA==");
    brush.normal_rx_check_buffer = QByteArray::fromBase64("AQEBAAAAAAAAAg==");
    brush.normal_rx_check_value = QByteArray::fromBase64("rAUAAAAAAAAAAA==");
    brush.normal_rx_check_length = 10;
    brush.inquiry_enable = 0;
    brush.app_flash_st_addr = 0x5000;
    brush.first_addr_check = 0;
    appendModel(tr("附件"), brush, {223, 223, 223});

    ModelInfoStruct base;
    base.verison = XSERIES;
    base.host_type = HOST_BOTTOM;
    base.normal_rx_baudrate = 115200;
    base.upgrade_baudrate = 115200;
    base.verify_mode = VERIFY_ADD8;
    base.upgrade_cmd.clear();
    base.handshake_check_buffer = QByteArray::fromBase64("rAYA3d3dAAAASQ==");
    base.normal_rx_check_buffer.clear();
    base.normal_rx_check_value.clear();
    base.normal_rx_check_length = 10;
    base.inquiry_enable = 0;
    base.app_flash_st_addr = 0x5000;
    base.first_addr_check = 0;
    appendModel(tr("底座"), base, {223, 223, 223});

    ModelInfoStruct battery;
    battery.verison = XSERIES;
    battery.host_type = HOST_BATTERY;
    battery.normal_rx_baudrate = 9600;
    battery.upgrade_baudrate = 115200;
    battery.verify_mode = VERIFY_ADD16;
    battery.upgrade_cmd = QByteArray::fromBase64("rP4B/gH9");
    battery.handshake_check_buffer = QByteArray::fromBase64("rO8D3d3dA4k=");
    battery.normal_rx_check_buffer = QByteArray::fromBase64("AQEBAAAAAgI=");
    battery.normal_rx_check_value = QByteArray::fromBase64("rO8DAAAAAAA=");
    battery.normal_rx_check_length = 8;
    battery.inquiry_enable = 1;
    battery.app_flash_st_addr = 0x2800;
    battery.first_addr_check = 0;
    appendModel(tr("电池包"), battery, {223, 223, 223});

    m_currentIndex = 0;
    saveConfig();
}

void ModelCfgDialog::loadConfig()
{
    QFile file(m_configFilePath);
    if (!file.exists() || !file.open(QIODevice::ReadOnly)) {
        m_inquiryColumnWidths = {tbInquiry->columnWidth(0), tbInquiry->columnWidth(1), tbInquiry->columnWidth(2)};
        createDefaultModels();
    } else {
        QByteArray data = file.readAll();
        file.close();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonObject root = doc.object();

        QJsonArray widthArr = root["inquiry_column_widths"].toArray();
        m_inquiryColumnWidths.clear();
        for (int i = 0; i < widthArr.size() && i < 3; ++i) {
            int width = widthArr[i].toInt();
            if (width > 0) {
                m_inquiryColumnWidths.append(width);
            }
        }
        if (m_inquiryColumnWidths.size() != 3) {
            m_inquiryColumnWidths = {tbInquiry->columnWidth(0), tbInquiry->columnWidth(1), tbInquiry->columnWidth(2)};
        }
        
        m_modelsArray = root["models"].toArray();
        m_currentIndex = root["selected_model_index"].toInt(0); // 取 index，默认 0
        
        if (m_modelsArray.isEmpty()) {
            createDefaultModels();
            return;
        }

        bool missingFlag = false;
        for (int i = 0; i < m_modelsArray.size(); ++i) {
            if (!m_modelsArray[i].toObject().contains(kBuiltInModelKey)) {
                missingFlag = true;
                break;
            }
        }

        if (missingFlag) {
            for (int i = 0; i < m_modelsArray.size(); ++i) {
                QJsonObject obj = m_modelsArray[i].toObject();
                obj[kBuiltInModelKey] = (i < 4);
                m_modelsArray[i] = obj;
            }
            saveConfig();
        }
    }

    // 更新下拉框
    cbModelSelect->blockSignals(true);
    cbModelSelect->clear();
    for (int i = 0; i < m_modelsArray.size(); ++i) {
        QJsonObject obj = m_modelsArray[i].toObject();
        QString name = obj["model_name"].toString();
        cbModelSelect->addItem(name);
    }
    
    if (m_currentIndex < 0 || m_currentIndex >= m_modelsArray.size()) m_currentIndex = 0;
    cbModelSelect->setCurrentIndex(m_currentIndex);
    cbModelSelect->blockSignals(false);

    m_restoringInquiryColumnWidths = true;
    for (int i = 0; i < 3 && i < m_inquiryColumnWidths.size(); ++i) {
        tbInquiry->setColumnWidth(i, m_inquiryColumnWidths[i]);
    }
    m_restoringInquiryColumnWidths = false;
    
    // 更新UI显示
    onModelChanged(m_currentIndex);
}

void ModelCfgDialog::saveConfig()
{
    QJsonObject root;
    root["selected_model_index"] = m_currentIndex;
    root["models"] = m_modelsArray;

    QJsonDocument doc(root);
    QFile file(m_configFilePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();
    }
}

void ModelCfgDialog::onModelChanged(int index)
{
    if (index < 0 || index >= m_modelsArray.size()) return;
    QJsonObject obj = m_modelsArray[index].toObject();
    btnDelete->setEnabled(!obj.value(kBuiltInModelKey).toBool(false));

    QJsonArray widthArr = obj["inquiry_column_widths"].toArray();
    QVector<int> widths;
    for (int i = 0; i < widthArr.size() && i < 3; ++i) {
        int width = widthArr[i].toInt();
        if (width > 0) {
            widths.append(width);
        }
    }

    if (widths.size() != 3) {
        if (m_inquiryColumnWidths.size() == 3) {
            widths = m_inquiryColumnWidths;
        } else {
            widths = {tbInquiry->columnWidth(0), tbInquiry->columnWidth(1), tbInquiry->columnWidth(2)};
        }
    }

    m_restoringInquiryColumnWidths = true;
    for (int i = 0; i < 3; ++i) {
        tbInquiry->setColumnWidth(i, widths[i]);
    }
    m_restoringInquiryColumnWidths = false;

    ModelInfoStruct info = jsonToStruct(obj);
    updateUiFromStruct(info);
}

void ModelCfgDialog::onCreateClicked()
{
    bool ok;
    QString text = QInputDialog::getText(this, tr("新建机型"), tr("输入机型名称:"), QLineEdit::Normal, "", &ok);
    if (ok && !text.isEmpty()) {
        // 防止重名
        for (int i = 0; i < m_modelsArray.size(); ++i) {
            if (m_modelsArray[i].toObject()["model_name"].toString() == text) {
                QMessageBox::warning(this, tr("错误"), tr("机型名称已存在!"));
                return;
            }
        }
        
        // 创建空白机型：输入框清空，下拉框使用默认项
        ModelInfoStruct newInfo;
        newInfo.verison = cbVersion->itemData(0).toInt();
        newInfo.host_type = cbHostType->itemData(0).toInt();
        newInfo.normal_rx_baudrate = cbNormalRxBaudrate->itemText(0).toUInt();
        newInfo.upgrade_baudrate = cbUpgradeBaudrate->itemText(0).toUInt();
        newInfo.verify_mode = cbVerifyMode->itemData(0).toInt();
        newInfo.upgrade_cmd.clear();
        newInfo.handshake_check_buffer.clear();
        newInfo.normal_rx_check_buffer.clear();
        newInfo.normal_rx_check_value.clear();
        newInfo.normal_rx_check_length = 0;
        newInfo.inquiry_enable = 0;
        for (int i = 0; i < 4; ++i) {
            newInfo.inquiry_cmd[i].clear();
            newInfo.inquiry_content[i].clear();
            newInfo.inquiry_res_check[i].clear();
        }
        newInfo.app_flash_st_addr = 0;
        newInfo.first_addr_check = 0;

        QJsonObject obj = structToJson(newInfo);
        obj["model_name"] = text;
        obj[kBuiltInModelKey] = false;
        QJsonArray widthArr;
        widthArr.append(tbInquiry->columnWidth(0));
        widthArr.append(tbInquiry->columnWidth(1));
        widthArr.append(tbInquiry->columnWidth(2));
        obj["inquiry_column_widths"] = widthArr;
        m_modelsArray.append(obj);
        
        m_currentIndex = m_modelsArray.size() - 1;
        saveConfig();
        loadConfig();
    }
}

void ModelCfgDialog::onDeleteClicked()
{
    int index = cbModelSelect->currentIndex();
    if (index < 0) {
        return;
    }

    QJsonObject obj = m_modelsArray[index].toObject();
    if (obj.value(kBuiltInModelKey).toBool(false)) {
        QMessageBox::warning(this, tr("警告"), tr("默认机型不允许删除!"));
        return;
    }

    if (m_modelsArray.size() <= 1) {
        QMessageBox::warning(this, tr("警告"), tr("必须保留至少一个机型!"));
        return;
    }

    m_modelsArray.removeAt(index);
    m_currentIndex = 0;
    saveConfig();
    loadConfig();
}

void ModelCfgDialog::onSaveClicked()
{
    int index = cbModelSelect->currentIndex();
    if (index < 0) return;

    QString modelName = cbModelSelect->currentText();
    
    ModelInfoStruct info;
    updateStructFromUi(info);
    
    QJsonObject obj = structToJson(info);
    obj["model_name"] = modelName;
    obj[kBuiltInModelKey] = m_modelsArray[index].toObject().value(kBuiltInModelKey).toBool(false);
    QJsonArray widthArr;
    widthArr.append(tbInquiry->columnWidth(0));
    widthArr.append(tbInquiry->columnWidth(1));
    widthArr.append(tbInquiry->columnWidth(2));
    obj["inquiry_column_widths"] = widthArr;
    m_modelsArray[index] = obj;

    m_inquiryColumnWidths = {tbInquiry->columnWidth(0), tbInquiry->columnWidth(1), tbInquiry->columnWidth(2)};
    
    m_currentIndex = index;  // 同步更新保存指向

    saveConfig();
    QMessageBox::information(this, tr("成功"), tr("当前机型配置已保存"));
}

void ModelCfgDialog::onApplyClicked()
{
    int index = cbModelSelect->currentIndex();
    if (index < 0) return;

    // 应用并不需要额外去从 UI 中抽出结构体保存到 Json
    // 我们只需要把当前的 Index 记下来就好，这样就相当于锁定了这个选则
    m_currentIndex = index;

    saveConfig(); // 将已选 index 回刷到文件里
    
    // 返回 QDialog::Accepted，并关闭窗口。主函数将根据返回结果执行逻辑
    accept(); 
}

// ============== 数据结构与UI绑定部分 ==============

QByteArray ModelCfgDialog::hexStringToByteArray(const QString& str)
{
    return QByteArray::fromHex(str.toUtf8());
}

QString ModelCfgDialog::byteArrayToHexString(const QByteArray& arr)
{
    return QString(arr.toHex(' ').toUpper());
}

void ModelCfgDialog::updateUiFromStruct(const ModelInfoStruct& info)
{
    cbVersion->setCurrentIndex(cbVersion->findData(info.verison));
    cbHostType->setCurrentIndex(cbHostType->findData(info.host_type));
    cbNormalRxBaudrate->setCurrentText(QString::number(info.normal_rx_baudrate));
    cbUpgradeBaudrate->setCurrentText(QString::number(info.upgrade_baudrate));
    cbVerifyMode->setCurrentIndex(cbVerifyMode->findData(info.verify_mode));
    
    leUpgradeCmd->setText(byteArrayToHexString(info.upgrade_cmd));
    leHandshakeCheckBuffer->setText(byteArrayToHexString(info.handshake_check_buffer));
    leNormalRxCheckBuffer->setText(byteArrayToHexString(info.normal_rx_check_buffer));
    leNormalRxCheckValue->setText(byteArrayToHexString(info.normal_rx_check_value));
    
    sbNormalRxCheckLength->setValue(info.normal_rx_check_length);
    
    // Set checkbox state and trigger table enable/disable
    chkInquiryEnable->setChecked(info.inquiry_enable > 0);
    tbInquiry->setEnabled(info.inquiry_enable > 0);
    
    for (int i = 0; i < 4; ++i) {
        if (!tbInquiry->item(i, 0)) tbInquiry->setItem(i, 0, new QTableWidgetItem(""));
        if (!tbInquiry->item(i, 1)) tbInquiry->setItem(i, 1, new QTableWidgetItem(""));
        if (!tbInquiry->item(i, 2)) tbInquiry->setItem(i, 2, new QTableWidgetItem(""));
        
        tbInquiry->item(i, 0)->setText(info.inquiry_content[i]);
        tbInquiry->item(i, 1)->setText(byteArrayToHexString(info.inquiry_cmd[i]));
        tbInquiry->item(i, 2)->setText(byteArrayToHexString(info.inquiry_res_check[i]));
    }
    
    if (info.app_flash_st_addr == 0) {
        leAppFlashStAddr->clear();
    } else {
        leAppFlashStAddr->setText("0x" + QString::number(info.app_flash_st_addr, 16).toUpper());
    }

    if (info.first_addr_check == 0) {
        leFirstAddrCheck->clear();
    } else {
        leFirstAddrCheck->setText("0x" + QString::number(info.first_addr_check, 16).toUpper());
    }
}

void ModelCfgDialog::updateStructFromUi(ModelInfoStruct& info)
{
    info.verison = cbVersion->currentData().toInt();
    info.host_type = cbHostType->currentData().toInt();
    info.normal_rx_baudrate = cbNormalRxBaudrate->currentText().toUInt();
    info.upgrade_baudrate = cbUpgradeBaudrate->currentText().toUInt();
    info.verify_mode = cbVerifyMode->currentData().toInt();
    
    info.upgrade_cmd = hexStringToByteArray(leUpgradeCmd->text());
    info.handshake_check_buffer = hexStringToByteArray(leHandshakeCheckBuffer->text());
    info.normal_rx_check_buffer = hexStringToByteArray(leNormalRxCheckBuffer->text());
    info.normal_rx_check_value = hexStringToByteArray(leNormalRxCheckValue->text());
    info.normal_rx_check_length = sbNormalRxCheckLength->value();
    info.inquiry_enable = chkInquiryEnable->isChecked() ? 1 : 0;
    
    for (int i = 0; i < 4; ++i) {
        QString contentText = tbInquiry->item(i, 0) ? tbInquiry->item(i, 0)->text() : "";
        QString cmdText = tbInquiry->item(i, 1) ? tbInquiry->item(i, 1)->text() : "";
        QString resCheckText = tbInquiry->item(i, 2) ? tbInquiry->item(i, 2)->text() : "";
        info.inquiry_content[i] = contentText;
        info.inquiry_cmd[i] = hexStringToByteArray(cmdText);
        info.inquiry_res_check[i] = hexStringToByteArray(resCheckText);
    }
    
    bool ok;
    info.app_flash_st_addr = leAppFlashStAddr->text().toUInt(&ok, 16);
    info.first_addr_check = leFirstAddrCheck->text().toUInt(&ok, 16);
}

// ============== 数据结构与JSON绑定部分 ==============

QJsonObject ModelCfgDialog::structToJson(const ModelInfoStruct& info)
{
    QJsonObject obj;
    obj["verison"] = info.verison;
    obj["host_type"] = info.host_type;
    obj["normal_rx_baudrate"] = (qint64)info.normal_rx_baudrate;
    obj["upgrade_baudrate"] = (qint64)info.upgrade_baudrate;
    obj["verify_mode"] = info.verify_mode;
    
    obj["upgrade_cmd"] = QString(info.upgrade_cmd.toBase64());
    obj["handshake_check_buffer"] = QString(info.handshake_check_buffer.toBase64());
    obj["normal_rx_check_buffer"] = QString(info.normal_rx_check_buffer.toBase64());
    obj["normal_rx_check_value"] = QString(info.normal_rx_check_value.toBase64());
    
    obj["normal_rx_check_length"] = info.normal_rx_check_length;
    obj["inquiry_enable"] = info.inquiry_enable;
    
    QJsonArray cmdArr;
    QJsonArray contentArr;
    QJsonArray resCheckArr;
    for (int i = 0; i < 4; ++i) {
        cmdArr.append(QString(info.inquiry_cmd[i].toBase64()));
        contentArr.append(info.inquiry_content[i]);
        resCheckArr.append(QString(info.inquiry_res_check[i].toBase64()));
    }
    obj["inquiry_cmd"] = cmdArr;
    obj["inquiry_content"] = contentArr;
    obj["inquiry_res_check"] = resCheckArr;
    
    obj["app_flash_st_addr"] = (qint64)info.app_flash_st_addr;
    obj["first_addr_check"] = (qint64)info.first_addr_check;
    return obj;
}

ModelInfoStruct ModelCfgDialog::jsonToStruct(const QJsonObject& obj)
{
    ModelInfoStruct info;
    info.verison = obj["verison"].toInt();
    info.host_type = obj["host_type"].toInt();
    info.normal_rx_baudrate = obj["normal_rx_baudrate"].toInt();
    info.upgrade_baudrate = obj["upgrade_baudrate"].toInt();
    info.verify_mode = obj["verify_mode"].toInt();
    
    info.upgrade_cmd = QByteArray::fromBase64(obj["upgrade_cmd"].toString().toUtf8());
    info.handshake_check_buffer = QByteArray::fromBase64(obj["handshake_check_buffer"].toString().toUtf8());
    info.normal_rx_check_buffer = QByteArray::fromBase64(obj["normal_rx_check_buffer"].toString().toUtf8());
    info.normal_rx_check_value = QByteArray::fromBase64(obj["normal_rx_check_value"].toString().toUtf8());
    
    info.normal_rx_check_length = obj["normal_rx_check_length"].toInt();
    info.inquiry_enable = obj["inquiry_enable"].toInt();
    
    QJsonArray cmdArr = obj["inquiry_cmd"].toArray();
    QJsonArray contentArr = obj["inquiry_content"].toArray();
    QJsonArray resCheckArr = obj["inquiry_res_check"].toArray();
    for (int i = 0; i < 4; ++i) {
        if (i < cmdArr.size()) {
            info.inquiry_cmd[i] = QByteArray::fromBase64(cmdArr[i].toString().toUtf8());
        }
        if (i < contentArr.size()) {
            info.inquiry_content[i] = contentArr[i].toString();
        }
        if (i < resCheckArr.size()) {
            info.inquiry_res_check[i] = QByteArray::fromBase64(resCheckArr[i].toString().toUtf8());
        }
    }
    
    info.app_flash_st_addr = obj["app_flash_st_addr"].toInt();
    info.first_addr_check = obj["first_addr_check"].toInt();
    return info;
}
