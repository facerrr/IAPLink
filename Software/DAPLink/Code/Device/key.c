#include "config.h"

unsigned char u8KeyShortTick;
KeyStruct KeyS[KEY_NUM];


/**
 * @brief  按键初始化
 * @param  None
 * @retval None
 */
void KeyValue_Init(void){
    u8KeyShortTick = 10;
	memset(&KeyS, 0, sizeof(KeyStruct) * KEY_NUM);
}


/**
 * @brief  按键延时增加
 * @param  None
 * @retval None
 */
void KeyDelay_ADD(void){
    unsigned char i;
    for(i = 0; i < KEY_NUM; i++){
        KeyS[i].u16Delay++;
    }
}


/**
 * @brief  按键状态清除
 * @param  keyX: 按键结构体
 * @retval None
 */
void KeyFlag_Clear(KeyStruct *keyX){
    keyX->u8Flag = 0;
}


/**
 * @brief  按键状态读取
 * @param  keyX: 按键结构体
 * @retval 按键状态
 */
unsigned char KeyFlag_Read(KeyStruct *keyX){
    return keyX->u8Flag;
}

unsigned char KeyHold_Read(KeyStruct *keyX){
    return keyX->u8Pressing;
}


/**
 * @brief  短按识别
 * @param  keyX: 按键结构体
 * @retval 按键状态
 */
void KeyShortPress_CHK(KeyStruct *keyX){
    if(keyX->u8Pressing == 1){
        if(keyX->u16Delay >= KEY_SHORT_TIM){
            keyX->u16Delay = KEY_SHORT_TIM;
            if(keyX->u8Keep != 1){
                keyX->u8Keep = 1;
                keyX->u8Flag = 1;
            }
        }
    }else{
        keyX->u8Flag = 0;
        keyX->u8Keep = 0;
        keyX->u16Delay = 0;
    }
}

/**
 * @brief  长短按识别
 * @param  keyX: 按键结构体
 * @retval 按键状态
 */
void KeyLongPress_CHK(KeyStruct *keyX){
    if(keyX->u8Pressing){
        if(keyX->u16Delay >= KEY_LONG_TIM){
            keyX->u16Delay = KEY_LONG_TIM;
            if(keyX->u8Keep != 1){
                keyX->u8Keep = 1;
                keyX->u8Flag = 2;
            }
        }
    }else{
        if(keyX->u16Delay <= u8KeyShortTick){
            keyX->u8Flag = 0;
        }else if(keyX->u16Delay < KEY_SHORT_END){
            keyX->u8Flag = 1;
        }

        keyX->u8Keep = 0;
        keyX->u16Delay = 0;
    }
}


/**
 * @brief  按键扫描
 * @param  keyX: 按键结构体
 * @retval 按键状态
 */
void KeyFunc(void){
    if(KEY0_PUSH){
        KeyS[0].u8Pressing = 1;
    } else{
        KeyS[0].u8Pressing = 0;
    }
    if(KEY1_PUSH){
        KeyS[1].u8Pressing = 1;
    } else{
        KeyS[1].u8Pressing = 0;
    }

    KeyLongPress_CHK(&KEY_MOD);
    KeyLongPress_CHK(&KEY_DOWNLOAD);

    if(u8KeyShortTick != KEY_SHORT_TIM){
        if(KEY_MOD.u8Flag != 0){
            u8KeyShortTick = KEY_SHORT_TIM;
            KEY_MOD.u16Delay = KEY_LONG_TIM;
        }
        if(KEY_DOWNLOAD.u8Flag != 0){
            u8KeyShortTick = KEY_SHORT_TIM;
            KEY_DOWNLOAD.u16Delay = KEY_LONG_TIM;
        }
    }
}


