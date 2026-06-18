#include "ipsCommon.h"
#include "spix.h"
#include "font.h"
#include "at32f403a_407_gpio.h"
#include "function.h"
#include "timerx.h"

uint16_t IPS_penColor = IPS_DEFAULT_PENCOLOR;
uint16_t IPS_backgroundColor = IPS_DEFAULT_BGCOLOR;

static IPS_dir_enum IPS_display_dir = IPS_DEFAULT_DISPLAY_DIR;
static IPS_font_size_enum IPS_display_font = IPS_DEFAULT_DISPLAY_FONT;
static uint8_t IPS_x_max = SCREEN_WIDTH;
static uint8_t IPS_y_max = SCREEN_HEIGHT;
static uint8_t IPS_colorMode = NORMAL;
static uint16_t IPS_buffer[SCREEN_HEIGHT][SCREEN_WIDTH] = {0};
static uint8_t spi_tx_buffer[IPS_BUFFER_LENGTH];

#define IPS_write_8bit_data(data)    (spi_write_8bit(data))
#define IPS_write_16bit_data(data)   (spi_write_16bit(data))


static void IPS_WriteCmd(const uint8_t cmd);
void IPS_SetRegion(const uint16_t x1, const uint16_t y1, const uint16_t x2, const uint16_t y2);
void IPS_SetDirection(IPS_dir_enum dir);
void IPS_SetColor(const uint16_t pen, const uint16_t bgcolor);

/*!
 * @brief   Write Command to IPS
 * @param   cmd         Command to write
 * @return  None
 */
static void IPS_WriteCmd(const uint8_t cmd){
    IPS_DC(0);
    IPS_write_8bit_data(cmd);
    IPS_DC(1);
}


/*!
 * @brief   Set region of IPS
 * @param   x1          Start x coordinate
 * @param   y1          Start y coordinate
 * @param   x2          End x coordinate
 * @param   y2          End y coordinate
 * @return  None
 * @note    None
 */
void IPS_SetRegion(const uint16_t x1, const uint16_t y1, const uint16_t x2, const uint16_t y2)
{
    if(IPS_display_dir == IPS_PORTAIT || IPS_display_dir == IPS_PORTAIT_180) {
        IPS_WriteCmd(0x2a);
        IPS_write_16bit_data(x1 + 24);
        IPS_write_16bit_data(x2 + 24);
        IPS_WriteCmd(0x2b);
        IPS_write_16bit_data(y1);
        IPS_write_16bit_data(y2);
    } else {
        IPS_WriteCmd(0x2a);
        IPS_write_16bit_data(x1);
        IPS_write_16bit_data(x2);
        IPS_WriteCmd(0x2b);
        IPS_write_16bit_data(y1 + 24);
        IPS_write_16bit_data(y2 + 24);
    }
    IPS_WriteCmd(0x2c);
}



/*!
 * @brief   Set display direction of IPS
 * @param   dir         Display direction
 * @return  None
 * @note    None
 */
void IPS_SetDirection(IPS_dir_enum dir)
{
    IPS_display_dir = dir;
    switch(IPS_display_dir)
    {
    case IPS_PORTAIT:
    case IPS_PORTAIT_180:
    {
        IPS_x_max = SCREEN_HEIGHT;
        IPS_y_max = SCREEN_WIDTH;
    }break;
    case IPS_CROSSWISE:
    case IPS_CROSSWISE_180:
    {
        IPS_x_max = SCREEN_WIDTH;
        IPS_y_max = SCREEN_HEIGHT;
    }break;
    }
}



/*!
 * @brief   Set color of IPS
 * @param   pen         Pen color
 * @param   bgcolor     Background color
 * @return  None
 * @note    None
 */
void IPS_SetColor(const uint16_t pen, const uint16_t bgcolor)
{
    IPS_penColor = pen;
    IPS_backgroundColor = bgcolor;
}


void IPS_SetDrawColor(IPS_ColorMode_e mode){
    IPS_colorMode = mode;
}


/*!
 * @brief   Copy buffer to IPS
 * @param   mode        0: Copy from IPS_buffer to buffer, 1: Copy from buffer to IPS_buffer
 * @param   buffer      Buffer pointer
 * @return  None
 * @note    None
 */
void IPS_BufferCopy(uint8_t mode, void *buffer)
{
    if (buffer == NULL) return;
    
    if (mode == 0) {
        memcpy(buffer, IPS_buffer, sizeof(IPS_buffer));
    } else if (mode == 1) {
        memcpy(IPS_buffer, buffer, sizeof(IPS_buffer));
    }
}



/*!
 * @brief   Send buffer to IPS
 * @return  None
 * @note    None
 */
void IPS_SendBuffer(void)
{
    IPS_CS(0);
    IPS_SetRegion(0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);
    #if DMA_ENABLE
    uint16_t *src = (uint16_t *)IPS_buffer;
    uint8_t *dst = spi_tx_buffer;

    const uint32_t totalSize = IPS_BUFFER_LENGTH >> 1;
    for (uint32_t i = 0; i < totalSize; i++) {
        *dst++ = (uint8_t)(*src >> 8);
        *dst++ = (uint8_t)(*src & 0xFF);
        src++;
    }
    spi_send_by_dma(spi_tx_buffer, IPS_BUFFER_LENGTH);

    #else
    for(int i = 0; i < IPS_BUFFER_LENGTH; i++){
        IPS_write_8bit_data(IPS_buffer[i]);
    }
    IPS_CS(1);
    #endif
}


/*!
 * @brief   Clear buffer
 * @param   None
 * @return  None
 * @note    Use black background color can easily clear the buffer by memset,
 *          but other color cannot and on board flash is not enough for another buffer
 *          to use memcpy, so using color other than black will slower this function.
 */
void IPS_ClearBuffer(void)
{
    if (IPS_backgroundColor == 0)
    {
        memset(IPS_buffer, 0, sizeof(IPS_buffer));
    }
    else
    {
        uint16_t line_buffer[IPS_x_max];
        for (int j = 0; j < IPS_x_max; j++)
        {
            line_buffer[j] = IPS_backgroundColor;
        }

        for (int i = 0; i < IPS_y_max; i++)
        {
            memcpy(&IPS_buffer[i][0], line_buffer, sizeof(line_buffer));
        }
    }
}


/*!
 * @brief   IPS 画点
 * @param   x           坐标x方向的位置 参数范围 [0, IPS_x_max-1]
 * @param   y           坐标y方向的位置 参数范围 [0, IPS_y_max-1]
 * @param   color       颜色格式 RGB565 可以使用 zf_common_font.h 中 rgb565_color_enum 枚举值或者自行写入
 * @return  None
 * @note    IPS_draw_point(0, 0, RGB565_RED);            // 坐标 0,0 画一个红色的点
 */
void IPS_DrawPoint(int16_t x, int16_t y, const uint16_t color)
{
    if (x < IPS_x_max && y < IPS_y_max && x >= 0 && y >= 0)
    {
        if (IPS_colorMode == XOR)
        {
            if (IPS_buffer[y][x] == color)
                IPS_buffer[y][x] = IPS_backgroundColor;
            else
                IPS_buffer[y][x] = color;
        } else
            IPS_buffer[y][x] = color;
    }
}


/*!
 * @brief   IPS 画线
 * @param   x_start     起点x坐标
 * @param   y_start     起点y坐标
 * @param   x_end       终点x坐标
 * @param   y_end       终点y坐标
 * @param   color       颜色格式 RGB565 可以使用 zf_common_font.h 中 rgb565_color_enum 枚举值或者自行写入
 * @return  None
 * @note    IPS_draw_line(0, 0, 10, 10, RGB565_RED);     // 坐标 0,0 到 10,10 画一条红色线段
 */
void IPS_DrawLine(int16_t x_start, int16_t y_start, int16_t x_end, int16_t y_end, const uint16_t color)
{
    int16_t x_dir = (x_start < x_end ? 1 : -1);
    int16_t y_dir = (y_start < y_end ? 1 : -1);
    float temp_rate = 0;
    float temp_b = 0;
    do
    {
        if (x_start != x_end)
        {
            temp_rate = (float) (y_start - y_end) / (float) (x_start - x_end);
            temp_b = (float) y_start - (float) x_start * temp_rate;
        } else
        {
            while (y_start != y_end)
            {
                IPS_DrawPoint(x_start, y_start, color);
                y_start += y_dir;
            }
            break;
        }
        if (func_abs(y_start - y_end) > func_abs(x_start - x_end))
        {
            while (y_start != y_end)
            {
                IPS_DrawPoint(x_start, y_start, color);
                y_start += y_dir;
                x_start = (int16_t) (((float) y_start - temp_b) / temp_rate);
            }
        } else
        {
            while (x_start != x_end)
            {
                IPS_DrawPoint(x_start, y_start, color);
                x_start += x_dir;
                y_start = (int16_t) ((float) x_start * temp_rate + temp_b);
            }
        }
    } while (0);
}


/*!
 * @brief   IPS 显示字符
 * @param   x           坐标x方向的位置 参数范围 [0, IPS_x_max-1]
 * @param   y           坐标y方向的位置 参数范围 [0, IPS_y_max-1]
 * @param   dat         需要显示的字符
 * @return  None
 * @note    IPS_show_char(0, 0, 'A');                     // 坐标 0,0 显示字符 A
 */
void IPS_ShowChar(int16_t x, int16_t y, const char dat)
{
    uint8_t i, j;
    switch (IPS_display_font)
    {
    case IPS_6X8_FONT:
        for (i = 0; i < 6; i++)
        {
           // 减去 32 是因为在 ASCII 编码表中，可见字符是从第 32 个字符开始的
            uint8_t temp_top = ascii_font_6x8[dat - 32][i];
            for (j = 0; j < 8; j++)
            {
                if (temp_top & 0x01)
                {
                    IPS_DrawPoint(x + i, y + j ,IPS_penColor);
                }
                temp_top >>= 1;
            }
        }
        break;
    default:
        break;
    }
}



/*!
 * @brief   IPS 显示字符串
 * @param   x           坐标x方向的位置 参数范围 [0, IPS_x_max-1]
 * @param   y           坐标y方向的位置 参数范围 [0, IPS_y_max-1]
 * @param   dat         需要显示的字符串
 * @return  None
 * @note    IPS_show_string(0, 0, "seekfree");
 */
void IPS_ShowStr(int16_t x, int16_t y, const char dat[])
{
    uint16_t j = 0;
    while (dat[j] != '\0')
    {
        switch (IPS_display_font)
        {
        case IPS_6X8_FONT:
            IPS_ShowChar(x + 6 * j, y, dat[j]);
            j++;
            break;
        default:
            break;
        }
    }
}


/*!
 * @brief   IPS 显示32位有符号 (去除整数部分无效的0)
 * @param   x           坐标x方向的位置 参数范围 [0, IPS_x_max-1]
 * @param   y           坐标y方向的位置 参数范围 [0, IPS_y_max-1]
 * @param   dat         需要显示的变量 数据类型 int32
 * @param   num         需要显示的位数 最高10位  不包含正负号
 * @return  None
 * @example IPS_show_int(0, 0, x, 3); 
 * @note    负数会显示一个 '-'号   正数显示一个空格
 */
void IPS_ShowInt(int16_t x, int16_t y, const int32_t dat, uint8_t num)
{
    int32_t dat_temp = dat;
    int32_t offset = 1;
    char data_buffer[12];

    memset(data_buffer, 0, 12);
    memset(data_buffer, ' ', num + 1);

    if (num < 10)
    {
        for (; num > 0; num--)
        {
            offset *= 10;
        }
        dat_temp %= offset;
    }
    func_int_to_str(data_buffer, dat_temp);
    IPS_ShowStr(x, y, (const char *) &data_buffer);
}


/*!
 * @brief   IPS 显示32位无符号 (去除整数部分无效的0)
 * @param   x           坐标x方向的位置 参数范围 [0, IPS_x_max-1]
 * @param   y           坐标y方向的位置 参数范围 [0, IPS_y_max-1]
 * @param   dat         需要显示的变量 数据类型 uint32
 * @param   num         需要显示的位数 最高10位  不包含正负号
 * @return  None
 * @example IPS_show_uint(0, 0, x, 3); 
 * @note    负数会显示一个 '-'号   正数显示一个空格
 */
void IPS_ShowUint(int16_t x, int16_t y, const uint32_t dat, uint8_t num)
{
    uint32_t dat_temp = dat;
    int32_t offset = 1;
    char data_buffer[12];
    memset(data_buffer, 0, 12);
    memset(data_buffer, ' ', num);

    if (num < 10)
    {
        for (; num > 0; num--)
        {
            offset *= 10;
        }
        dat_temp %= offset;
    }
    func_uint_to_str(data_buffer, dat_temp);
    IPS_ShowStr(x, y, (const char *) &data_buffer);
}


/*!
 * @brief   IPS 显示浮点数 (去除整数部分无效的0)
 * @param   x           坐标x方向的位置 参数范围 [0, IPS_x_max-1]
 * @param   y           坐标y方向的位置 参数范围 [0, IPS_y_max-1]
 * @param   dat         需要显示的变量 数据类型 float 或 double
 * @param   num         整数位显示长度   最高8位
 * @param   pointnum    小数位显示长度   最高6位
 * @return  None
 * @example IPS_show_float(0, 0, x, 2, 3); 
 * @note    特别注意当发现小数部分显示的值与你写入的值不一样的时候，
 *          可能是由于浮点数精度丢失问题导致的，这并不是显示函数的问题，
 *          有关问题的详情，请自行百度学习   浮点数精度丢失问题。
 *          负数会显示一个 '-'号   正数显示一个空格
 */
void IPS_ShowFloat(int16_t x, int16_t y, const float dat, uint8_t num, uint8_t pointnum)
{
    float dat_temp = dat;
    float offset = 1.0;
    char data_buffer[17];
    memset(data_buffer, 0, 17);
    memset(data_buffer, ' ', num + pointnum + 2);

    if (num < 10)
    {
        for (; num > 0; num--)
        {
            offset *= 10;
        }
        dat_temp = dat_temp - ((int) dat_temp / (int) offset) * offset;
    }
    func_float_to_str(data_buffer, dat_temp, pointnum);
    IPS_ShowStr(x, y, data_buffer);
}



bool reversedColor = false;
/*!
 * @brief   IPS 修改颜色
 * @param   None
 * @return  None
 * @note    None
 */
void IPS_ModifyColor(void)
{
    if (reversedColor)
    {
        IPS_penColor = IPS_DEFAULT_BGCOLOR;
        IPS_backgroundColor = IPS_DEFAULT_PENCOLOR;
    } else
    {
        IPS_penColor = IPS_DEFAULT_PENCOLOR;
        IPS_backgroundColor = IPS_DEFAULT_BGCOLOR;
    }
}


/*!
 * @brief   IPS 画圆
 * @param   x           圆心x坐标
 * @param   y           圆心y坐标
 * @param   r           圆半径
 * @param   color       颜色格式 RGB565 可以使用 zf_common_font.h 中 rgb565_color_enum 枚举值或者自行写入
 * @param   section     画圆的部分，可以使用 CIRCLE_UPPER_RIGHT, CIRCLE_UPPER_LEFT, CIRCLE_LOWER_LEFT, CIRCLE_LOWER_RIGHT, CIRCLE_DRAW_ALL
 * @return  None
 * @note    IPS_draw_circle(80, 40, 20, RGB565_RED, CIRCLE_DRAW_ALL);     // 画一个红色的圆
 */
void IPS_DrawCircle(int16_t x, int16_t y, uint16_t r, const uint16_t color, uint8_t section)
{
    // y = kx, k = 1
    uint16_t x0 = (uint16_t) (r * cos(0.01745 * 45));
    uint16_t fx;

    // x^2 + y^2 = r^2, y->x / x->y
    for (int i = -x0 + 1; i < 0; ++i)
    {
        fx = (uint16_t) sqrt(pow(r, 2) - pow(i, 2));
        if (section & CIRCLE_UPPER_RIGHT)
        {
            IPS_DrawPoint(x - i, y - fx, color);
            IPS_DrawPoint(x + fx, y + i, color);
        }
        if (section & CIRCLE_UPPER_LEFT)
        {
            IPS_DrawPoint(x + i, y - fx, color);
            IPS_DrawPoint(x - fx, y + i, color);
        }
        if (section & CIRCLE_LOWER_LEFT)
        {
            IPS_DrawPoint(x + i, y + fx, color);
            IPS_DrawPoint(x - fx, y - i, color);
        }
        if (section & CIRCLE_LOWER_RIGHT)
        {
            IPS_DrawPoint(x - i, y + fx, color);
            IPS_DrawPoint(x + fx, y - i, color);
        }
    }

    // Add support for XOR color mode
    fx = (uint16_t) sqrt(pow(r, 2) - pow(x0, 2));
    if (section & CIRCLE_UPPER_RIGHT)
    {
        if (r > 1)
        {
            IPS_DrawPoint(x + r, y, color);
            IPS_DrawPoint(x, y - r, color);
        }
        if (x0 == fx)
            IPS_DrawPoint(x + x0, y - x0, color);
        else
        {
            IPS_DrawPoint(x + x0, y - fx, color);
            IPS_DrawPoint(x + fx, y - x0, color);
        }
    }
    if (section & CIRCLE_UPPER_LEFT)
    {
        if (r > 1)
        {
            IPS_DrawPoint(x - r, y, color);
            IPS_DrawPoint(x, y - r, color);
        }
        if (x0 == fx)
            IPS_DrawPoint(x - x0, y - x0, color);
        else
        {
            IPS_DrawPoint(x - x0, y - fx, color);
            IPS_DrawPoint(x - fx, y - x0, color);
        }
    }
    if (section & CIRCLE_LOWER_LEFT)
    {
        if (r > 1)
        {
            IPS_DrawPoint(x - r, y, color);
            IPS_DrawPoint(x, y + r, color);
        }
        if (x0 == fx)
            IPS_DrawPoint(x - x0, y + x0, color);
        else
        {
            IPS_DrawPoint(x - x0, y + fx, color);
            IPS_DrawPoint(x - fx, y + x0, color);
        }
    }
    if (section & CIRCLE_LOWER_RIGHT)
    {
        if (r > 1)
        {
            IPS_DrawPoint(x + r, y, color);
            IPS_DrawPoint(x, y + r, color);
        }
        if (x0 == fx)
            IPS_DrawPoint(x + x0, y + x0, color);
        else
        {
            IPS_DrawPoint(x + x0, y + fx, color);
            IPS_DrawPoint(x + fx, y + x0, color);
        }
    }
    if (section == CIRCLE_DRAW_ALL)
    {
        IPS_DrawPoint(x + r, y, color);
        IPS_DrawPoint(x - r, y, color);
        IPS_DrawPoint(x, y - r, color);
        IPS_DrawPoint(x, y + r, color);
    }
}


void IPS_DrawDisc(int16_t x, int16_t y, uint16_t r, const uint16_t color, uint8_t section)
{
    // y = kx, k = 1
    uint16_t x0 = (uint16_t) (r * cos(0.01745 * 45));
    uint16_t fx;

    // x^2 + y^2 = r^2, y->x / x->y
    for (int i = -x0 + 1; i < 0; ++i)
    {
        fx = (uint16_t) sqrt(pow(r, 2) - pow(i, 2));
        if (section & CIRCLE_UPPER_RIGHT)
        {
            IPS_DrawLine(x - i, y - fx, x - i, y + i, color);
            IPS_DrawLine(x + fx, y + i, x - i, y + i, color);
        }
        if (section & CIRCLE_UPPER_LEFT)
        {
            IPS_DrawLine(x + i, y - fx, x + i, y + i, color);
            IPS_DrawLine(x - fx, y + i, x + i, y + i, color);
        }
        if (section & CIRCLE_LOWER_LEFT)
        {
            IPS_DrawLine(x + i, y + fx, x + i, y - i, color);
            IPS_DrawLine(x - fx, y - i, x + i, y - i, color);
        }
        if (section & CIRCLE_LOWER_RIGHT)
        {
            IPS_DrawLine(x - i, y + fx, x - i, y - i, color);
            IPS_DrawLine(x + fx, y - i, x - i, y - i, color);
        }
    }

    // Add support for XOR color mode
    IPS_DrawPoint(x, y, color);
    if (r != 2)
        IPS_DrawPoint(x, y, color);

    fx = (uint16_t) sqrt(pow(r, 2) - pow(x0, 2));
    if (section & CIRCLE_UPPER_RIGHT)
    {
        if (r > 1)
        {
            IPS_DrawLine(x + r, y, x, y, color);
            IPS_DrawLine(x, y - r, x, y, color);
        }
        if (r > 2)
            IPS_DrawLine(x, y, x + x0, y - x0, color);
        if (x0 == fx)
            IPS_DrawPoint(x + x0, y - x0, color);
        else
        {
            IPS_DrawPoint(x + x0, y - fx, color);
            IPS_DrawPoint(x + fx, y - x0, color);
            IPS_DrawPoint(x + x0, y - x0, color);
        }
    }
    if (section & CIRCLE_UPPER_LEFT)
    {
        if (r > 1)
        {
            IPS_DrawLine(x - r, y, x, y, color);
            IPS_DrawLine(x, y - r, x, y, color);
        }
        if (r > 2)
            IPS_DrawLine(x, y, x - x0, y - x0, color);
        if (x0 == fx)
            IPS_DrawPoint(x - x0, y - x0, color);
        else
        {
            IPS_DrawPoint(x - x0, y - fx, color);
            IPS_DrawPoint(x - fx, y - x0, color);
            IPS_DrawPoint(x - x0, y - x0, color);
        }
    }
    if (section & CIRCLE_LOWER_LEFT)
    {
        if (r > 1)
        {
            IPS_DrawLine(x - r, y, x, y, color);
            IPS_DrawLine(x, y + r, x, y, color);
        }
        if (r > 2)
            IPS_DrawLine(x, y, x - x0, y + x0, color);
        if (x0 == fx)
            IPS_DrawPoint(x - x0, y + x0, color);
        else
        {
            IPS_DrawPoint(x - x0, y + fx, color);
            IPS_DrawPoint(x - fx, y + x0, color);
            IPS_DrawPoint(x - x0, y + x0, color);
        }
    }
    if (section & CIRCLE_LOWER_RIGHT)
    {
        if (r > 1)
        {
            IPS_DrawLine(x + r, y, x, y, color);
            IPS_DrawLine(x, y + r, x, y, color);
        }
        if (r > 2)
            IPS_DrawLine(x, y, x + x0, y + x0, color);
        if (x0 == fx)
            IPS_DrawPoint(x + x0, y + x0, color);
        else
        {
            IPS_DrawPoint(x + x0, y + fx, color);
            IPS_DrawPoint(x + fx, y + x0, color);
            IPS_DrawPoint(x + x0, y + x0, color);
        }
    }
    if (section == CIRCLE_DRAW_ALL)
    {
        IPS_DrawLine(x + r, y, x, y, color);
        IPS_DrawLine(x - r, y, x, y, color);
        IPS_DrawLine(x, y - r, x, y, color);
        IPS_DrawLine(x, y + r, x, y, color);
        IPS_DrawPoint(x, y, color);
    }
}

void IPS_DrawFrame(int16_t x, int16_t y, uint16_t width, uint16_t height, const uint16_t color)
{
    for (int i = x; i < x + width; i++)
    {
        IPS_DrawPoint(i, y, color);
        IPS_DrawPoint(i, y + height - 1, color);
    }
    for (int j = y; j < y + height; j++)
    {
        IPS_DrawPoint(x, j, color);
        IPS_DrawPoint(x + width - 1, j, color);
    }
}
void IPS_DrawBox(int16_t x, int16_t y, uint16_t width, uint16_t height, const uint16_t color)
{
    for (int i = x; i < x + width; ++i)
    {
        for (int j = y; j < y + height; ++j)
        {
            IPS_DrawPoint(i, j, color);
        }
    }
}


void IPS_DrawRFrame(int16_t x, int16_t y, uint16_t width, uint16_t height, const uint16_t color, uint8_t r)
{
    for (int i = x + r + 1; i < x + width - r - 1; i++)
    {
        IPS_DrawPoint(i, y, color);
        IPS_DrawPoint(i, y + height - 1, color);
    }
    for (int j = y + r + 1; j < y + height - r - 1; j++)
    {
        IPS_DrawPoint(x, j, color);
        IPS_DrawPoint(x + width - 1, j, color);
    }

    IPS_DrawCircle(x + r, y + r, r, color, CIRCLE_UPPER_LEFT);
    IPS_DrawCircle(x + width - 1 - r, y + r, r, color, CIRCLE_UPPER_RIGHT);
    IPS_DrawCircle(x + r, y + height - 1 - r, r, color, CIRCLE_LOWER_LEFT);
    IPS_DrawCircle(x + width - 1 - r, y + height - 1 - r, r, color, CIRCLE_LOWER_RIGHT);
}
void IPS_DrawRBox(int16_t x, int16_t y, uint16_t width, uint16_t height, const uint16_t color, uint8_t r)
{
    IPS_DrawDisc(x + r, y + r, r, color, CIRCLE_UPPER_LEFT);
    IPS_DrawDisc(x + width - 1 - r, y + r, r, color, CIRCLE_UPPER_RIGHT);
    IPS_DrawDisc(x + r, y + height - 1 - r, r, color, CIRCLE_LOWER_LEFT);
    IPS_DrawDisc(x + width - 1 - r, y + height - 1 - r, r, color, CIRCLE_LOWER_RIGHT);

    IPS_DrawBox(x + r + 1, y, width - 2 - 2 * r, r + 1, color);
    IPS_DrawBox(x, y + r + 1, width, height - 2 * r - 2, color);
    IPS_DrawBox(x + r + 1, y + height - 1 - r, width - 2 - 2 * r, r + 1, color);
}


void IPS_ShowBMP(int16_t x, int16_t y, uint16_t width, uint16_t height, const uint8_t *pic)
{
    uint8_t temp, j;
    uint8_t x0 = x;
    uint8_t *tmp = (uint8_t *) pic;
    uint16_t i, picSize = 0;

    picSize = (width / 8 + ((width % 8) ? 1 : 0)) * height;

    for (i = 0; i < picSize; i++)
    {
        temp = tmp[i];
        for (j = 0; j < 8; j++)
        {
            if (temp & 0x01)
            {
                IPS_DrawPoint(x, y, IPS_penColor);
            }
            temp >>= 1;
            x++;

            if ((x - x0) == width)
            {
                x = x0;
                y++;
                break;
            }
        }
    }
}


void IPS_ShowGrayImage(uint16_t x, uint16_t y, const uint8_t *image, uint16_t width, uint16_t height, uint16_t dis_width,
                     uint16_t dis_height, uint8_t threshold)
{
    uint16_t i, j;
    uint16_t color, temp;
    uint32_t width_index, height_index;

    IPS_CS(0);
    for (j = 0; j < dis_height; j++)
    {
        height_index = j * height / dis_height;
        for (i = 0; i < dis_width; i++)
        {
            width_index = i * width / dis_width;
            temp = *(image + height_index * width + width_index);
            if (threshold == 0)
            {
                color = (0x001f & ((temp) >> 3)) << 11;
                color = color | (((0x003f) & ((temp) >> 2)) << 5);
                color = color | (0x001f & ((temp) >> 3));
                IPS_DrawPoint(x + i, y + j, color);
            } else if (temp < threshold)
                IPS_DrawPoint(x + i, y + j, RGB565_BLACK);
            else
                IPS_DrawPoint(x + i, y + j, RGB565_WHITE);
        }
    }
    IPS_CS(1);
}


/*!
 * @brief   IPS 初始化
 * @param   None
 * @return  None
 * @note    None
 */
void IPS_Init(void){
    gpio_init_type gpio_init_struct;
    gpio_default_para_init(&gpio_init_struct);

    gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_MODERATE;
    gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
    gpio_init_struct.gpio_mode = GPIO_MODE_MUX;
    gpio_init_struct.gpio_pins = IPS_SCK_PIN;
    gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
    gpio_init(IPS_SCK_PORT, &gpio_init_struct);

    gpio_init_struct.gpio_mode = GPIO_MODE_MUX;
    gpio_init_struct.gpio_pins = IPS_SDA_PIN;
    gpio_init(IPS_SDA_PORT, &gpio_init_struct);

    gpio_init_struct.gpio_mode = GPIO_MODE_OUTPUT;
    gpio_init_struct.gpio_pins = IPS_CS_PIN;
    gpio_init(IPS_CS_PORT, &gpio_init_struct);
    gpio_bits_set(IPS_CS_PORT, IPS_CS_PIN);

    gpio_init_struct.gpio_pins = IPS_DC_PIN;
    gpio_init(IPS_DC_PORT, &gpio_init_struct);
    gpio_bits_reset(IPS_DC_PORT, IPS_DC_PIN);

    gpio_init_struct.gpio_pins = IPS_RST_PIN;
    gpio_init(IPS_RST_PORT, &gpio_init_struct);
    gpio_bits_reset(IPS_RST_PORT, IPS_RST_PIN);

    gpio_init_struct.gpio_pins = IPS_BLK_PIN;
    gpio_init(IPS_BLK_PORT, &gpio_init_struct);
    gpio_bits_set(IPS_BLK_PORT, IPS_BLK_PIN);

    SPIx_Init();

    IPS_SetDirection(IPS_display_dir);
    IPS_SetColor(IPS_penColor, IPS_backgroundColor);

    IPS_RST(0);
    delay_ms(200);

    IPS_RST(1);
    delay_ms(100);

    IPS_CS(0);
    IPS_WriteCmd(0x36);
    delay_ms(100);
    if(IPS_display_dir == 0)
    {
        IPS_write_8bit_data(0x08);
    }
    else if(IPS_display_dir == 1)
    {
        IPS_write_8bit_data(0xC8);
    }
    else if(IPS_display_dir == 2)
    {
        IPS_write_8bit_data(0x78);
    }
    else
    {
        IPS_write_8bit_data(0xA8);
    }

    IPS_WriteCmd(0x3A);
    IPS_write_8bit_data(0x05);

    IPS_WriteCmd(0xB2);
    IPS_write_8bit_data(0x0C);
    IPS_write_8bit_data(0x0C);
    IPS_write_8bit_data(0x00);
    IPS_write_8bit_data(0x33);
    IPS_write_8bit_data(0x33);

    IPS_WriteCmd(0xB7);
    IPS_write_8bit_data(0x35);

    IPS_WriteCmd(0xBB);
    IPS_write_8bit_data(0x37);

    IPS_WriteCmd(0xC0);
    IPS_write_8bit_data(0x2C);

    IPS_WriteCmd(0xC2);
    IPS_write_8bit_data(0x01);

    IPS_WriteCmd(0xC3);
    IPS_write_8bit_data(0x12);

    IPS_WriteCmd(0xC4);
    IPS_write_8bit_data(0x20);

    IPS_WriteCmd(0xC5);
    IPS_write_8bit_data(0x06);

    IPS_WriteCmd(0xC6);
    IPS_write_8bit_data(0x0F);

    IPS_WriteCmd(0xD0);
    IPS_write_8bit_data(0xA4);
    IPS_write_8bit_data(0xA1);

    IPS_WriteCmd(0xE0);
    IPS_write_8bit_data(0xD0);
    IPS_write_8bit_data(0x04);
    IPS_write_8bit_data(0x0D);
    IPS_write_8bit_data(0x11);
    IPS_write_8bit_data(0x13);
    IPS_write_8bit_data(0x2B);
    IPS_write_8bit_data(0x3F);
    IPS_write_8bit_data(0x54);
    IPS_write_8bit_data(0x4C);
    IPS_write_8bit_data(0x18);
    IPS_write_8bit_data(0x0D);
    IPS_write_8bit_data(0x0B);
    IPS_write_8bit_data(0x1F);
    IPS_write_8bit_data(0x23);

    IPS_WriteCmd(0xE1);
    IPS_write_8bit_data(0xD0);
    IPS_write_8bit_data(0x04);
    IPS_write_8bit_data(0x0C);
    IPS_write_8bit_data(0x11);
    IPS_write_8bit_data(0x13);
    IPS_write_8bit_data(0x2C);
    IPS_write_8bit_data(0x3F);
    IPS_write_8bit_data(0x44);
    IPS_write_8bit_data(0x51);
    IPS_write_8bit_data(0x2F);
    IPS_write_8bit_data(0x1F);
    IPS_write_8bit_data(0x1F);
    IPS_write_8bit_data(0x20);
    IPS_write_8bit_data(0x23);

    IPS_WriteCmd(0x11);
    delay_ms(120);

    IPS_WriteCmd(0x29);
    IPS_CS(1);

    IPS_ClearBuffer();
    IPS_SendBuffer();
}