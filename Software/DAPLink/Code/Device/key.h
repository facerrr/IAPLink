#ifndef __KEY_H__
#define __KEY_H__

#define KEY_NUM 2

#define KEY0_PUSH           ((ADC_GetAvgValue(0) < 20))
#define KEY1_PUSH           ((ADC_GetAvgValue(1) < 20))

// 按键短按触发时间
#define KEY_SHORT_TIM       TIME1MS_(50)
// 按键短按结束时间
#define KEY_SHORT_END       TIME1MS_(600)
// 按键长按触发时间
#define KEY_LONG_TIM        TIME1MS_(5000)

// 按键结构体
typedef struct{
    unsigned char u8Pressing;
    unsigned char u8Flag;
    unsigned char u8Keep;
    unsigned short int u16Delay;
}KeyStruct;

extern KeyStruct KeyS[KEY_NUM];


#define KEY_STRUCT_SIZE     sizeof(KeyStruct)

#define KEY_MOD             KeyS[0]
#define KEY_DOWNLOAD        KeyS[1]

void KeyValue_Init(void);
void KeyDelay_ADD(void);
unsigned char KeyFlag_Read(KeyStruct *keyX);
unsigned char KeyHold_Read(KeyStruct *keyX);
void KeyFlag_Clear(KeyStruct *keyX);
void KeyFunc(void);


#endif // __KEY_H__