#ifndef __DRV_LED_H
#define __DRV_LED_H

#include "config.h"

#define LED_MODE_ON() gpio_bits_set(GPIOB, GPIO_PINS_8);
#define LED_PWR_ON()  gpio_bits_set(GPIOB, GPIO_PINS_15)
#define LED_CON_ON()  gpio_bits_set(GPIOB, GPIO_PINS_14)
#define LED_USR_ON()  gpio_bits_set(GPIOB, GPIO_PINS_13)
#define LED_OFF()    gpio_bits_reset(GPIOB, GPIO_PINS_15 | GPIO_PINS_14 | GPIO_PINS_13)

#define LED_PWR_STATE() gpio_input_data_bit_read(GPIOB, GPIO_PINS_15);
#define LED_CON_STATE() gpio_input_data_bit_read(GPIOB, GPIO_PINS_14);
#define LED_USB_STATE() gpio_input_data_bit_read(GPIOB, GPIO_PINS_13);

void led_init(void);

#endif // __DRV_LED_H
