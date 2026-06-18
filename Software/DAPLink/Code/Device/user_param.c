#include "user_param.h"

user_param_t user_param;

void user_param_reset(void)
{
    flash_ee_data_write(PARAM_DAP_ID_ADDR, 0x0001);
    // Inited
    flash_ee_data_write(PARAM_INITED_ADDR, 0x0001);
}

void user_param_init(void)
{
    flash_ee_init();
    flash_ee_data_read(PARAM_INITED_ADDR, &user_param.inited);
    // 恢复出厂
    if(!user_param.inited)
    {
        user_param_reset();
    }
    
    flash_ee_data_read(PARAM_DAP_ID_ADDR, &user_param.dap_id);
    user_param.major = 1;
    user_param.minor = 2;
    user_param.patch = 0;
}
