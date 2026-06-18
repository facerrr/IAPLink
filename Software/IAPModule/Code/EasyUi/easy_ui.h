#ifndef __EASY_UI_H__
#define __EASY_UI_H__

#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include "profile_photo_erbws.h"
#include "timerx.h"
#include "ipsCommon.h"
#include "key.h"
#include "base_types.h"

#define ROTARY          0

#define FONT_WIDTH              8
#define FONT_HEIGHT             12
#define ITEM_HEIGHT             16
#define CHECK_BOX_OFFSET        2
#define SCROLL_BAR_WIDTH        4
#define ITEM_LINES              ((uint8_t)(SCREEN_HEIGHT / ITEM_HEIGHT))
#define MAX_LAYER               10
#define ICON_SIZE               50

#define INDICATOR_MOVE_TIME     140
#define ITEM_MOVE_TIME          140
#define TRANSITION_TIME         120

#define EasyUIScreenInit()                                      (IPS_Init())
#define EasyUIDisplayStr(x, y, str)                             (IPS_ShowStr(x, y, str))
#define EasyUIDrawDot(x, y, color)                              (IPS_DrawPoint(x, y, color))
#define EasyUIDrawBox(x, y, width, height, color)               (IPS_DrawBox(x, y, width, height, color))
#define EasyUIDrawFrame(x, y, width, height, color)             (IPS_DrawFrame(x, y, width, height, color))
#define EasyUIDrawRFrame(x, y, width, height, color, r)         (IPS_DrawRFrame(x, y, width, height, color, r))
#define EasyUIDrawRBox(x, y, width, height, color, r)           (IPS_DrawRBox(x, y, width, height, color, r))
#define EasyUISendBuffer()                                      (IPS_SendBuffer())
#define EasyUISetDrawColor(mode)                                (IPS_SetDrawColor(mode))
#define EasyUIDisplayBMP(x, y, width, height, pic)              (IPS_ShowBMP(x, y, width, height, pic))
#define EasyUIModifyColor()                                     (IPS_ModifyColor())

#define EasyUIDelay_ms(time)                                    (delay_ms(time))

typedef double paramType;

typedef enum
{
    ITEM_PAGE_DESCRIPTION,
    ITEM_JUMP_PAGE,
    ITEM_SWITCH,
    ITEM_CHANGE_VALUE,
    ITEM_PROGRESS_BAR,
    ITEM_RADIO_BUTTON,
    ITEM_CHECKBOX,
    ITEM_MESSAGE
} EasyUIItem_e;


typedef enum
{
    PAGE_LIST,
    PAGE_ICON,
    PAGE_CUSTOM
} EasyUIPage_e;


typedef struct EasyUI_item
{
    struct EasyUI_item *next;

    EasyUIItem_e funcType;
    uint8_t id;
    int16_t lineId;
    float posForCal;
    float step;
    int16_t position;
    char *title;

    uint8_t *icon;                              // PAGE_ICON
    char *msg;                                  // ITEM_MESSAGE
    bool *flag;                                 // ITEM_CHECKBOX and ITEM_RADIO_BUTTON and ITEM_SWITCH
    bool flagDefault;                           // Factory default setting
    paramType *param;                           // ITEM_CHANGE_VALUE and ITEM_PROGRESS_BAR
    paramType paramDefault;                     // Factory default setting
    paramType paramBackup;                      // ITEM_CHANGE_VALUE and ITEM_PROGRESS_BAR
    uint8_t pageId;                             // ITEM_JUMP_PAGE
    void (*Event)(struct EasyUI_item *item);    // ITEM_CHANGE_VALUE and ITEM_PROGRESS_BAR
} EasyUIItem_t;


typedef struct EasyUI_page
{
    struct EasyUI_page *next;

    EasyUIPage_e funcType;
    EasyUIItem_t *itemHead, *itemTail;
    uint8_t id;

    void (*Event)(struct EasyUI_page *page);
} EasyUIPage_t;

typedef struct {
    float kp;        // 比例系数
    float ki;        // 积分系数
    float kd;        // 微分系数
    float error;     // 当前误差
    float lastError; // 上一次误差
    float integral;  // 积分项
} PID_Controller;


typedef struct {
    char* text;
    uint16_t x;
    uint16_t y;
    uint16_t width;
    uint16_t offset;
    uint8_t msgDrawn;
    uint16_t dspTime;
    uint8_t pidEn;
    uint8_t blurEn;
} MsgBox_t;


void EasyUIAddItem(EasyUIPage_t *page, EasyUIItem_t *item, char *_title, int func, ...);
void EasyUIAddPage(EasyUIPage_t *page, int func, ...);
void EasyUITransitionAnim(void);
void EasyUIBackgroundBlur(void);
void EasyUIKeyActionMonitor(void);
void EasyUIEventIAPFinished(EasyUIItem_t *item);

void EasyUI(uint8_t timer);
void EasyUI_Func(void);
void EasyUI_Tick(void);
void EasyUI_DrawMsgBox(void);

void EasyUI_Init(void);
uint8_t EasyUI_FileIndexGet(uint8_t mode);
void EasyUI_FileReInit(uint8_t* filename, uint16_t len);
void EasyUI_TransEnd(uint8_t transResult);
bool EasyUI_HandleAnimation(void);
void EasyUI_DrawKeyPressTimeBox(uint32_t pressTime);
void EasyUI_ClearBuffer(void);
void EasyUI_MsgScrolling(void);

#endif // __EASY_UI_H__

