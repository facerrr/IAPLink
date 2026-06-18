#ifndef __CONFIG_H__
#define __CONFIG_H__

#define USE_HEXT                TRUE

#define LED_PIN                 GPIO_PINS_13
#define LED_GPIO_PORT           GPIOC

#define IA_PIN                  GPIO_PINS_2
#define IA_GPIO_PORT            GPIOA
#define IB_PIN                  GPIO_PINS_1
#define IB_GPIO_PORT            GPIOA
#define IC_PIN                  GPIO_PINS_0
#define IC_GPIO_PORT            GPIOA

#define H1_PIN                  GPIO_PINS_10
#define H1_GPIO_PORT            GPIOA
#define H2_PIN                  GPIO_PINS_9
#define H2_GPIO_PORT            GPIOA
#define H3_PIN                  GPIO_PINS_8
#define H3_GPIO_PORT            GPIOA

#define L1_PIN                  GPIO_PINS_15
#define L1_GPIO_PORT            GPIOB
#define L2_PIN                  GPIO_PINS_14
#define L2_GPIO_PORT            GPIOB
#define L3_PIN                  GPIO_PINS_13
#define L3_GPIO_PORT            GPIOB

#define VBUS_AD_PIN             GPIO_PINS_3
#define VBUS_AD_GPIO_PORT       GPIOA

#define TEMP_AD_PIN             GPIO_PINS_0
#define TEMP_AD_GPIO_PORT       GPIOB

#define UART_TX_PIN             GPIO_PINS_10
#define UART_TX_GPIO_PORT       GPIOB
#define UART_RX_PIN             GPIO_PINS_11
#define UART_RX_GPIO_PORT       GPIOB


#define USB_BUFFER_SIZE (4096)


#define PAGE_SIZE            ((uint32_t) 0x800U) // 2KB

#define APP_MAIN_ADDR        ((uint32_t) (0x8000000 + 0 * PAGE_SIZE))       // Page 0
#define APP_BACK_ADDR        ((uint32_t) (0x8000000 + 100 * PAGE_SIZE))     // Page 100
#define APP_MAX_SIZE         ((uint32_t) (50 * PAGE_SIZE))                  // 100KB

#define BOOTLOADER_ADDR      ((uint32_t) (0x8000000 + 150 * PAGE_SIZE))     // Page 150
#define BOOTLOADER_MAX_SIZE  ((uint32_t) (10 * PAGE_SIZE))                  // 10KB

#define USR_CONFIG_ADDR      ((uint32_t) (0x8000000 + 160 * PAGE_SIZE))     // Page 160
#define USR_CONFIG_MAX_SIZE  ((uint32_t) (5 * PAGE_SIZE))                   // 5KB


#define UTILS_LP_FAST(value, sample, filter_constant)   (value = value*(100 - filter_constant) / 100 + sample * filter_constant / 100)
#define UTILS_LP_MOVING_AVG_APPROX(value, sample, N)    UTILS_LP_FAST(value, sample, 200 / ((N) + 1))


#endif  // __CONFIG_H__



