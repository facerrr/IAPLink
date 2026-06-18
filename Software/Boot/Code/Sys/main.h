#ifndef __MAIN_H__
#define __MAIN_H__


#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include "at32f403a_407.h"
#include "at32f403a_407_int.h"

#include "config.h"
#include "utils.h"
#include "timerx.h"
#include "flashx.h"

#include "modem.h"

void SysClkInit(void);
void Clock_Config(confirm_state state);

#endif  // __MAIN_H__



