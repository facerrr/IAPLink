#include "main.h"
#include "easy_ui.h"

LedStruct LEDs[2];
void LEDState_Set(uint8_t index, uint8_t state);

void LED_Init(void){
    gpio_init_type gpio_initstruct = {0};
    gpio_default_para_init(&gpio_initstruct);
    gpio_initstruct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER;
    gpio_initstruct.gpio_mode = GPIO_MODE_OUTPUT;
    gpio_initstruct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
    gpio_initstruct.gpio_pins = LED_PIN | GPIO_PINS_13 | GPIO_PINS_14 | GPIO_PINS_15;
    gpio_initstruct.gpio_pull = GPIO_PULL_UP;
    gpio_init(GPIOB, &gpio_initstruct);

    gpio_initstruct.gpio_pins = LED_MOD_PIN | LED_DOWNLOAD_PIN;
    gpio_init(GPIOA, &gpio_initstruct);

    gpio_bits_reset(GPIOB, LED_PIN | GPIO_PINS_13 | GPIO_PINS_14 | GPIO_PINS_15);
    gpio_bits_reset(GPIOA, LED_MOD_PIN | LED_DOWNLOAD_PIN);

    memset(LEDs, 0, sizeof(LEDs));

    LED_MOD.Port = GPIOA;
    LED_MOD.Pin = LED_MOD_PIN;

    LED_DOWNLOAD.Port = GPIOA;
    LED_DOWNLOAD.Pin = LED_DOWNLOAD_PIN;
}


void LED_Tick(void){
    for(uint8_t i = 0; i < 2; i++){
        if(LEDs[i].FlashDelay < 2000){
            LEDs[i].FlashDelay++;
        }
    }
}


void LED_Func(void){
    for (uint8_t i = 0; i < 2; i++){
        if(LEDs[i].Mode != LEDs[i].PreMode){
            LEDs[i].PreMode = LEDs[i].Mode;
            LEDs[i].FlashDelay = 0;
        }
        switch (LEDs[i].Mode){
        case LED_OFF:
            LEDState_Set(i, 0);
            break;
        case LED_ON:
            LEDState_Set(i, 1);
            break;
        case LED_FLASH:
            LED_Flash(i);
            break;
        default:
            LEDState_Set(i, 0);
            break;
        }
    }
}


unsigned char LED_Flash(uint8_t index){
    static uint8_t i;
    char* msg;
    if(LEDs[index].FlashDelay <  80){
        LEDState_Set(index, 1);
    }else if(LEDs[index].FlashDelay <  120){
        LEDState_Set(index, 0);
    }else{
        LEDs[index].FlashDelay = 0;
        return 1;
    }
    return 0;
}


void LEDState_Set(uint8_t index, uint8_t state){
    if(LEDs[index].State != state){
        LEDs[index].State = state;
        if(state == 0){
            gpio_bits_reset(LEDs[index].Port, LEDs[index].Pin);
        }else{
            gpio_bits_set(LEDs[index].Port, LEDs[index].Pin);
        }
    }
}


void LED_File(uint8_t index){
    switch (index){
    case 0:
        gpio_bits_reset(GPIOB, GPIO_PINS_14 | GPIO_PINS_15);
        gpio_bits_set(GPIOB, GPIO_PINS_13);
        break;
    case 1:
        gpio_bits_reset(GPIOB, GPIO_PINS_13 | GPIO_PINS_15);
        gpio_bits_set(GPIOB, GPIO_PINS_14);
        break;
    case 2:
        gpio_bits_set(GPIOB, GPIO_PINS_13 | GPIO_PINS_14);
        gpio_bits_reset(GPIOB, GPIO_PINS_15);
        break;
    case 3:
        gpio_bits_reset(GPIOB, GPIO_PINS_13 | GPIO_PINS_14);
        gpio_bits_set(GPIOB, GPIO_PINS_15);
        break;
    default:
        break;
    }
}