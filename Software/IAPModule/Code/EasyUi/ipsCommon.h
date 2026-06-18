#ifndef __IPS_COMMON_H__
#define __IPS_COMMON_H__

#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#define SCREEN_WIDTH            160
#define SCREEN_HEIGHT           80
#define IPS_BUFFER_LENGTH       25600


#define IPS_SPI                      (SPI3)

#define IPS_SCK_PORT                 (GPIOB)
#define IPS_SCK_PIN                  (GPIO_PINS_3)
#define IPS_SDA_PORT                 (GPIOB)
#define IPS_SDA_PIN                  (GPIO_PINS_5)

#define IPS_RST_PORT                 (GPIOB)
#define IPS_RST_PIN                  (GPIO_PINS_4)
#define IPS_DC_PORT                  (GPIOB)
#define IPS_DC_PIN                   (GPIO_PINS_6)
#define IPS_CS_PORT                  (GPIOA)
#define IPS_CS_PIN                   (GPIO_PINS_15)
#define IPS_BLK_PORT                 (GPIOB)
#define IPS_BLK_PIN                  (GPIO_PINS_7)


#define IPS_DEFAULT_DISPLAY_DIR      (IPS_CROSSWISE_180)                  // 默认的显示方向
#define IPS_DEFAULT_PENCOLOR         (RGB565_WHITE)                          // 默认的画笔颜色
#define IPS_DEFAULT_BGCOLOR          (RGB565_BLACK)                          // 默认的背景颜色
#define IPS_DEFAULT_DISPLAY_FONT     (IPS_6X8_FONT)                       // 默认的字体模式

#define IPS_DC(x)                    ((x) ? (IPS_DC_PORT->scr = IPS_DC_PIN) : (IPS_DC_PORT->clr = IPS_DC_PIN))
#define IPS_RST(x)                   ((x) ? (IPS_RST_PORT->scr = IPS_RST_PIN) : (IPS_RST_PORT->clr = IPS_RST_PIN))
#define IPS_CS(x)                    ((x) ? (IPS_CS_PORT->scr = IPS_CS_PIN) : (IPS_CS_PORT->clr = IPS_CS_PIN))
#define IPS_BLK(x)                   ((x) ? (IPS_BLK_PORT->scr = IPS_BLK_PIN) : (IPS_BLK_PORT->clr = IPS_BLK_PIN))

#define CIRCLE_UPPER_RIGHT              0x01
#define CIRCLE_UPPER_LEFT               0x02
#define CIRCLE_LOWER_LEFT               0x04
#define CIRCLE_LOWER_RIGHT              0x08
#define CIRCLE_DRAW_ALL                 (CIRCLE_UPPER_RIGHT | CIRCLE_UPPER_LEFT | CIRCLE_LOWER_LEFT | CIRCLE_LOWER_RIGHT)

extern uint16_t IPS_penColor;
extern uint16_t IPS_backgroundColor;


typedef enum
{
    NORMAL = 1,
    XOR
} IPS_ColorMode_e;

typedef enum
{
    IPS_PORTAIT                      = 0,                                    // 纵向模式
    IPS_PORTAIT_180                  = 1,                                    // 纵向模式  旋转180
    IPS_CROSSWISE                    = 2,                                    // 横向模式
    IPS_CROSSWISE_180                = 3,                                    // 横向模式  旋转180
}IPS_dir_enum;


typedef enum
{
    IPS_6X8_FONT                     = 0,                                    // 6x8      字体
}IPS_font_size_enum;


void IPS_SetDrawColor(IPS_ColorMode_e mode);
void IPS_BufferCopy(uint8_t mode, void *buffer);
void IPS_SendBuffer(void);
void IPS_ClearBuffer(void);
void IPS_DrawPoint(int16_t x, int16_t y, const uint16_t color);
void IPS_DrawLine(int16_t x_start, int16_t y_start, int16_t x_end, int16_t y_end, const uint16_t color);
void IPS_ShowChar(int16_t x, int16_t y, const char dat);
void IPS_ShowStr(int16_t x, int16_t y, const char dat[]);
void IPS_ShowInt(int16_t x, int16_t y, const int32_t dat, uint8_t num);
void IPS_ShowUint(int16_t x, int16_t y, const uint32_t dat, uint8_t num);
void IPS_ShowFloat(int16_t x, int16_t y, const float dat, uint8_t num, uint8_t pointnum);
void IPS_ModifyColor(void);

void IPS_DrawCircle(int16_t x, int16_t y, uint16_t r, const uint16_t color, uint8_t section);
void IPS_DrawDisc(int16_t x, int16_t y, uint16_t r, const uint16_t color, uint8_t section);
void IPS_DrawFrame(int16_t x, int16_t y, uint16_t width, uint16_t height, const uint16_t color);
void IPS_DrawBox(int16_t x, int16_t y, uint16_t width, uint16_t height, const uint16_t color);
void IPS_DrawRFrame(int16_t x, int16_t y, uint16_t width, uint16_t height, const uint16_t color, uint8_t r);
void IPS_DrawRBox(int16_t x, int16_t y, uint16_t width, uint16_t height, const uint16_t color, uint8_t r);
void IPS_ShowBMP(int16_t x, int16_t y, uint16_t width, uint16_t height, const uint8_t *pic);
void IPS_ShowGrayImage(uint16_t x, uint16_t y, const uint8_t *image, uint16_t width, uint16_t height, uint16_t dis_width, uint16_t dis_height, uint8_t threshold);
void IPS_SetDirection(IPS_dir_enum dir);
void IPS_SetColor(const uint16_t pen, const uint16_t bgcolor);
void IPS_Init(void);

#endif // __IPS_COMMON_H__