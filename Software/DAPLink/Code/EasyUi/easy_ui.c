#include <stdio.h>
#include "easy_ui.h"
#include "function.h"
#include "timerx.h"

EasyUIPage_t *pageHead = NULL, *pageTail = NULL;

uint8_t pageIndex[MAX_LAYER] = {0};
uint8_t itemIndex[MAX_LAYER] = {0};
uint8_t layer = 0;
uint8_t currentItemIndex = 0 ;
uint8_t transType = 0;

uint8_t opnForward, opnBackward;
uint8_t opnEnter, opnExit, opnUp, opnDown;

uint32_t u32EasyUITick;

bool functionIsRunning = false, listLoop = true;

// IAP Value
uint8_t u8TransEndFlag = 0;
uint8_t u8AnimationState = 0;
uint8_t u8AnimationStep = 0;
uint32_t u32AnimationTick = 0;

// message box
uint32_t u32MsgDSPTick = 0;
MsgBox_t msgBox;

// key press box
uint8_t u8KeyPressedHint = 0;
char keyPressTimeStr[20] = {0};
bool keyPressBoxDrawn = false;

// item pid
PID_Controller itemYAxisPID = {0.6f, 0.1f, 0.0f, 0.0f, 0.0f, 0.0f};
PID_Controller itemXAxisPID = {0.8f, 0.1f, 0.0f, 0.0f, 0.0f, 0.0f};

PID_Controller keyPressBoxPID = {0.2f, 0.03f, 0.0f, 0.0f, 0.0f, 0.0f};
PID_Controller msgBoxPID = {0.2f, 0.03f, 0.0f, 0.0f, 0.0f, 0.0f};

void EasyUIDrawIndicator(EasyUIPage_t *page, uint8_t index, uint8_t status);
void EasyUI_SetMsgBox(char *msg, uint16_t keepTime, uint8_t pidEn);
float EasyUI_PIDCalc(PID_Controller *pid, float target, float current);

uint8_t u8PicReDrawn;

/*!
 * @brief   Initialize EasyUI
 *
 */

/*!
 * @brief   Add item to page
 *
 * @param   page        EasyUI page struct
 * @param   item        EasyUI item struct
 * @param   _title      String of item title
 * @param   func        See EasyUIItem_e
 * @param   ...         ITEM_PAGE_DESCRIPTION: ignore this
 *                      ITEM_CALL_FUNCTION: fill with function
 *                      ITEM_JUMP_PAGE: fill with target page id
 *                      ITEM_CHECKBOX / ITEM_RADIO_BUTTON / ITEM_SWITCH: fill with bool value
 *                      ITEM_CHANGE_VALUE / ITEM_PROGRESS_BAR: fill with param that need to be changed and matched function
 *                      ITEM_MESSAGE: fill with message and matched function
 * @return  void
 *
 * @note    Do not modify
 *          ITEM_CHANGE_VALUE: the incoming param should always be paramType *,
 *          and cannot use casted variables(Don't know why)
 *          ITEM_PROGRESS_BAR: the incoming param should be 0 - 100
 *          If page type is PAGE_ICON, filled with icon array in the last variable
 */
void EasyUIAddItem(EasyUIPage_t *page, EasyUIItem_t *item, char *_title, int func, ...)
{
    *item->flag = false;
    item->flagDefault = false;
    *item->param = 0;
    item->paramDefault = 0;
    item->paramBackup = 0;
    item->pageId = 0;
    item->Event = NULL;

    va_list variableArg;
    va_start(variableArg, func);
    item->title = _title;
    item->funcType = func;
    switch (item->funcType)
    {
    case ITEM_JUMP_PAGE:
        item->pageId = va_arg(variableArg, int);
        break;
    case ITEM_CHECKBOX:
    case ITEM_RADIO_BUTTON:
    case ITEM_SWITCH:
        item->flag = va_arg(variableArg, bool *);
        item->flagDefault = *item->flag;
        break;
    case ITEM_PROGRESS_BAR:
    case ITEM_CHANGE_VALUE:
        item->param = va_arg(variableArg, paramType *);
        item->paramBackup = *item->param;
        item->paramDefault = *item->param;
        item->Event = va_arg(variableArg, void (*)(EasyUIItem_t * ));
        break;
    case ITEM_MESSAGE:
        item->msg = va_arg(variableArg, char *);
        item->Event = va_arg(variableArg, void (*)(EasyUIItem_t * ));
    default:
        break;
    }

    if (page->funcType == PAGE_ICON)
        item->icon = va_arg(variableArg, uint8_t *);

    va_end(variableArg);

    item->next = NULL;

    if (page->itemHead == NULL)
    {
        item->id = 0;
        page->itemHead = item;
        page->itemTail = item;
    } else
    {
        item->id = page->itemTail->id + 1;
        page->itemTail->next = item;
        page->itemTail = page->itemTail->next;
    }

    item->lineId = item->id;
    item->posForCal = 0;
    item->step = 0;
    item->position = 0;
}


/*!
 * @brief   Add page to UI
 *
 * @param   page    EasyUI page struct
 * @param   func    See EasyUIPage_e
 * @param   ...     PAGE_LIST: ignore this
 *                  PAGE_CUSTOM: fill with certain function
 * @return  void
 *
 * @note    Do not modify, the first page should always be the fist one to be added.
 */
void EasyUIAddPage(EasyUIPage_t *page, int func, ...)
{
    page->Event = NULL;

    va_list variableArg;
    va_start(variableArg, func);
    page->itemHead = NULL;
    page->itemTail = NULL;
    page->next = NULL;

    page->funcType = func;
    if (page->funcType == PAGE_CUSTOM)
        page->Event = va_arg(variableArg, void (*)(EasyUIPage_t * ));
    va_end(variableArg);

    if (pageHead == NULL)
    {
        page->id = 0;
        pageHead = page;
        pageTail = page;
    } else
    {
        page->id = pageTail->id + 1;
        pageTail->next = page;
        pageTail = pageTail->next;
    }
}


/*!
 * @brief   Blur transition animation
 *
 * @param   void
 * @return  void
 *
 * @note    Use before clearing the buffer
 *          Also use after all the initialization is done for better experience
 */
void EasyUITransitionAnim()
{
    u8AnimationState = 1;
    u8AnimationStep = 0;
    u32AnimationTick = 0;
}


/*!
 * @brief   Blur the background for other use
 *
 * @param   void
 * @return  void
 */
void EasyUIBackgroundBlur()
{
    u8AnimationState = 2;
    u8AnimationStep = 0;
    u32AnimationTick = 0;
}


/*!
 * @brief   ????????
 * @param   void
 * @return  bool    ??????
 */
bool EasyUI_HandleAnimation(void)
{
    if(u8AnimationState == 0){
        return true;
    }
    uint32_t delayTime = 0;
    if(u8AnimationState == 1){
        delayTime = TRANSITION_TIME / 4;
        switch (u8AnimationStep){
        case 0:
            for (int j = 1; j < SCREEN_HEIGHT + 1; j += 2) {
                for (int i = 0; i < SCREEN_WIDTH + 1; i += 2) {
                    EasyUIDrawDot(i, j, IPS096_backgroundColor);
                }
            }
            EasyUISendBuffer();
            u8AnimationStep = 1;
            u32AnimationTick = 0;
            break;
        case 1:
            if (u32AnimationTick >= delayTime) {
                for (int j = 1; j < SCREEN_HEIGHT + 1; j += 2) {
                    for (int i = 1; i < SCREEN_WIDTH + 1; i += 2) {
                        EasyUIDrawDot(i, j, IPS096_backgroundColor);
                    }
                }
                EasyUISendBuffer();
                u8AnimationStep = 2;
                u32AnimationTick = 0;
            }
            break;
        case 2:
            if (u32AnimationTick >= delayTime) {
                for (int j = 0; j < SCREEN_HEIGHT + 1; j += 2) {
                    for (int i = 1; i < SCREEN_WIDTH + 1; i += 2) {
                        EasyUIDrawDot(i, j, IPS096_backgroundColor);
                    }
                }
                EasyUISendBuffer();
                u8AnimationStep = 3;
                u32AnimationTick = 0;
            }
            break;
        case 3:
            if (u32AnimationTick >= delayTime) {
                for (int j = 1; j < SCREEN_HEIGHT + 1; j += 2) {
                    for (int i = 1; i < SCREEN_WIDTH + 1; i += 2) {
                        EasyUIDrawDot(i - 1, j - 1, IPS096_backgroundColor);
                    }
                }
                EasyUISendBuffer();
                u8AnimationStep = 4;
                u32AnimationTick = 0;
            }
            break; 
        case 4:
            if (u32AnimationTick >= delayTime) {
                u8AnimationState = 0;
                u8AnimationStep = 0;
                return true;
            }
            break;
        default:
            break;
        }
    }else if(u8AnimationState == 2){ // BackgroundBlur
        switch (u8AnimationStep) {
        case 0:
            for (int j = 1; j < SCREEN_HEIGHT + 1; j += 2) {
                for (int i = 0; i < SCREEN_WIDTH + 1; i += 2) {
                    EasyUIDrawDot(i, j, IPS096_backgroundColor);
                }
            }
            EasyUISendBuffer();
            u8AnimationStep = 1;
            u32AnimationTick = 0;
            break;
        case 1:
            if (u32AnimationTick >= delayTime) {
                for (int j = 1; j < SCREEN_HEIGHT + 1; j += 2) {
                    for (int i = 1; i < SCREEN_WIDTH + 1; i += 2) {
                        EasyUIDrawDot(i, j, IPS096_backgroundColor);
                    }
                }
                EasyUISendBuffer();
                u8AnimationStep = 2;
                u32AnimationTick = 0;
            }
            break;
        case 2:
            if (u32AnimationTick >= delayTime) {
                for (int j = 0; j < SCREEN_HEIGHT + 1; j += 2) {
                    for (int i = 1; i < SCREEN_WIDTH + 1; i += 2) {
                        EasyUIDrawDot(i, j, IPS096_backgroundColor);
                    }
                }
                EasyUISendBuffer();
                u8AnimationStep = 3;
                u32AnimationTick = 0;
            }
            break;
        case 3:
            if (u32AnimationTick >= delayTime) {
                u32AnimationTick = 0;
                u8AnimationState = 0;
                u8AnimationStep = 0;
                return true;
            }
            break;
        }
    }
    return false;
}


/*!
 * @brief   Draw progress bar
 *
 * @param   item    EasyUI item struct
 * @return  void
 *
 * @note    Internal call
 */
void EasyUIDrawProgressBar(EasyUIItem_t *item)
{
    static int16_t x, y;
    static uint16_t width, height;
    static uint8_t itemHeightOffset = (ITEM_HEIGHT - FONT_HEIGHT) / 2 + 1;
    static uint16_t barWidth;

    EasyUISetDrawColor(NORMAL);

    // Display information and draw box
    height = ITEM_HEIGHT * 2 + 2;
    if (strlen(item->title) + 1 > 12)
        width = (strlen(item->title) + 1) * FONT_WIDTH + 7;
    else
        width = 12 * FONT_WIDTH + 7;
    if (width < 2 * SCREEN_WIDTH / 3)
        width = 2 * SCREEN_WIDTH / 3;
    x = (SCREEN_WIDTH - width) / 2;
    y = (SCREEN_HEIGHT - height) / 2;

    barWidth = width - 6 * FONT_WIDTH - 10;

    EasyUIDrawFrame(x - 1, y - 1, width + 2, height + 2, IPS096_penColor);
    EasyUIDrawBox(x, y, width, height, IPS096_backgroundColor);
    EasyUIDisplayStr(x + 3, y + itemHeightOffset, item->title);
    EasyUIDisplayStr(x + 3 + strlen(item->title) * FONT_WIDTH, y + itemHeightOffset, ":");
    EasyUIDrawFrame(x + 3, y + ITEM_HEIGHT + itemHeightOffset, barWidth, FONT_HEIGHT, IPS096_penColor);
    EasyUIDrawBox(x + 5, y + ITEM_HEIGHT + itemHeightOffset + 2, (float) *item->param / 100 * barWidth - 4,
                  FONT_HEIGHT - 4, IPS096_penColor);
    EasyUIDisplayFloat(x + width - 6 * FONT_WIDTH - 4, y + ITEM_HEIGHT + itemHeightOffset, *item->param, 3, 2);

    EasyUISendBuffer();
}


/*!
 * @brief   Draw check box
 *
 * @param   x           Check box position x
 * @param   y           Check box position y
 * @param   size        Size of check box
 * @param   offset      Offset of selected rounded box
 * @param   boolValue   True of false
 * @return  void
 *
 * @note    Internal call
 */
void EasyUIDrawCheckbox(int16_t x, int16_t y, uint16_t size, uint8_t offset, bool boolValue, uint8_t r)
{
    EasyUIDrawRFrame(x, y, size, size, IPS096_penColor, r);
    if (boolValue)
        EasyUIDrawRBox(x + offset, y + offset, size - 2 * offset, size - 2 * offset, IPS096_penColor, r);
}


/*!
 * @brief   Get position of item with linear animation
 *
 * @param   page    Struct of page
 * @param   item    Struct of item
 * @param   index   Current index
 * @param   timer   Fill this with interrupt trigger time
 * @return  void
 *
 * @note    Internal call
 */
void EasyUIGetItemPos(EasyUIPage_t *page, EasyUIItem_t *item, uint8_t index, uint8_t timer)
{
    static uint8_t itemHeightOffset = (ITEM_HEIGHT - FONT_HEIGHT) / 2;
    static uint16_t time = 0;
    static int16_t move = 0, target = 0;
    static uint8_t lastIndex = 0, moveFlag = 0;
    uint8_t speed = ITEM_MOVE_TIME / timer;

    // Item need to move or not
    if (moveFlag == 0)
    {
        for (EasyUIItem_t *itemTmp = page->itemHead; itemTmp != NULL; itemTmp = itemTmp->next)
        {
            if (index == itemTmp->id && itemTmp->lineId < 0)
            {
                move = itemTmp->lineId;
                moveFlag = 1;
                break;
            } else if (index == itemTmp->id && itemTmp->lineId > ITEM_LINES - 1)
            {
                move = itemTmp->lineId - ITEM_LINES + 1;
                moveFlag = 1;
                break;
            }
        }
    }

    // Change the item lineId and get target position
    for (EasyUIItem_t *itemTmp = page->itemHead; itemTmp != NULL; itemTmp = itemTmp->next)
    {
        itemTmp->lineId -= move;
    }
    move = 0;
    moveFlag = 0;
    target = itemHeightOffset + item->lineId * ITEM_HEIGHT;

    // Calculate current position
    if (time == 0 || index != lastIndex)
    {
        item->step = ((float) target - (float) item->position) / (float) speed;
    }
    if (time >= ITEM_MOVE_TIME)
    {
        item->posForCal = target;
    } else
        item->posForCal += item->step;

    item->position = (int16_t) item->posForCal;
    lastIndex = index;

    // Time counter
    if (item->next == NULL)
    {
        if (target == item->position)
            time = 0;
        else
            time += timer;
    }
}


/*!
 * @brief   Display item according to its funcType
 * @param   item    Struct of item
 * @return  void
 *
 * @note    Internal call
 */
void EasyUIDisplayItem(EasyUIItem_t *item)
{
    switch (item->funcType)
    {
    case ITEM_JUMP_PAGE:
        EasyUIDisplayStr(2, item->position + 3, "#");
        EasyUIDisplayStr(5 + FONT_WIDTH, item->position + 3, item->title);
        break;
    case ITEM_PAGE_DESCRIPTION:
        EasyUIDisplayStr(2, item->position, item->title);
        break;
    case ITEM_CHECKBOX:
    case ITEM_RADIO_BUTTON:
        EasyUIDisplayStr(2, item->position, "-");
        EasyUIDisplayStr(5 + FONT_WIDTH, item->position, item->title);
//        EasyUIDrawCheckbox(SCREEN_WIDTH - 7 - SCROLL_BAR_WIDTH - ITEM_HEIGHT + 2,
//                           item->position - (ITEM_HEIGHT - FONT_HEIGHT) / 2 + 1, ITEM_HEIGHT - 2, CHECK_BOX_OFFSET,
//                           *item->flag, 1);
        break;
    case ITEM_SWITCH:
        EasyUIDisplayStr(2, item->position, "-");
        EasyUIDisplayStr(5 + FONT_WIDTH, item->position, item->title);
        if (*item->flag)
            EasyUIDisplayStr(SCREEN_WIDTH - 7 - 2 * FONT_WIDTH - SCROLL_BAR_WIDTH, item->position, "on");
        else
            EasyUIDisplayStr(SCREEN_WIDTH - 7 - 3 * FONT_WIDTH - SCROLL_BAR_WIDTH, item->position, "off");
        break;
    case ITEM_PROGRESS_BAR:
    case ITEM_CHANGE_VALUE:
        EasyUIDisplayStr(2, item->position, "-");
        EasyUIDisplayStr(5 + FONT_WIDTH, item->position, item->title);
        if (*item->param < 10 && *item->param >= 0)
            EasyUIDisplayFloat(SCREEN_WIDTH - 7 - 4 * FONT_WIDTH - SCROLL_BAR_WIDTH, item->position,
                               *item->param, 4, 2);
        else if (*item->param < 100 && *item->param > -10)
            EasyUIDisplayFloat(SCREEN_WIDTH - 7 - 5 * FONT_WIDTH - SCROLL_BAR_WIDTH, item->position,
                               *item->param, 4, 2);
        else if (*item->param < 1000 && *item->param > -100)
            EasyUIDisplayFloat(SCREEN_WIDTH - 7 - 6 * FONT_WIDTH - SCROLL_BAR_WIDTH, item->position,
                               *item->param, 4, 2);
        else if (*item->param < 10000 && *item->param > -1000)
            EasyUIDisplayFloat(SCREEN_WIDTH - 7 - 7 * FONT_WIDTH - SCROLL_BAR_WIDTH, item->position,
                               *item->param, 4, 2);
        else    // Hide because it's too long
            EasyUIDisplayStr(SCREEN_WIDTH - 7 - 5 * FONT_WIDTH - SCROLL_BAR_WIDTH, item->position, "**.**");
        break;
    default:
        EasyUIDisplayStr(2, item->position + 3, "-");
        EasyUIDisplayStr(5 + FONT_WIDTH, item->position + 3, item->title);
        break;
    }
}


/*!
 * @brief   Get position of indicator and scroll bar with linear animation
 *
 * @param   page    Struct of page
 * @param   index   Current index
 * @param   timer   Fill this with interrupt trigger time
 * @param   status  Fill this with 1 to reset height
 * @return  void
 *
 * @note    Internal call
 */
void EasyUIDrawIndicator(EasyUIPage_t *page, uint8_t index, uint8_t status)
{
    static float stepLength = 0, stepY = 0, length = 0, y = SCREEN_HEIGHT;
    static uint8_t lastIndex = 0;
    static uint16_t lengthTarget = 0, yTarget = 0;

    if (status)
        y = SCREEN_HEIGHT;

    if (page->funcType != PAGE_LIST)
        return;

    // Get Initial length
    if ((int) length == 0)
    {
        if (page->itemHead->funcType == ITEM_PAGE_DESCRIPTION)
            length = (float) (strlen(page->itemHead->title)) * FONT_WIDTH + 5;
        else
            length = (float) (strlen(page->itemHead->title) + 1) * FONT_WIDTH + 8;
    }

    // Get target length and y
    for (EasyUIItem_t *itemTmp = page->itemHead; itemTmp != NULL; itemTmp = itemTmp->next)
    {
        if (index == itemTmp->id)
        {
            if (itemTmp->funcType == ITEM_PAGE_DESCRIPTION)
                lengthTarget = (strlen(itemTmp->title)) * FONT_WIDTH;
            else
                lengthTarget = (strlen(itemTmp->title) + 1) * FONT_WIDTH;
            yTarget = itemTmp->lineId * ITEM_HEIGHT;
            if (index != lastIndex && func_abs(index - lastIndex) < page->itemTail->id)
            {
                if (itemTmp->position < 0)
                    y = (float) 3 * ITEM_HEIGHT / 4;
                else if (itemTmp->position >= (ITEM_LINES) * ITEM_HEIGHT)
                    y = (ITEM_LINES - 2) * ITEM_HEIGHT + (float) ITEM_HEIGHT / 4;
            }
            break;
        }
    }

    length += EasyUI_PIDCalc(&itemXAxisPID, (float)lengthTarget, (float)length);
    y += EasyUI_PIDCalc(&itemYAxisPID, (float)yTarget, (float)y);

    // Draw rounded box and scroll bar
    EasyUISetDrawColor(XOR);
    EasyUIDrawRBox(0, (int16_t) y, (int16_t) length, ITEM_HEIGHT, IPS096_penColor, 1);
    EasyUISetDrawColor(NORMAL);
    EasyUIDrawRBox(SCREEN_WIDTH - SCROLL_BAR_WIDTH, (int16_t) y, SCROLL_BAR_WIDTH, ITEM_HEIGHT, IPS096_penColor, 1);
    lastIndex = index;
    // Draw dot area
    int16_t dotStartX = (int16_t)length;
    int16_t dotStartY = y;
    for (uint8_t row = 0; row < ITEM_HEIGHT; row++) {
        for (uint8_t col = 0; col < 6; col++) {
            if ((row + col) % 2 == 0) {
                EasyUIDrawDot(dotStartX + col, dotStartY + row, IPS096_penColor);
            }
        }
    }

}


/*!
 * @brief   Different response to operation according to funcType
 *
 * @param   page    Struct of page
 * @param   item    Struct of item
 * @param   index   Current index
 * @return  void
 *
 * @note    Internal call
 */
void EasyUIItemOperationResponse(EasyUIPage_t *page, EasyUIItem_t *item, uint8_t *index)
{
    switch (item->funcType)
    {
    case ITEM_JUMP_PAGE:
        if (layer == MAX_LAYER - 1)
            break;

        itemIndex[layer++] = *index;
        if(layer == 2){
            layer = 0;
        }
        pageIndex[layer] = item->pageId;
        *index = 0;
        for (EasyUIItem_t *itemTmp = page->itemHead; itemTmp != NULL; itemTmp = itemTmp->next)
        {
            if (itemTmp->lineId < 0)
                continue;

            itemTmp->position = 0;
            itemTmp->posForCal = 0;
        }
        EasyUITransitionAnim();
        break;
    case ITEM_CHECKBOX:
    case ITEM_SWITCH:
        *item->flag = !*item->flag;
        break;
    case ITEM_RADIO_BUTTON:
        for (EasyUIItem_t *itemTmp = page->itemHead; itemTmp != NULL; itemTmp = itemTmp->next)
        {
            if (itemTmp->funcType == ITEM_RADIO_BUTTON && itemTmp->id != item->id)
                *itemTmp->flag = false;
        }
        *item->flag = !*item->flag;
        break;
    case ITEM_PROGRESS_BAR:
    case ITEM_CHANGE_VALUE:
        functionIsRunning = true;
        EasyUIBackgroundBlur();
        break;
    case ITEM_MESSAGE:
        functionIsRunning = true;
        EasyUI_SetMsgBox(item->msg, 0, 0);
        break;
    default:
        break;
    }
}

void EasyUIEventIAPFinished(EasyUIItem_t *item){
    if(u8TransEndFlag == 0){
        return;
    }
//    switch (u8TransEndFlag){
//        case IAP_TRANS_SUCCESS:
//            item->msg = "Download Success!";
//            break;
//        case IAP_TRANS_FAILED:
//            item->msg = "Download Fail!";
//            break;
//        case MODEM_TRANS_SUCCESS:
//            item->msg = "Update Success!";
//            break;
//        case MODEM_TRANS_FAILED:
//            item->msg = "Update Fail!";
//            break;
//        default:
//            break;
//    }
    EasyUI_SetMsgBox(item->msg, 500, 0);
    functionIsRunning = 0;
    u8TransEndFlag = 0;
}

/*!
 * @brief   Welcome Page with two size of photo, and read params from flash if not empty
 *
 * @param   mode    choose the size of photo (0 for smaller one and 1 for bigger one)
 * @return  void
 */
void EasyUIInit(uint8_t mode)
{
    EasyUIScreenInit();

    // Display the welcome photo and info
    EasyUIModifyColor();
    EasyUIClearBuffer();
    EasyUIDisplayBMP((SCREEN_WIDTH - 140) / 2, (SCREEN_HEIGHT - 41) / 2, 140, 41, DAPLink_logo);
    EasyUIDisplayStr(SCREEN_WIDTH - 1 - 14 * FONT_WIDTH, SCREEN_HEIGHT - 1 - FONT_HEIGHT, "Designed(YU XUAN)");

    EasyUISendBuffer();

    u32EasyUITick = 0;
}


/*!
 * @brief   Sync the operation bool value
 *
 * @param   void
 * @return  void
 */
void EasyUIKeyActionMonitor(void)
{
    if (opnForward || opnBackward || opnEnter || opnExit || opnUp || opnDown)
        return;

    if(KeyFlag_Read(&KEY_MOD) == 1){
        KeyFlag_Clear(&KEY_MOD);
        opnForward = true;
    }else{
        if(KEY_MOD.u16Delay > 1000){
            u8KeyPressedHint = 1;
        }else{
            u8KeyPressedHint = 0;
            keyPressBoxDrawn = false;
        }
    }

#if ROTARY == 1
#endif
}


/*!
 * @brief   Main function of EasyUI
 *
 * @param   timer   Fill this with interrupt trigger time
 * @return  void
 */
void EasyUI(uint8_t timer)
{
    static uint8_t itemSum = 0;

    EasyUIModifyColor();
    EasyUISetDrawColor(NORMAL);

//    // Get current page by id
//    EasyUIPage_t *page = pageHead;
//    while (page->id != pageIndex[layer])
//    {
//        page = page->next;
//    }
    
    if(u8KeyPressedHint){
        EasyUI_DrawKeyPressTimeBox(KEY_MOD.u16Delay);
        u8PicReDrawn = 1;
        return;
    }else{
        keyPressBoxDrawn = 0;
    }
    
    if(u8PicReDrawn){
        EasyUIModifyColor();
        EasyUIClearBuffer();
        EasyUIDisplayBMP((SCREEN_WIDTH - 140) / 2, (SCREEN_HEIGHT - 41) / 2, 140, 41, DAPLink_logo);
        EasyUIDisplayStr(SCREEN_WIDTH - 1 - 14 * FONT_WIDTH, SCREEN_HEIGHT - 1 - FONT_HEIGHT, "Designed(YU XUAN)");

        EasyUISendBuffer();
        u8PicReDrawn = 0;
    }


}


void EasyUI_Func(void){
    if(!EasyUI_HandleAnimation()){
        return;
    }
    EasyUIKeyActionMonitor();

    if(u32EasyUITick > 20){
        u32EasyUITick = 0;
        EasyUI(20);
    }
}


/*!
 * @brief   Tick function for EasyUI
 * @param   void
 * @return  void
 */
void EasyUI_Tick(void){
    u32EasyUITick++;
    if (u8AnimationState != 0) {
        u32AnimationTick++;
    }
    u32MsgDSPTick++;
}


/*!
 * @brief   ???????
 * @param   remainTime  ????(ms)
 * @return  void
 */
void EasyUI_DrawKeyPressTimeBox(uint32_t pressTime){
    uint8_t pressSec = pressTime / 1000;
    sprintf(keyPressTimeStr, "KEY PRESSED: %d S", pressSec);
    uint16_t width = strlen(keyPressTimeStr) * FONT_WIDTH + 10;
    uint16_t x = (SCREEN_WIDTH - width) / 2;
    uint8_t offset = 2;
    
    static uint16_t currentHeight = 0;

    if(keyPressBoxDrawn == false){
        currentHeight = 1; // ????????
        EasyUIBackgroundBlur();
        keyPressBoxDrawn = true;
    }else{
        currentHeight += EasyUI_PIDCalc(&keyPressBoxPID, (float)ITEM_HEIGHT, (float)currentHeight);
        if(currentHeight > ITEM_HEIGHT) currentHeight = ITEM_HEIGHT;
        
        uint16_t height = (currentHeight - FONT_HEIGHT) > 0 ? 0 : (currentHeight - FONT_HEIGHT);

        EasyUIDrawRBox(x, 0, width, currentHeight, IPS096_penColor, 1);
        EasyUISetDrawColor(XOR);
        EasyUIDisplayStr(x + 20, + (height) / 2 + 4, keyPressTimeStr);
        EasyUISetDrawColor(NORMAL);

        EasyUISendBuffer();
    }
}

/*!
 * @brief   Draw message box
 *
 * @param   msg     The message need to be displayed
 * @return  void
 */
void EasyUI_DrawMsgBox(void)
{
    uint16_t targetY = 0;
    switch (msgBox.msgDrawn){
    case 0:
        EasyUIBackgroundBlur();
        msgBox.msgDrawn = 1;
        msgBox.width = strlen(msgBox.text) * FONT_WIDTH + 5;
        msgBox.offset = 2;
        msgBox.x = (SCREEN_WIDTH - msgBox.width) / 2;
        msgBox.y = 0;
        break;
    
    case 1:
        targetY = (SCREEN_HEIGHT - ITEM_HEIGHT) / 2;
        if(msgBox.pidEn){
            msgBox.y += EasyUI_PIDCalc(&msgBoxPID, (float)targetY, (float)msgBox.y);
        }else{
            msgBox.y = targetY;
        }
        EasyUIDrawRBox(msgBox.x + msgBox.offset, msgBox.y- msgBox.offset, msgBox.width, ITEM_HEIGHT, IPS096_backgroundColor, 1);
        EasyUIDrawRFrame(msgBox.x + msgBox.offset, msgBox.y - msgBox.offset, msgBox.width, ITEM_HEIGHT, IPS096_penColor, 1);
        EasyUIDrawRBox(msgBox.x - msgBox.offset, msgBox.y + msgBox.offset, msgBox.width, ITEM_HEIGHT, IPS096_penColor, 1);
        EasyUISetDrawColor(XOR);
        EasyUIDisplayStr(msgBox.x - msgBox.offset + 2, msgBox.y + msgBox.offset + (ITEM_HEIGHT - FONT_HEIGHT) / 2, msgBox.text);
        EasyUISetDrawColor(NORMAL);
        EasyUISendBuffer();
        if(msgBox.y == (SCREEN_HEIGHT - ITEM_HEIGHT) / 2){
            msgBox.msgDrawn = 2;
            u32MsgDSPTick = 0;
        }
        break;
    case 2:
        if(u32MsgDSPTick > msgBox.dspTime){
            msgBox.msgDrawn = 0;
            msgBox.text = NULL;
            msgBox.width = 0;
            msgBox.offset = 0;
            msgBox.x = 0;
        }
    default:
        break;
    }
}

/*!
 * @brief   Set message box
 * @return  void
 */
void EasyUI_SetMsgBox(char *msg, uint16_t keepTime, uint8_t pidEn){
    memset(&msgBoxPID, 0, sizeof(msgBoxPID));
    memset(&msgBox, 0, sizeof(msgBox));
    msgBox.text = msg;
    msgBox.dspTime = keepTime;
    msgBox.pidEn = pidEn;
}



float EasyUI_PIDCalc(PID_Controller *pid, float target, float current){
    pid->error = target - current;
    pid->integral += pid->error;
    if (pid->integral > 100.0f) pid->integral = 100.0f;
    if (pid->integral < -100.0f) pid->integral = -100.0f;
    float pidOutput = pid->kp * pid->error + pid->ki * pid->integral + 
    pid->kd * (pid->error - pid->lastError);
    pid->lastError = pid->error;
    return pidOutput;
}