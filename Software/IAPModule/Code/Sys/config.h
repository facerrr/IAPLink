#ifndef __CONFIG_H__
#define __CONFIG_H__

#define USE_HEXT                TRUE

#define LED_PIN                 GPIO_PINS_9
#define LED_GPIO_PORT           GPIOB

#define LED_MOD_PIN             GPIO_PINS_3
#define LED_MOD_GPIO_PORT       GPIOA

#define LED_DOWNLOAD_PIN        GPIO_PINS_4
#define LED_DOWNLOAD_GPIO_PORT  GPIOA

#define USB_BUFFER_SIZE         (4096)


#define RECV_NORMAL             0   
#define RECV_ZSERIES_IAP        1
#define RECV_XSERIES_IAP        2

#define ZSERIES                 0
#define XSERIES                 1


#define UTILS_LP_FAST(value, sample, filter_constant)   (value = value*(100 - filter_constant) / 100 + sample * filter_constant / 100)
#define UTILS_LP_MOVING_AVG_APPROX(value, sample, N)    UTILS_LP_FAST(value, sample, 200 / ((N) + 1))


#endif  // __CONFIG_H__



