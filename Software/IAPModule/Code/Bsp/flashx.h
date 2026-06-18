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
#define IAP_FILE_SIZE           (25 * FLASH_SECTOR_SIZE)
#define IAP_FILE_APP_SIZE       (24 * FLASH_SECTOR_SIZE)
#define IAP_FILE_INFO_SIZE      (1 * FLASH_SECTOR_SIZE)


/*  start + 0xC000 = end   Info start + 0x800 = Info End               
File1   start: 0x08080000  end: 0x0808C000  Info start: 0x0808C000  end: 0x0808C800
File2   start: 0x0808C800  end: 0x08098800  Info start: 0x08098800  end: 0x08099000
File3   start: 0x08099000  end: 0x080A5000  Info start: 0x080A5000  end: 0x080A5800
File4   start: 0x080A5800  end: 0x080B1800  Info start: 0x080B1800  end: 0x080B2000
File5   start: 0x080B2000  end: 0x080BE000  Info start: 0x080BE000  end: 0x080BE800
File6   start: 0x080BE800  end: 0x080CA800  Info start: 0x080CA800  end: 0x080CB000
File7   start: 0x080CB000  end: 0x080D7000  Info start: 0x080D7000  end: 0x080D7800
File8   start: 0x080D7800  end: 0x080E3800  Info start: 0x080E3800  end: 0x080E4000
*/


#define APP_UPGRADE             ((uint32_t)0x67890123)
#define APP_FLAG1               ((uint32_t)0x56781234)
#define APP_FLAG2               ((uint32_t)0x12345678)

extern uint8_t u8FileDownLoadFlag;

en_result_t Flashx_WriteBytes(uint32_t u32Addr, uint8_t* pu8WriteBuff, uint32_t u32ByteLen);
void Flashx_ReadBytes(uint32_t u32Addr, uint8_t *pu8ReadBuff, uint32_t u32ByteLength);
en_result_t Flashx_EraseSector(uint32_t u32Addr);
void AppToBoot(void);
error_status IAP_ReadFileInfo(uint32_t fileAddr, char *outFileName, uint16_t maxLen);

#endif // __FLASHX_H__