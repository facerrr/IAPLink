#ifndef CONFIG_H
#define CONFIG_H

#include <QObject>
#include <QString>

#define ZSERIES 0
#define XSERIES 1
#define IAPLINK 2

#define RECV_NORMAL         0
#define RECV_ZSERIES_IAP    1
#define RECV_XSERIES_IAP    2
#define RECV_ONEWIRE_IAP    3

#define HOST_HANDHELD       0
#define HOST_BRUSH          1
#define HOST_BOTTOM         2
#define HOST_BATTERY        3
#define HOST_SELFDEFINE     4

#define VERIFY_CRC16   0
#define VERIFY_ADD8    1
#define VERIFY_ADD16   2

enum en_thread_number_t
{
    TransFileThead  = 0x01,
    RecvFileThead   = 0x02,
};

enum en_trans_status_t
{
    TransIdle           = 0x00,
    TransBegin          = 0x01,
    TransTimeout        = 0x02,
    TransFinished       = 0x03,
    TransFailed         = 0x04,
    TransAbort          = 0x05,
    TransAddrError      = 0x06,
    TransFileInvalid    = 0x07,
};

enum en_packet_type_t
{
    PACKET_TYPE_CONTROL     = 0x11,
    PACKET_TYPE_DATA        = 0x12,
};

enum en_packet_cmd_t
{
    PACKET_CMD_HANDSHAKE    = 0x20,
    PACKET_CMD_JUMP_TO_APP  = 0x21,
    PACKET_CMD_APP_DOWNLOAD = 0x22,
    PACKET_CMD_APP_UPLOAD   = 0x23,
    PACKET_CMD_ERASE_FLASH  = 0x24,
    PACKET_CMD_FLASH_CRC    = 0x25,
    PACKET_CMD_APP_UPGRADE  = 0x26,
};

enum en_packet_status_t
{
    PACKET_ACK_OK           = 0x00,
    PACKET_ACK_ERROR        = 0x01,
    PACKET_ACK_ABORT        = 0x02,
    PACKET_ACK_TIMEOUT      = 0x03,
    PACKET_ACK_ADDR_ERROR   = 0x04,
};

enum en_frame_para_t
{
    FRAME_HEAD              = 0x6DAC,
    FRAME_SHELL_SIZE        = 8,
    FRAME_NUM_XOR_BYTE      = 0xFF,
    FRAME_MIN_SIZE          = 8,
    FRAME_MAX_SIZE          = 530,

    FRAME_HEAD_INDEX        = 0x00,
    FRAME_NUM_INDEX         = 0x02,
    FRAME_XORNUM_INDEX      = 0x03,
    FRAME_LENGTH_INDEX      = 0x04,
    FRAME_PACKET_INDEX      = 0x06,
};

enum en_packet_para_t
{
    PACKET_INSTRUCT_SIZE    = 6,
    PACKET_DATA_SIZE        = 512,
    PACKET_MIN_SIZE         = 6,
    PACKET_MAX_SIZE         = 522,

    PACKET_CMD_INDEX        = 0x06,
    PACKET_TYPE_INDEX       = 0x07,
    PACKET_RESULT_INDEX     = 0x07,
    PACKET_ADDRESS_INDEX    = 0x08,
    PACKET_DATA_INDEX       = 0x0C,
};

typedef enum
{
    FRAME_RECV_IDLE_STATUS       = 0x00,
    FRAME_RECV_HEADER_STATUS     = 0x01,
    FRAME_RECV_DATA_STATUS       = 0x02,
    FRAME_RECV_PROC_STATUS       = 0x03,
} en_frame_recv_status_t;


#define FRAME_HEAD_H_INDEX                  0x00
#define FRAME_HEAD_L_INDEX                  0x01
#define FRAME_NUM_INDEX                     0x02
#define FRAME_XORNUM_INDEX                  0x03
#define FRAME_LENGTH_INDEX                  0x04
#define FRAME_PACKET_INDEX                  0x06


struct IAPTransStruct{
    quint8 trans_status;
    quint8 is_upgrading;
    quint8 is_app_program;
    quint8 trans_numble;
    quint8 trans_process;
    quint8 recv_type;
    quint32 add8_flash;
    quint16 crc_flash;
};

struct ModelInfoStruct {
    quint8 verison = XSERIES;
    quint8 host_type = HOST_HANDHELD;
    quint32 normal_rx_baudrate = 115200;
    quint32 upgrade_baudrate = 115200;
    quint8 verify_mode = VERIFY_ADD8;
    QByteArray upgrade_cmd;
    QByteArray handshake_check_buffer;
    QByteArray normal_rx_check_buffer;
    QByteArray normal_rx_check_value;
    quint8 normal_rx_check_length;
    quint8 inquiry_enable;
    QByteArray inquiry_cmd[4];
    QString inquiry_content[4];
    QByteArray inquiry_res_check[4];
    quint32 app_flash_st_addr = 0x5000;
    quint32 first_addr_check = 0;
};

quint16 Cal_CRC16(quint8* data, int offset, quint32 size);
quint8 Cal_ADD8(quint8* data, int offset, quint32 size);
quint16 Cal_ADD16(quint8* data, int offset, quint32 size);

class config
{
public:
    config();
};

#endif // CONFIG_H
