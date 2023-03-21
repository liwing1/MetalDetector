#ifndef __MY_GPIO_H__
#define __MY_GPIO_H__

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

typedef enum{
    LED_OFF = 0,
    LED_ON,
} led_level_t;

typedef enum{
    BUZZER_OFF = 0,
    BUZZER_ON,
} buzzer_level_t;

extern QueueHandle_t gpio_evt_queue;

void init_gpio(void);
void init_pwm(void);

void led_set_level(led_level_t led_level);
void buzzer_set_level(buzzer_level_t buzzer_level);

#endif