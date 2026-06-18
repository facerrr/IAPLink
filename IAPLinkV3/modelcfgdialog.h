#ifndef MODELCFGDIALOG_H
#define MODELCFGDIALOG_H

#include <QDialog>
#include <QJsonObject>
#include <QJsonArray>
#include <QComboBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QTableWidget>
#include <QVector>
#include "config.h"

// 如果 config.h 中没有定义 ModelInfoStruct，请确保你的结构体在这里或 config.h 被完全声明
struct ModelInfoStruct;

class ModelCfgDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ModelCfgDialog(QWidget *parent = nullptr);
    ~ModelCfgDialog() {}
    int getSelectedIndex() const { return cbModelSelect->currentIndex(); }

private slots:
    void onModelChanged(int index);
    void onCreateClicked();
    void onDeleteClicked();
    void onApplyClicked();
    void onSaveClicked();

private:
    void setupUi();
    void loadConfig();
    void saveConfig();
    void updateUiFromStruct(const ModelInfoStruct& info);
    void updateStructFromUi(ModelInfoStruct& info);
    
    QJsonObject structToJson(const ModelInfoStruct& info);
    ModelInfoStruct jsonToStruct(const QJsonObject& obj);
    QByteArray hexStringToByteArray(const QString& str);
    QString byteArrayToHexString(const QByteArray& arr);
    
    void createDefaultModels();

    ModelInfoStruct *m_model_info;
    QString m_configFilePath;
    
    QJsonArray m_modelsArray;
    int m_currentIndex;
    QVector<int> m_inquiryColumnWidths;
    bool m_restoringInquiryColumnWidths = false;

    // UI 控制部分
    QComboBox *cbModelSelect;
    QPushButton *btnCreate;
    QPushButton *btnDelete;
    QPushButton *btnSave;
    QPushButton *btnApply;

    // ModelInfo 对应输入框
    QComboBox *cbVersion;
    QComboBox *cbHostType;
    QComboBox *cbNormalRxBaudrate;
    QComboBox *cbUpgradeBaudrate;
    QComboBox *cbVerifyMode;

    QLineEdit *leUpgradeCmd;
    QLineEdit *leHandshakeCheckBuffer;
    QLineEdit *leNormalRxCheckBuffer;
    QLineEdit *leNormalRxCheckValue;
    QSpinBox *sbNormalRxCheckLength;

    QCheckBox *chkInquiryEnable;
    QTableWidget *tbInquiry;

    QLineEdit *leAppFlashStAddr;
    QLineEdit *leFirstAddrCheck;
};

#endif // MODELCFGDIALOG_H
