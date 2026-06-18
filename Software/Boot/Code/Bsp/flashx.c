#include "main.h"

uint8_t flash_buf[SECTOR_SIZE];
error_status flash_write_nocheck(uint32_t u32Addr, uint8_t* pu8WriteBuff, uint16_t num_write);


/**
 * @brief  Flash字节写
 * @param  u32Addr: 写入地址 
 * @param  pu8WriteBuff: 写入数据缓冲区
 * @param  u32ByteLen: 写入字节数
 * @retval 写入状态
 */
en_result_t Flashx_WriteBytes(uint32_t u32Addr, uint8_t* pu8WriteBuff, uint32_t u32ByteLen){
    uint32_t i; 
    flash_unlock();
    for (i=0; i < u32ByteLen; i++){
        flash_flag_clear(FLASH_BANK2_ODF_FLAG | FLASH_BANK2_EPPERR_FLAG | FLASH_BANK2_PRGMERR_FLAG);
        if(FLASH_OPERATE_DONE != flash_byte_program(u32Addr + i, *(pu8WriteBuff + i))){
            flash_lock();
            return Error;
        }
    }
    flash_lock();
    return Ok;
}

uint8_t Flash_Write(uint32_t u32Addr, uint8_t* pu8WriteBuff, uint32_t u32ByteLen){
    uint32_t offsetAddr;
    uint32_t sector_position, sector_offset, sector_remain;
    uint16_t i;
    flash_status_type status = FLASH_OPERATE_DONE;
    flash_unlock();

    offsetAddr = u32Addr - FLASH_BASE;
    sector_position = offsetAddr / SECTOR_SIZE;
    sector_offset = offsetAddr % SECTOR_SIZE;
    sector_remain = SECTOR_SIZE - sector_offset;
    if(u32ByteLen <= sector_remain){
        sector_remain = u32ByteLen;
    }
    while (1){
        Flashx_ReadBytes(sector_position * SECTOR_SIZE + FLASH_BASE, flash_buf, SECTOR_SIZE);
        for(i = 0; i < sector_remain; i++){
            if(flash_buf[sector_offset + i] != 0xFF){
                break;
            }
        }
        if(i < sector_remain){
            status = flash_operation_wait_for(ERASE_TIMEOUT);
            flash_flag_clear(FLASH_PRGMERR_FLAG | FLASH_EPPERR_FLAG);
            if(status == FLASH_OPERATE_TIMEOUT){
                return 0;
            } 
            status = flash_sector_erase(sector_position * SECTOR_SIZE + FLASH_BASE);
            if(status != FLASH_OPERATE_DONE){
                return 0;
            }
            for(i = 0; i < sector_remain; i++){
                flash_buf[i + sector_offset] = pu8WriteBuff[i];
            }
            if(flash_write_nocheck(sector_position * SECTOR_SIZE + FLASH_BASE, flash_buf, SECTOR_SIZE / 2) != SUCCESS){
                return 0;
            }
        }else{
            if(flash_write_nocheck(u32Addr, pu8WriteBuff, sector_remain) != SUCCESS){
                return 0;
            }
        }
        if(u32ByteLen == sector_remain){
            break;
        }else{
            sector_position++;
            sector_offset = 0;
            pu8WriteBuff += sector_remain;
            u32Addr += (sector_remain * 2);
            u32ByteLen -= sector_remain;
            if(u32ByteLen > (SECTOR_SIZE / 2)){
                sector_remain = SECTOR_SIZE / 2;
            }else{
                sector_remain = u32ByteLen;
            }
        }
    }
    flash_lock();
    return 1;
}



/**
 * @brief  Flash字节读
 * @param  u32Addr: 读取地址 
 * @param  pu8ReadBuff: 读取数据缓冲区
 * @param  u32ByteLength: 读取字节数
 * @retval 无
 */
void Flashx_ReadBytes(uint32_t u32Addr, uint8_t *pu8ReadBuff, uint32_t u32ByteLength){
    uint16_t i;
    for(i = 0; i < u32ByteLength; i++){
        pu8ReadBuff[i] = *(uint8_t *)(u32Addr + i);
    }
}



/**
 * @brief  Flash扇区擦除
 * @param  u32Addr: 擦除地址 
 * @retval 擦除状态
 */
en_result_t Flashx_EraseSector(uint32_t u32Addr){
    flash_status_type status = FLASH_OPERATE_DONE;
    flash_unlock();
    status = flash_operation_wait_for(ERASE_TIMEOUT);
    flash_flag_clear(FLASH_PRGMERR_FLAG | FLASH_EPPERR_FLAG);
    if(status == FLASH_OPERATE_TIMEOUT){
        flash_lock();
        return Error;
    }
    status = flash_sector_erase(u32Addr);
    if(status != FLASH_OPERATE_DONE){
        flash_lock();
        return Error;
    }
    flash_lock();
    return Ok;
}


/**
 * @brief  Flash字节写
 * @param  u32Addr: 写入地址 
 * @param  u8WriteData: 写入数据
 * @retval 写入状态
 */
error_status flash_write_nocheck(uint32_t u32Addr, uint8_t* pu8WriteBuff, uint16_t num_write){
    uint16_t i;
    flash_status_type status = FLASH_OPERATE_DONE;
    for(i = 0; i < num_write; i++){
        status = flash_byte_program(u32Addr, pu8WriteBuff[i]);
        if(status != FLASH_OPERATE_DONE){
            return ERROR;
        }
        u32Addr++;
    }
    return SUCCESS;
}