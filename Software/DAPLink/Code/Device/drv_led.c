#include "drv_led.h"

void led_init(void)
{
    crm_periph_clock_enable(CRM_GPIOB_PERIPH_CLOCK, TRUE);
    gpio_init_type gpio_initstruct = {0};
    gpio_default_para_init(&gpio_initstruct);
    gpio_initstruct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER;
    gpio_initstruct.gpio_mode = GPIO_MODE_OUTPUT;
    gpio_initstruct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
    gpio_initstruct.gpio_pins = GPIO_PINS_15 | GPIO_PINS_14 | GPIO_PINS_13 | GPIO_PINS_8;
    gpio_initstruct.gpio_pull = GPIO_PULL_UP;
    gpio_init(GPIOB, &gpio_initstruct);

    gpio_bits_reset(GPIOB, GPIO_PINS_15 | GPIO_PINS_14 | GPIO_PINS_13 | GPIO_PINS_8);
}
