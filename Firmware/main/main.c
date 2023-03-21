#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_log.h"

#include "my_gpio.h"

// #include "ldc1101_driver.h"

static bool sensor_on_off = 0;  //LED começa desligado

void systemInit()
{
    init_gpio();    // Init buzzer, led e botao
    init_pwm();     // PWM -> CLKIN
    
    // mikrobus_gpioInit( _MIKROBUS1, _MIKROBUS_INT_PIN, _GPIO_INPUT );
    // mikrobus_gpioInit( _MIKROBUS1, _MIKROBUS_PWM_PIN, _GPIO_INPUT );
    // mikrobus_gpioInit( _MIKROBUS1, _MIKROBUS_CS_PIN, _GPIO_OUTPUT );
    
    // mikrobus_spiInit( _MIKROBUS1, &_LDC1101_SPI_CFG[0] );
    vTaskDelay(100/portTICK_PERIOD_MS);
}

void applicationInit()
{
    // ldc1101_spiDriverInit( (T_LDC1101_P)&_MIKROBUS1_GPIO, (T_LDC1101_P)&_MIKROBUS1_SPI );
    // ldc1101_init();

    // // Configuration
    // ldc1101_writeByte(_LDC1101_REG_CFG_RP_MEASUREMENT_DYNAMIC_RANGE,_LDC1101_RP_SET_RP_MAX_24KOhm | _LDC1101_RP_SET_RP_MIN_1_5KOhm);
    // ldc1101_writeByte(_LDC1101_REG_CFG_INTERNAL_TIME_CONSTANT_1,_LDC1101_TC1_C1_0_75pF | _LDC1101_TC1_R1_21_1kOhm);
    // ldc1101_writeByte(_LDC1101_REG_CFG_INTERNAL_TIME_CONSTANT_2,_LDC1101_TC2_C2_3pF | _LDC1101_TC2_R2_30_5kOhm);
    // ldc1101_writeByte(_LDC1101_REG_CFG_RP_L_CONVERSION_INTERVAL, 0xD0 | _LDC1101_DIG_CFG_RESP_TIME_768s);
    // ldc1101_setPowerMode(_LDC1101_FUNC_MODE_ACTIVE_CONVERSION_MODE);
    
    // ldc1101_goTo_RPmode();
    ESP_LOGW("LDC1101", "--- Start measurement ---");
}

void applicationTask()
{
    // RP_Data = ldc1101_getRPData();
    // LongWordToStr(RP_Data, demoText);
    // mikrobus_logWrite(" Inductive Linear Position :", _LOG_TEXT);
    // mikrobus_logWrite(demoText, _LOG_LINE);
    // Delay_100ms();
}

void button_task(void* arg)
{
    uint32_t io_num;
    for(;;) {
        // Entra neste if quando botão é precionado
        if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            
            sensor_on_off = !sensor_on_off; //toggle sensor

            led_set_level(sensor_on_off);

            vTaskDelay(50/portTICK_PERIOD_MS);
        }
    }
}

void app_main(void)
{
    systemInit();
    applicationInit();

    xTaskCreate(button_task, "button_task", 4096, NULL, 10, NULL);
    while (1)
    {
        applicationTask();
    }
}
