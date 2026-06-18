#ifndef __FUNCTION_H__
#define __FUNCTION_H__

#include <stdint.h>
#include <string.h>


#define func_abs(x) ((x) >= 0 ? (x): -(x))


int32_t func_str_to_int (char *str);
void func_int_to_str (char *str, int32_t number);
uint32_t func_str_to_uint (char *str);
void func_uint_to_str (char *str, uint32_t number);
float func_str_to_float (char *str);
void func_float_to_str (char *str, float number, uint8_t point_bit);
double func_str_to_double (char *str);
void func_double_to_str (char *str, double number, uint8_t point_bit);
uint32_t func_str_to_hex (char *str);
void func_hex_to_str (char *str, uint32_t number);


#endif // __FUNCTION_H__