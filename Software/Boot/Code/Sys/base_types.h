#ifndef __BASE_TYPES_H__
#define __BASE_TYPES_H__

#include <stdint.h>

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
    PACKET_ACK_OK               = 0x00,
    PACKET_ACK_ERROR            = 0x01,
    PACKET_ACK_ABORT            = 0x02,
    PACKET_ACK_TIMEOUT          = 0x03,
    PACKET_ACK_ADDR_ERROR       = 0x04,
    PACKET_ACK_FLASH_SIZE_ERROR = 0x05,
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
    PACKET_FLASH_CRC_INDEX  = 0x0C,
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

typedef void         (*func_ptr_t)(void);

typedef enum
{
    Ok                          = 0u,  /*!< No error */
    Error                       = 1u,  /*!< Non-specific error code */
    ErrorAddressAlignment       = 2u,  /*!< Non-specific error code */
    ErrorAccessRights           = 3u,  /*!< Wrong mode (e.g. user/system) mode is set */
    ErrorInvalidParameter       = 4u,  /*!< Provided parameter is not valid */
    ErrorOperationInProgress    = 5u,  /*!< A conflicting or requested operation is still in progress */
    ErrorInvalidMode            = 6u,  /*!< Operation not allowed in current mode */
    ErrorUninitialized          = 7u,  /*!< Module (or part of it) was not initialized properly */
    ErrorBufferFull             = 8u,  /*!< Circular buffer can not be written because the buffer is full */
    ErrorTimeout                = 9u,  /*!< Time Out error occurred (e.g. I2C arbitration lost, Flash time-out, etc. */
    ErrorNotReady               = 10u, /*!< A requested final state is not reached */
    OperationInProgress         = 11u  /*!< Indicator for operation in progress*/
}en_result_t;


typedef struct{
    uint8_t FrameRecvStatus;
    uint32_t FrameSize;
    uint32_t FrameDataIndex;
}RecvStruct;


typedef struct{
    uint16_t crcFlash;
    uint8_t transNumber;
    uint8_t transStatus;
    uint8_t iapIngFlag;
    uint8_t recvType;
    uint8_t appIsRuning;
}TransStruct;


typedef struct{
    uint8_t timerEn;
    uint32_t timerTick;
    uint32_t timeout;
}TimerStruct;


typedef struct{
    uint8_t transProcess;
    uint32_t flashAddr;
    uint8_t threadSta;
    uint8_t transBuffer[PACKET_DATA_SIZE];
    uint32_t fileLength;
    uint32_t fileIndex;
    uint32_t transFileSize;
    uint8_t txResult;
    uint16_t crc16;
    uint8_t sendAlready;
}LoadingPmStruct;

#endif // __BASE_TYPES_H__

