import os
import numpy as np
from PIL import Image, ImageFont, ImageDraw
import argparse
import traceback
import sys

def generate_bitmap_array(image_path, output_path=None, array_name=None, 
                          reverse_bits=True, inverted=True, bit_format="阴码 逐行式 逆向", color_mode="BW"):
    """
    将图像转换为C语言位图数组
    
    参数:
    image_path - 输入图像路径
    output_path - 输出C文件路径
    array_name - 数组名称
    reverse_bits - 是否需要颠倒每个字节中的位(阴码/阳码)
    inverted - 是否反转颜色(0表示黑色或1表示白色)
    bit_format - 位模式描述
    color_mode - 颜色模式: "BW"(黑白), "RGB565"(16位彩色)
    """
    # 加载并处理图像
    try:
        img = Image.open(image_path)
        
        # 获取图像信息
        width, height = img.size
        
        # 默认数组名称
        if array_name is None:
            base_name = os.path.splitext(os.path.basename(image_path))[0]
            array_name = f"{base_name}_{width}{height}"
        
        # 默认输出路径
        if output_path is None:
            dirname = os.path.dirname(image_path)
            output_path = os.path.join(dirname, f"{array_name}.c")
        
        # 创建输出数组
        byte_array = []
        
        if color_mode == "RGB565":
            # 转换为RGB模式
            if img.mode != 'RGB':
                img = img.convert('RGB')
                
            # 获取像素数据
            pixels = np.array(img)
            
            # 处理每个像素
            for y in range(height):
                for x in range(width):
                    r, g, b = pixels[y, x]
                    
                    # 转换为RGB565格式 (5位R, 6位G, 5位B)
                    r = (r >> 3) & 0x1F  # 取高5位
                    g = (g >> 2) & 0x3F  # 取高6位
                    b = (b >> 3) & 0x1F  # 取高5位
                    
                    # 组合成16位值 (先高位后低位)
                    pixel_value = (r << 11) | (g << 5) | b
                    
                    # 添加到数组 (低位字节在前)
                    byte_array.append(pixel_value & 0xFF)         # 低字节
                    byte_array.append((pixel_value >> 8) & 0xFF)  # 高字节
            
            total_bytes = width * height * 2
        
        else:  # 黑白模式
            # 转换为黑白图像
            if img.mode != '1':
                img = img.convert('L')  # 转换为灰度
                img = img.point(lambda x: 0 if x < 128 else 255, '1')  # 二值化
                
            # 获取像素数据
            pixels = np.array(img, dtype=np.uint8)
            if inverted:
                pixels = 1 - pixels  # 反转颜色(如果需要)
                
            # 计算每行字节数
            bytes_per_row = (width + 7) // 8
            total_bytes = bytes_per_row * height
            
            # 处理每一行
            for y in range(height):
                row_bytes = []
                for x_byte in range(bytes_per_row):
                    byte_value = 0
                    
                    # 处理这个字节的8个位
                    for bit in range(8):
                        x = x_byte * 8 + bit
                        if x < width:
                            pixel_value = pixels[y, x]
                            
                            # 设置位值(根据reverse_bits)
                            if pixel_value:
                                if reverse_bits:
                                    byte_value |= (1 << bit)
                                else:
                                    byte_value |= (1 << (7 - bit))
                    
                    row_bytes.append(byte_value)
                byte_array.extend(row_bytes)
        
        # 生成C代码
        with open(output_path, 'w') as f:
            # 写入文件头
            f.write(f"""/*
 * Auto-generated bitmap array from {os.path.basename(image_path)}
 * {bit_format if color_mode == "BW" else "RGB565 格式"}
 * {width} * {height} pix
 * Color mode: {color_mode}
 */

#include "{os.path.splitext(os.path.basename(output_path))[0]}.h"

/*
 * {array_name}
 * {bit_format if color_mode == "BW" else "RGB565 格式"}
 * {width} * {height} pix
 * Color mode: {color_mode}
 */
const uint8_t {array_name}[{total_bytes}] = {{\n""")
            
            # 写入数组数据
            line = "    "
            for i, byte in enumerate(byte_array):
                line += f"0x{byte:02X},"
                if (i + 1) % 16 == 0:
                    f.write(line + "\n")
                    line = "    "
            
            # 写入剩余数据
            if line.strip():
                f.write(line + "\n")
            
            # 写入文件尾
            f.write("};\n")
        
        # 生成.h文件
        h_path = os.path.splitext(output_path)[0] + ".h"
        with open(h_path, 'w') as f:
            guard = os.path.splitext(os.path.basename(output_path))[0].upper() + "_H"
            f.write(f"""#ifndef __{guard}
#define __{guard}

#include "main.h"

/*
 * {array_name}
 * {bit_format if color_mode == "BW" else "RGB565 格式"}
 * {width} * {height} pix
 * Color mode: {color_mode}
 */
extern const uint8_t {array_name}[{total_bytes}];

#endif /* __{guard} */
""")
        
        print(f"成功生成数组: {array_name}")
        print(f"C文件: {output_path}")
        print(f"H文件: {h_path}")
        print(f"图像尺寸: {width}x{height} 像素")
        print(f"数组大小: {total_bytes} 字节")
        print(f"颜色模式: {color_mode}")
        
        return byte_array, width, height
        
    except Exception as e:
        print(f"错误: {e}")
        traceback.print_exc()
        return None, 0, 0

def text_to_bitmap(text, width, height, font_size=None, font_path=None, 
                  output_path=None, array_name=None, 
                  reverse_bits=True, inverted=True, bit_format="阴码 逐行式 逆向", color_mode="BW"):
    """
    生成文本图像并转换为位图数组
    """
    try:
        # 创建一个空白图像，使用彩色或黑白模式
        if color_mode == "RGB565":
            img = Image.new('RGB', (width, height), color=(255, 255, 255))
        else:
            img = Image.new('1', (width, height), color=1)
            
        draw = ImageDraw.Draw(img)
        
        # 设置字体
        if font_path is None:
            # 使用默认字体
            if font_size is None:
                font_size = height // 2  # 默认字体大小为高度的一半
            
            # 尝试加载一个好看的字体
            font = None
            try:
                # 尝试几种常见字体
                font_paths = [
                    "arial.ttf", 
                    "C:/Windows/Fonts/Arial.ttf",
                    "C:/Windows/Fonts/arialbd.ttf",
                    "C:/Windows/Fonts/calibri.ttf",
                    "C:/Windows/Fonts/calibrib.ttf",
                    "C:/Windows/Fonts/times.ttf",
                    "C:/Windows/Fonts/timesbd.ttf"
                ]
                
                for path in font_paths:
                    try:
                        font = ImageFont.truetype(path, font_size)
                        break
                    except:
                        continue
                        
                if font is None:
                    font = ImageFont.load_default()
            except:
                font = ImageFont.load_default()
        else:
            if font_size is None:
                font_size = height // 2
            font = ImageFont.truetype(font_path, font_size)
        
        # 计算文本大小以居中 - 使用现代方法
        try:
            # 新版PIL使用textbbox
            left, top, right, bottom = draw.textbbox((0, 0), text, font=font)
            text_width = right - left
            text_height = bottom - top
        except AttributeError:
            # 回退到估计值
            text_width = len(text) * font_size // 2
            text_height = font_size
        
        position = ((width - text_width) // 2, (height - text_height) // 2)
        
        # 绘制文本 (黑白或彩色)
        if color_mode == "RGB565":
            draw.text(position, text, fill=(0, 0, 0), font=font)
        else:
            draw.text(position, text, fill=0, font=font)
        
        # 保存临时图像
        temp_path = "temp_text_image.png"
        img.save(temp_path)
        
        # 生成位图数组
        result = generate_bitmap_array(
            temp_path, output_path, array_name, 
            reverse_bits, inverted, bit_format, color_mode
        )
        
        # 删除临时文件
        os.remove(temp_path)
        
        return result
    
    except Exception as e:
        print(f"生成文本图像时出错: {e}")
        traceback.print_exc()
        return None, 0, 0

def create_logo_image(width, height, text="IAPLink", output_path=None, color=True):
    """创建一个简单的logo图像文件"""
    try:
        # 创建一个空白图像
        if color:
            img = Image.new('RGB', (width, height), color=(255, 255, 255))
        else:
            img = Image.new('1', (width, height), color=1)
            
        draw = ImageDraw.Draw(img)
        
        # 尝试加载一个好看的字体
        try:
            # 尝试几种常见字体
            font_paths = [
                "C:/Windows/Fonts/arialbd.ttf",
                "C:/Windows/Fonts/timesbd.ttf",
                "C:/Windows/Fonts/calibrib.ttf",
                "C:/Windows/Fonts/Arial.ttf",
                "arial.ttf", 
                "Arial Bold.ttf",
                "timesbd.ttf"
            ]
            
            font = None
            for path in font_paths:
                try:
                    font = ImageFont.truetype(path, height // 2)
                    break
                except:
                    continue
                    
            if font is None:
                font = ImageFont.load_default()
        except:
            font = ImageFont.load_default()
        
        # 计算文本尺寸以居中
        try:
            # 新版PIL使用textbbox
            left, top, right, bottom = draw.textbbox((0, 0), text, font=font)
            text_width = right - left
            text_height = bottom - top
        except AttributeError:
            # 回退到估计值
            text_width = len(text) * height // 4
            text_height = height // 2
        
        position = ((width - text_width) // 2, (height - text_height) // 2)
        
        # 绘制文本 (彩色或黑白)
        if color:
            # 可以使用渐变色填充字体
            draw.text(position, text, fill=(0, 0, 128), font=font)  # 深蓝色
        else:
            draw.text(position, text, fill=0, font=font)
        
        # 保存图像
        if output_path is None:
            output_path = f"{text}_{width}x{height}.png"
        
        img.save(output_path)
        print(f"已创建图像: {output_path}")
        return output_path
    
    except Exception as e:
        print(f"创建logo图像时出错: {e}")
        traceback.print_exc()
        return None

def print_color_display_function():
    """打印彩色显示函数实现"""
    print("""
// 添加到您的IPS096.h文件中:
void IPS096_ShowBMP(int16_t x, int16_t y, uint16_t width, uint16_t height, const uint8_t *pic, uint8_t color_mode);

// 彩色版本的位图显示函数实现 - 添加到IPS096.c文件中:
void IPS096_ShowBMP(int16_t x, int16_t y, uint16_t width, uint16_t height, const uint8_t *pic, uint8_t color_mode)
{
    IPS096_SetWindows(x, y, x + width - 1, y + height - 1);
    
    if (color_mode == 1) {  // RGB565模式
        // 直接显示RGB565格式的图像数据
        for (uint32_t i = 0; i < width * height; i++) {
            // 每个像素2字节，低字节在前
            uint16_t color = pic[i*2] | (pic[i*2+1] << 8);
            IPS096_WriteData_16Bit(color);
        }
    } else {  // 黑白模式
        uint32_t pos = 0;
        uint8_t temp, t;
        uint16_t color;
        uint8_t bytesPerLine = (width + 7) / 8;
        
        for (uint16_t j = 0; j < height; j++) {
            for (uint16_t i = 0; i < width; i++) {
                if (i % 8 == 0) {
                    temp = pic[pos++];
                }
                
                t = temp & 0x01;  // 阴码读取LSB
                temp >>= 1;       // 右移一位
                
                color = (t == 0) ? IPS096_BACK_COLOR : IPS096_PEN_COLOR;
                IPS096_WriteData_16Bit(color);
            }
            
            // 跳过行末剩余字节
            if (width % 8 != 0)
                pos += (8 - (width % 8)) / 8;
        }
    }
}
""")
    print("// 调用例子: IPS096_ShowBMP(0, 0, 96, 96, color_image, 1); // 彩色模式")
    print("// 调用例子: IPS096_ShowBMP(0, 0, 96, 96, bw_image, 0);    // 黑白模式")

def main():
    parser = argparse.ArgumentParser(description='图像到位图数组转换工具')
    parser.add_argument('--image', help='输入图像路径')
    parser.add_argument('--text', help='直接创建文本图像')
    parser.add_argument('--width', type=int, default=140, help='图像宽度(仅用于文本模式)')
    parser.add_argument('--height', type=int, default=70, help='图像高度(仅用于文本模式)')
    parser.add_argument('--font-size', type=int, help='字体大小(仅用于文本模式)')
    parser.add_argument('--font', help='字体路径(仅用于文本模式)')
    parser.add_argument('--output', help='输出C文件路径')
    parser.add_argument('--name', help='数组名称')
    parser.add_argument('--normal-bits', action='store_false', dest='reverse_bits', help='使用阳码(默认为阴码)')
    parser.add_argument('--not-inverted', action='store_false', dest='inverted', help='不反转颜色')
    parser.add_argument('--color', choices=['BW', 'RGB565'], default='BW', help='颜色模式: BW(黑白), RGB565(16位彩色)')
    parser.add_argument('--print-func', action='store_true', help='打印彩色显示函数实现')
    args = parser.parse_args()
    
    # 如果要打印显示函数实现
    if args.print_func:
        print_color_display_function()
        return
    
    # 处理命令行参数
    if args.text:
        # 创建文本图像并转换
        text_to_bitmap(
            args.text, args.width, args.height, args.font_size, args.font,
            args.output, args.name, args.reverse_bits, args.inverted, 
            color_mode=args.color
        )
    elif args.image:
        # 从图像文件生成
        generate_bitmap_array(
            args.image, args.output, args.name, 
            args.reverse_bits, args.inverted,
            color_mode=args.color
        )
    else:
        # 交互式模式
        print("位图数组生成工具")
        print("================")
        print("支持生成黑白和彩色(RGB565)图像数组")
        print()
        
        mode = input("选择模式: [1] 从图像文件生成 [2] 生成文本图像 [3] 创建并转换logo [4] 显示彩色显示函数: ")
        
        if mode == "1":
            image_path = input("输入图像路径: ")
            if not os.path.exists(image_path):
                print(f"错误: 文件 '{image_path}' 不存在")
                return
                
            color_mode = input("颜色模式 [1] 黑白 [2] 彩色RGB565 (默认: 1): ")
            color_mode = "RGB565" if color_mode == "2" else "BW"
            
            array_name = input("数组名称 (留空为自动): ")
            output_path = input("输出C文件路径 (留空为自动): ")
            
            if color_mode == "BW":
                reverse_bits = input("使用阴码? (Y/n): ").lower() != 'n'
                inverted = input("反转颜色? (Y/n): ").lower() != 'n'
            else:
                reverse_bits = True  # 对RGB565不适用
                inverted = False     # 对RGB565不适用
            
            generate_bitmap_array(
                image_path, 
                output_path if output_path else None, 
                array_name if array_name else None,
                reverse_bits, 
                inverted,
                color_mode=color_mode
            )
            
        elif mode == "2":
            text = input("输入文本: ")
            width = int(input("宽度(像素) (默认: 140): ") or "140")
            height = int(input("高度(像素) (默认: 70): ") or "70")
            
            color_mode = input("颜色模式 [1] 黑白 [2] 彩色RGB565 (默认: 1): ")
            color_mode = "RGB565" if color_mode == "2" else "BW"
            
            font_size = input("字体大小 (留空为自动): ")
            font_size = int(font_size) if font_size else None
            
            font_path = input("字体路径 (留空为自动尝试系统字体): ") or None
            array_name = input("数组名称 (留空为自动): ")
            output_path = input("输出C文件路径 (留空为自动): ")
            
            if color_mode == "BW":
                reverse_bits = input("使用阴码? (Y/n): ").lower() != 'n'
                inverted = input("反转颜色? (Y/n): ").lower() != 'n'
            else:
                reverse_bits = True  # 对RGB565不适用
                inverted = False     # 对RGB565不适用
            
            text_to_bitmap(
                text, width, height, font_size, font_path,
                output_path if output_path else None,
                array_name if array_name else None,
                reverse_bits, inverted, color_mode=color_mode
            )
            
        elif mode == "3":
            text = input("Logo文本 (默认: IAPLink): ") or "IAPLink"
            width = int(input("宽度(像素) (默认: 140): ") or "140")
            height = int(input("高度(像素) (默认: 70): ") or "70")
            
            color_mode = input("颜色模式 [1] 黑白 [2] 彩色RGB565 (默认: 2): ") or "2"
            color_mode = "RGB565" if color_mode == "2" else "BW"
            use_color = color_mode == "RGB565"
            
            # 创建logo图像
            logo_path = create_logo_image(width, height, text, color=use_color)
            
            if logo_path:
                array_name = input("数组名称 (留空为自动): ")
                output_path = input("输出C文件路径 (留空为自动): ")
                
                if color_mode == "BW":
                    reverse_bits = input("使用阴码? (Y/n): ").lower() != 'n'
                    inverted = input("反转颜色? (Y/n): ").lower() != 'n'
                else:
                    reverse_bits = True  # 对RGB565不适用
                    inverted = False     # 对RGB565不适用
                
                generate_bitmap_array(
                    logo_path, 
                    output_path if output_path else None, 
                    array_name if array_name else None,
                    reverse_bits, 
                    inverted,
                    color_mode=color_mode
                )
                
                # 询问是否删除临时图像文件
                if input("删除临时图像文件? (Y/n): ").lower() != 'n':
                    try:
                        os.remove(logo_path)
                        print(f"已删除临时文件: {logo_path}")
                    except:
                        print(f"无法删除临时文件: {logo_path}")
        
        elif mode == "4":
            print_color_display_function()
            
        else:
            print("无效的选择!")

if __name__ == "__main__":
    main()