#ifndef __CONFIG_H__
#define __CONFIG_H__

#define USE_HEXT    TRUE

#include <stdint.h>
#include <string.h>
#include "at32f403a_407.h"
#include "at32f403a_407_int.h"

#include "base_types.h"

#include "timerx.h"
#include "adcx.h"
#include "flashx.h"
#include "key.h"
#include "spix.h"

#define UTILS_LP_FAST(value, sample, filter_constant)   (value = value*(100 - filter_constant) / 100 + sample * filter_constant / 100)
#define UTILS_LP_MOVING_AVG_APPROX(value, sample, N)    UTILS_LP_FAST(value, sample, 200 / ((N) + 1))

void delay_init(void);
void delay_us(uint32_t nus);
void delay_ms(uint16_t nms);
void delay_sec(uint16_t sec);



#endif // __CONFIG_H__