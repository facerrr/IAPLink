#ifndef __USER_PARAM_H
#define __USER_PARAM_H

#include "eeprom.h"

#define PARAM_INITED_ADDR 0x0
#define PARAM_DAP_ID_ADDR 0x1

typedef struct {
    uint16_t inited;
    uint16_t dap_id;
    uint16_t major;
    uint16_t minor;
    uint16_t patch;
} user_param_t;

extern user_param_t user_param;

void user_param_init(void);

#endif // __USER_PARAM_H
