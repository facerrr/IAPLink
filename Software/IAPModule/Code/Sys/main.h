#ifndef __MAIN_H__
#define __MAIN_H__


#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include "at32f403a_407.h"
#include "at32f403a_407_int.h"

#include "base_types.h"

#include "config.h"
#include "timerx.h"
#include "adcx.h"
#include "uartx.h"
#include "flashx.h"
#include "spix.h"

#include "key.h"
#include "led.h"

#include "iap.h"
#include "recv.h"
#include "modem.h"

void SysClkInit(void);
void Clock_Config(void);

#endif  // __MAIN_H__



