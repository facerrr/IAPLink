#include "main.h"
#include "base_types.h"

uint32_t JumpAddress;
func_ptr_t Jump_To_Application;
uint32_t temp;
static en_result_t IAP_JumpToApp(uint32_t u32Addr);
static void IAP_ResetConfig(void);


void IAP_Init(void){
    // PreiModule_Init();
    Modem_RamInit();
}

void IAP_Main(void){
    en_result_t enRet;  
    while (1){
        enRet = Modem_Func();
        if(Ok == enRet){
            IAP_ResetConfig();
            if(Error == IAP_JumpToApp(FLASH_IAP_ADDR_ST)){
                while(1);
            }
        }
    }
}


void IAP_UpdateCheck(void){
    uint32_t u32AppFlag;
    uint32_t u32AppAddr;
    uint32_t u32UpgradeFlag;
    u32UpgradeFlag = *(__IO uint32_t *)(FLASH_INFO_ADDR_ST);
    u32AppFlag = *(__IO uint32_t *)(FLASH_INFO_ADDR_ST + 4);
    
    if(u32UpgradeFlag != APP_UPGRADE){
        if(u32AppFlag == APP_FLAG1){
            u32AppAddr = FLASH_IAP_ADDR_ST;
        }else{
            u32AppAddr = FLASH_DAP_ADDR_ST;
        }
        IAP_JumpToApp(u32AppAddr);
    }
}


static en_result_t IAP_JumpToApp(uint32_t u32Addr){
//    App_Clear_Sys_Status();
    temp = *((__IO uint32_t *)(u32Addr));
    uint32_t u32StackTop = (temp & 0x2FFE0000);
    if(u32StackTop == 0x20000000){
        JumpAddress = *((__IO uint32_t *)(u32Addr + 4));
        Jump_To_Application = (void (*)(void))JumpAddress;
        __set_MSP(*(__IO uint32_t*)u32Addr);
        Jump_To_Application();
    }
    return Error;
}

static void IAP_ResetConfig(void)
{
    App_Clear_Sys_Status();
//    PreiModule_DeInit();
}