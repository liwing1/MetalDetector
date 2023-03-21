#ifndef __MY_GPIO_H__
#define __MY_GPIO_H__

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

extern QueueHandle_t gpio_evt_queue;

void init_gpio(void);
void init_pwm(void);

void led_set_level(bool led_level);
void buzzer_set_level(bool buzzer_level);

#endif