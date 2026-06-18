#ifndef __LED_H__
#define __LED_H__

#include <stdint.h>
#include "at32f403a_407_gpio.h"

#define LED_IAP_ON()        gpio_bits_set(LED_GPIO_PORT, LED_PIN)

#define LED_MOD              (LEDs[0])
#define LED_DOWNLOAD         (LEDs[1])

#define LED_OFF             0
#define LED_ON              1
#define LED_FLASH           2


typedef struct{
    uint16_t FlashDelay;
    uint8_t State;
    gpio_type* Port;
    uint16_t Pin;
    uint8_t Mode;
    uint8_t PreMode;
}LedStruct;

extern LedStruct LEDs[2];

void LED_Init(void);
void LED_Tick(void);
void LED_Func(void);
unsigned char LED_Flash(uint8_t index);
void LEDState_Set(uint8_t index, uint8_t state);
void LED_File(uint8_t index);


#endif  // __LED_H__