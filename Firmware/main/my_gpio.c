#include "my_gpio.h"
#include "driver/gpio.h"
#include "driver/ledc.h"

#define PIN_STATUS_LED  GPIO_NUM_17
#define PIN_BUZZER      GPIO_NUM_21
#define PIN_BUTTON      GPIO_NUM_26

#define PIN_CLKIN       GPIO_NUM_4

QueueHandle_t gpio_evt_queue = NULL;

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

void init_gpio(void)
{
    // Config BUTTON
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_NEGEDGE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL<<PIN_BUTTON),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_ENABLE,
    };
    gpio_config(&io_conf);
    gpio_set_intr_type(PIN_BUTTON, GPIO_INTR_ANYEDGE);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(PIN_BUTTON, gpio_isr_handler, (void*) PIN_BUTTON);
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));


    // Config BUZZER
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pin_bit_mask = (1ULL<<PIN_BUZZER);
    gpio_config(&io_conf);

    // CONFIG LED
    io_conf.pin_bit_mask = (1ULL<<PIN_STATUS_LED);
    gpio_config(&io_conf);
}

void init_pwm(void)
{
    // Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_HIGH_SPEED_MODE,
        .timer_num        = LEDC_TIMER_0,
        .duty_resolution  = LEDC_TIMER_1_BIT,
        .freq_hz          = 16000000,  // Set output frequency at 16 MHz
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_HIGH_SPEED_MODE,
        .channel        = LEDC_CHANNEL_0,
        .timer_sel      = LEDC_TIMER_0,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = PIN_CLKIN,
        .duty           = 0, // Set duty to 50%
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

    // Set duty to 50%
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, 1));
    // Update duty to apply the new value
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0));
}

void led_set_level(bool led_level)
{
    gpio_set_level(PIN_STATUS_LED, led_level);
}

void buzzer_set_level(bool buzzer_level)
{
    gpio_set_level(PIN_BUZZER, buzzer_level);
}