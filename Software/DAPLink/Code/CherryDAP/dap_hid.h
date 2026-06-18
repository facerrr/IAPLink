#ifndef __DAP_HID_H
#define __DAP_HID_H

#include <stdio.h>
#include <stdint.h>

#define HID_PACK_REPORT_ID_LEN 1
#define HID_PACK_FRONT_LEN 2
#define HID_PACK_CMD_LEN 1
#define HID_PACK_LENGTH_LEN 1
#define HID_PACK_CRC_LEN 2

#define HID_PACK_HEADER_0 0x55
#define HID_PACK_HEADER_1 0xaa

#define HID_IN_REPORT_ID 0x02

void chry_dap_hid_exec(uint8_t *data, uint8_t len);

#endif // __DAP_HID_H
