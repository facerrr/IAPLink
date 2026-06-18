#ifndef __FLASHX_H__
#define __FLASHX_H__

#include <stdint.h>
#include "at32f403a_407.h"
#include "base_types.h"

#define SECTOR_SIZE     2048

#define FLASH_SECTOR_SIZE       0x800ul
#define FLASH_SECTOR_NUM        512

// boot 48kb
#define FLASH_BOOT_SIZE         (24 * FLASH_SECTOR_SIZE)    // 24 * 2KB = 0x8000
#define FLASH_BOOT_ADDR_ST      (FLASH_BASE) 
#define FLASH_BOOT_ADDR_ED      (FLASH_BASE + FLASH_BOOT_SIZE - 1)

// info 2kb
#define FLASH_INFO_SIZE         (1 * FLASH_SECTOR_SIZE)
#define FLASH_INFO_ADDR_ST      (FLASH_BOOT_ADDR_ED + 1)    // 0x08008000
#define FLASH_INFO_ADDR_ED      (FLASH_INFO_ADDR_ST + FLASH_INFO_SIZE - 1) 

// daplink 100kb   0x0800C800
#define FLASH_DAP_SIZE          (50 * FLASH_SECTOR_SIZE)
#define FLASH_DAP_ADDR_ST       (FLASH_INFO_ADDR_ED + 1)
#define FLASH_DAP_ADDR_ED       (FLASH_DAP_ADDR_ST + FLASH_DAP_SIZE - 1)

// iaplink 100kb   0x08025800
#define FLASH_IAP_SIZE          (50 * FLASH_SECTOR_SIZE)
#define FLASH_IAP_ADDR_ST       (FLASH_DAP_ADDR_ED + 1)
#define FLASH_IAP_ADDR_ED       (FLASH_IAP_ADDR_ST + FLASH_IAP_SIZE - 1)

#define FLASH_BINK2_BASE        (0x08080000)
#define IAP_FILE_SIZE           (50 * FLASH_SECTOR_SIZE)
#define IAP_FILE_APP_SIZE       (49 * FLASH_SECTOR_SIZE)
#define IAP_FILE_INFO_SIZE      (1 * FLASH_SECTOR_SIZE)

// iap file1 100kb
#define FLASH_FILE1_SIZE        (49 * FLASH_SECTOR_SIZE)
#define FLASH_FILE1_ADDR_ST     (0x08080000)
#define FLASH_FILE1_ADDR_ED     (FLASH_FILE1_ADDR_ST + FLASH_FILE1_SIZE - 1)
#define FLASH_FILE1_INFO_ST     (FLASH_FILE1_ADDR_ED + 1)    //0x8098800
#define FLASH_FILE1_INFO_ED     (FLASH_FILE1_INFO_ST + 1 * FLASH_SECTOR_SIZE - 1)

// iap file2 100kb  0x08080000 + 1024 * 100 = 0x08099000
#define FLASH_FILE2_SIZE        (49 * FLASH_SECTOR_SIZE)
#define FLASH_FILE2_ADDR_ST     (FLASH_FILE1_INFO_ED + 1)
#define FLASH_FILE2_ADDR_ED     (FLASH_FILE2_ADDR_ST + FLASH_FILE2_SIZE - 1)
#define FLASH_FILE2_INFO_ST     (FLASH_FILE2_ADDR_ED + 1)
#define FLASH_FILE2_INFO_ED     (FLASH_FILE2_INFO_ST + 1 * FLASH_SECTOR_SIZE - 1)


// iap file3 100kb  0x08099000 + 1024 * 100 = 0x080B2000
#define FLASH_FILE3_SIZE        (49 * FLASH_SECTOR_SIZE)
#define FLASH_FILE3_ADDR_ST     (FLASH_FILE2_INFO_ED + 1)
#define FLASH_FILE3_ADDR_ED     (FLASH_FILE3_ADDR_ST + FLASH_FILE3_SIZE - 1)
#define FLASH_FILE3_INFO_ST     (FLASH_FILE3_ADDR_ED + 1)
#define FLASH_FILE3_INFO_ED     (FLASH_FILE3_INFO_ST + 1 * FLASH_SECTOR_SIZE - 1)

// iap file4 100kb  0x080B2000 + 1024 * 100 = 0x080D2000
#define FLASH_FILE4_SIZE        (49 * FLASH_SECTOR_SIZE)
#define FLASH_FILE4_ADDR_ST     (FLASH_FILE3_INFO_ED + 1)
#define FLASH_FILE4_ADDR_ED     (FLASH_FILE4_ADDR_ST + FLASH_FILE4_SIZE - 1)
#define FLASH_FILE4_INFO_ST     (FLASH_FILE4_ADDR_ED + 1)
#define FLASH_FILE4_INFO_ED     (FLASH_FILE4_INFO_ST + 1 * FLASH_SECTOR_SIZE - 1)

#define APP_UPGRADE             ((uint32_t)0x67890123)
#define APP_FLAG1               ((uint32_t)0x56781234)
#define APP_FLAG2               ((uint32_t)0x12345678)

en_result_t Flashx_WriteBytes(uint32_t u32Addr, uint8_t* pu8WriteBuff, uint32_t u32ByteLen);
void Flashx_ReadBytes(uint32_t u32Addr, uint8_t *pu8ReadBuff, uint32_t u32ByteLength);
en_result_t Flashx_EraseSector(uint32_t u32Addr);


#endif // __FLASHX_H__