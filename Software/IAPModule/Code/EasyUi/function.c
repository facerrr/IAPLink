#include "function.h"

int32_t func_str_to_int (char *str)
{
//    zf_assert(str != NULL);
    uint8_t sign = 0;                                                             // ?????? 0-???? 1-????
    int32_t temp = 0;                                                             // ??????????
    do
    {
        if(NULL == str)
        {
            break;
        }

        if('-' == *str)                                                         // ????????????????
        {
            sign = 1;                                                           // ??????
            str ++;
        }
        else if('+' == *str)                                                    // ?????????????????
        {
            str ++;
        }

        while(('0' <= *str) && ('9' >= *str))                                   // ????????????
        {
            temp = temp * 10 + ((uint8_t)(*str) - 0x30);                          // ???????
            str ++;
        }

        if(sign)
        {
            temp = -temp;
        }
    }while(0);
    return temp;
}


void func_int_to_str (char *str, int32_t number)
{
//    zf_assert(str != NULL);
    uint8_t data_temp[16];                                                        // ??????
    uint8_t bit = 0;                                                              // ????¦Ë??
    int32_t number_temp = 0;

    do
    {
        if(NULL == str)
        {
            break;
        }

        if(0 > number)                                                          // ????
        {
            *str ++ = '-';
            number = -number;
        }
        else if(0 == number)                                                    // ????????? 0
        {
            *str = '0';
            break;
        }

        while(0 != number)                                                      // ?????????????
        {
            number_temp = number % 10;
            data_temp[bit ++] = func_abs(number_temp);                          // ??????????????
            number /= 10;                                                       // ????????????¦Ë??
        }
        while(0 != bit)                                                         // ???????????????????
        {
            *str ++ = (data_temp[bit - 1] + 0x30);                              // ???????????????§Ö?????? ???????????????
            bit --;
        }
    }while(0);
}


uint32_t func_str_to_uint (char *str)
{
//    zf_assert(str != NULL);
    uint32_t temp = 0;                                                            // ??????????

    do
    {
        if(NULL == str)
        {
            break;
        }

        while(('0' <= *str) && ('9' >= *str))                                  // ????????????
        {
            temp = temp * 10 + ((uint8_t)(*str) - 0x30);                         // ???????
            str ++;
        }
    }while(0);

    return temp;
}


void func_uint_to_str (char *str, uint32_t number)
{
//    zf_assert(str != NULL);
    int8_t data_temp[16];                                                         // ??????
    uint8_t bit = 0;                                                              // ????¦Ë??

    do
    {
        if(NULL == str)
        {
            break;
        }

        if(0 == number)                                                         // ????? 0
        {
            *str = '0';
            break;
        }

        while(0 != number)                                                      // ?????????????
        {
            data_temp[bit ++] = (number % 10);                                  // ??????????????
            number /= 10;                                                       // ????????????¦Ë??
        }
        while(0 != bit)                                                         // ???????????????????
        {
            *str ++ = (data_temp[bit - 1] + 0x30);                              // ???????????????§Ö?????? ???????????????
            bit --;
        }
    }while(0);
}


float func_str_to_float (char *str)
{
//    zf_assert(str != NULL);
    uint8_t sign = 0;                                                             // ?????? 0-???? 1-????
    float temp = 0.0;                                                           // ?????????? ????????
    float temp_point = 0.0;                                                     // ?????????? §³??????
    float point_bit = 1;                                                        // §³????????

    do
    {
        if(NULL == str)
        {
            break;
        }

        if('-' == *str)                                                         // ????
        {
            sign = 1;                                                           // ??????
            str ++;
        }
        else if('+' == *str)                                                    // ?????????????????
        {
            str ++;
        }

        // ???????????
        while(('0' <= *str) && ('9' >= *str))                                   // ????????????
        {
            temp = temp * 10 + ((uint8_t)(*str) - 0x30);                          // ????????????
            str ++;
        }
        if('.' == *str)
        {
            str ++;
            while(('0' <= *str) && ('9' >= *str) && point_bit < 1000000.0)      // ???????????? ????????????????¦Ë
            {
                temp_point = temp_point * 10 + ((uint8_t)(*str) - 0x30);          // ???§³?????????
                point_bit *= 10;                                                // ????????§³???????
                str ++;
            }
            temp_point /= point_bit;                                            // ????§³??
        }
        temp += temp_point;                                                     // ????????

        if(sign)
        {
            temp = -temp;
        }
    }while(0);
    return temp;
}



void func_float_to_str (char *str, float number, uint8_t point_bit)
{
//    zf_assert(str != NULL);
    int data_int = 0;                                                           // ????????
    int data_float = 0.0;                                                       // §³??????
    int data_temp[8];                                                           // ???????????
    int data_temp_point[6];                                                     // §³?????????
    uint8_t bit = point_bit;                                                      // ???????¦Ë??

    do
    {
        if(NULL == str)
        {
            break;
        }

        // ???????????
        data_int = (int)number;                                                 // ?????????? int
        if(0 > number)                                                          // ?§Ø???????????????????
        {
            *str ++ = '-';
        }
        else if(0.0 == number)                                                  // ?????? 0
        {
            *str ++ = '0';
            *str ++ = '.';
            *str = '0';
            break;
        }

        // ???§³??????
        number = number - data_int;                                             // ??????????????
        while(bit --)
        {
            number = number * 10;                                               // ???????§³??¦Ë???????????????
        }
        data_float = (int)number;                                               // ??????????

        // ???????????????
        bit = 0;
        do
        {
            data_temp[bit ++] = data_int % 10;                                  // ?????????????§Õ???????????
            data_int /= 10;
        }while(0 != data_int);
        while(0 != bit)
        {
            *str ++ = (func_abs(data_temp[bit - 1]) + 0x30);                    // ????????????§Õ??????? ??????????
            bit --;
        }

        // §³?????????????
        if(point_bit != 0)
        {
            bit = 0;
            *str ++ = '.';
            if(0 == data_float)
            {
                *str = '0';
            }
            else
            {
                while(0 != point_bit)                                           // ?§Ø???§¹¦Ë??
                {
                    data_temp_point[bit ++] = data_float % 10;                  // ????§Õ???????????
                    data_float /= 10;
                    point_bit --;                                                
                }
                while(0 != bit)
                {
                    *str ++ = (func_abs(data_temp_point[bit - 1]) + 0x30);      // ????????????§Õ??????? ??????????
                    bit --;
                }
            }
        }
    }while(0);
}


double func_str_to_double (char *str)
{
//    zf_assert(str != NULL);
    uint8_t sign = 0;                                                             // ?????? 0-???? 1-????
    double temp = 0.0;                                                          // ?????????? ????????
    double temp_point = 0.0;                                                    // ?????????? §³??????
    double point_bit = 1;                                                       // §³????????

    do
    {
        if(NULL == str)
        {
            break;
        }

        if('-' == *str)                                                         // ????
        {
            sign = 1;                                                           // ??????
            str ++;
        }
        else if('+' == *str)                                                    // ?????????????????
        {
            str ++;
        }

        // ???????????
        while(('0' <= *str) && ('9' >= *str))                                   // ????????????
        {
            temp = temp * 10 + ((uint8_t)(*str) - 0x30);                          // ????????????
            str ++;
        }
        if('.' == *str)
        {
            str ++;
            while(('0' <= *str) && ('9' >= *str) && point_bit < 1000000000.0)   // ???????????? ????????????????¦Ë
            {
                temp_point = temp_point * 10 + ((uint8_t)(*str) - 0x30);          // ???§³?????????
                point_bit *= 10;                                                // ????????§³???????
                str ++;
            }
            temp_point /= point_bit;                                            // ????§³??
        }
        temp += temp_point;                                                     // ????????

        if(sign)
        {
            temp = -temp;
        }
    }while(0);
    return temp;

}


void func_double_to_str (char *str, double number, uint8_t point_bit)
{
//    zf_assert(str != NULL);
    int data_int = 0;                                                           // ????????
    int data_float = 0.0;                                                       // §³??????
    int data_temp[12];                                                          // ???????????
    int data_temp_point[9];                                                     // §³?????????
    uint8_t bit = point_bit;                                                      // ???????¦Ë??

    do
    {
        if(NULL == str)
        {
            break;
        }

        // ???????????
        data_int = (int)number;                                                 // ?????????? int
        if(0 > number)                                                          // ?§Ø???????????????????
        {
            *str ++ = '-';
        }
        else if(0.0 == number)                                                  // ?????? 0
        {
            *str ++ = '0';
            *str ++ = '.';
            *str = '0';
            break;
        }

        // ???§³??????
        number = number - data_int;                                             // ??????????????
        while(bit --)
        {
            number = number * 10;                                               // ???????§³??¦Ë???????????????
        }
        data_float = (int)number;                                               // ??????????

        // ???????????????
        bit = 0;
        do
        {
            data_temp[bit ++] = data_int % 10;                                  // ?????????????§Õ???????????
            data_int /= 10;
        }while(0 != data_int);
        while(0 != bit)
        {
            *str ++ = (func_abs(data_temp[bit - 1]) + 0x30);                    // ????????????§Õ??????? ??????????
            bit --;
        }

        // §³?????????????
        if(point_bit != 0)
        {
            bit = 0;
            *str ++ = '.';
            if(0 == data_float)
                *str = '0';
            else
            {
                while(0 != point_bit)                                           // ?§Ø???§¹¦Ë??
                {
                    data_temp_point[bit ++] = data_float % 10;                  // ????§Õ???????????
                    data_float /= 10;
                    point_bit --;                                                
                }
                while(0 != bit)
                {
                    *str ++ = (func_abs(data_temp_point[bit - 1]) + 0x30);      // ????????????§Õ??????? ??????????
                    bit --;
                }
            }
        }
    }while(0);
}


uint32_t func_str_to_hex (char *str)
{
//    zf_assert(str != NULL);
    uint32_t str_len = strlen(str);                                               // ???????
    uint32_t result_data = 0;                                                     // ???????
    uint8_t temp = 0;                                                             // ???????
    uint8_t flag = 0;                                                             // ???¦Ë

    do
    {
        if(NULL == str)
        {
            break;
        }

        if(flag)
        {
            if(('a' <= *str) && ('f' >= *str))
            {
                temp = (*str - 87);
            }
            else if(('A' <= *str) && ('F' >= *str))
            {
                temp = (*str - 55);
            }
            else if(('0' <= *str) && ('9' >= *str))
            {
                temp = (*str - 48);
            }
            else
            {
                break;
            }
            result_data = ((result_data << 4) | (temp & 0x0F));
        }
        else
        {
//            if(strncmp("0x", str, 2))
            if((*str == '0') && (*(str + 1) == 'x'))
            {
                str ++;
                flag = 1;
            }
        }
        str ++;
    }while(str_len --);

    return result_data;
}


void func_hex_to_str (char *str, uint32_t number)
{
//    zf_assert(str != NULL);
    const char hex_index[16] = {
        '0', '1', '2', '3',
        '4', '5', '6', '7',
        '8', '9', 'A', 'B',
        'C', 'D', 'E', 'F'};
    int8_t data_temp[12];                                                         // ??????
    uint8_t bit = 0;                                                              // ????¦Ë??

    *str++ = '0';
    *str++ = 'x';
    do
    {
        if(NULL == str)
        {
            break;
        }

        if(0 == number)                                                         // ????? 0
        {
            *str = '0';
            break;
        }

        while(0 != number)                                                      // ?????????????
        {
            data_temp[bit ++] = (number & 0xF);                                 // ??????????????
            number >>= 4;                                                       // ????????????¦Ë??
        }
        while(0 != bit)                                                         // ???????????????????
        {
            *str ++ = hex_index[data_temp[bit - 1]];                            // ???????????????§Ö?????? ???????????????
            bit --;
        }
    }while(0);
}