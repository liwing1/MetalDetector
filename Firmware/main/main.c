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
#include "ldc1101_driver.h"

#define LDC1101_TAG "LDC1101"

#define RP_W_NO_TARGET  (float) 5.63
#define RP_W_TARGET     (float) 5.55
#define RP_TRESHOLD     (float) (RP_W_NO_TARGET - ((RP_W_NO_TARGET - RP_W_TARGET)/2))

typedef enum{
    SENSOR_OCIOSO = 0,
    SENSOR_DETECT,
} sensor_state_t;

static sensor_state_t sensor_state = SENSOR_OCIOSO;  //sensor começa desligado

void systemInit()
{
    init_gpio();    // Init buzzer, led e botao
    init_pwm();     // PWM -> CLKIN
    
    vTaskDelay(100/portTICK_PERIOD_MS);
}

void applicationInit()
{
    _LDC1101_SPI_INTERFACE spi_interface ={
        .miso_io_num = 12,
        .mosi_io_num = 13,
        .sclk_io_num = 14,
        .clk_speed_hz = 8000000,
    };
    ldc1101_spiDriverInit( (T_LDC1101_P)&spi_interface );
    ldc1101_init();

    // Valores otimizados da ref: http://www.ti.com/lit/zip/slyc137
    // RPmax = 12k; RPmin = 3k
    ldc1101_writeByte(_LDC1101_REG_CFG_RP_MEASUREMENT_DYNAMIC_RANGE,_LDC1101_RP_SET_RP_MAX_12KOhm | _LDC1101_RP_SET_RP_MIN_3KOhm);
    // C1 = 6pF; R1 = 41.63k
    ldc1101_writeByte(_LDC1101_REG_CFG_INTERNAL_TIME_CONSTANT_1,_LDC1101_TC1_C1_6pF | 0x1D);
    // C2 = 24pF; R2 = 97.5k
    ldc1101_writeByte(_LDC1101_REG_CFG_INTERNAL_TIME_CONSTANT_2,_LDC1101_TC2_C2_24pF | 0x39);
    // MINfreq = 7MHz; RESPtime = 192s
    ldc1101_writeByte(_LDC1101_REG_CFG_RP_L_CONVERSION_INTERVAL, 0xD0 | _LDC1101_DIG_CFG_RESP_TIME_192s);

    ldc1101_setPowerMode(_LDC1101_FUNC_MODE_ACTIVE_CONVERSION_MODE);
    
    ldc1101_goTo_RPmode();
    ESP_LOGW(LDC1101_TAG, "--- Start measurement ---");
}

void ldc1101_task()
{   
    uint16_t RP_Data;
    float Rp;

    while(1)
    {
        RP_Data = 0;
        Rp = 0;

        switch(sensor_state)
        {
            case SENSOR_OCIOSO:
                buzzer_set_level(BUZZER_OFF);

            break;

            case SENSOR_DETECT:
                RP_Data = ldc1101_getRPData();
                // Calcular Rp pela eq 4
                Rp = (12000 * 3000)/((12000 * (1-RP_Data/65535)) + (3000 * (RP_Data/65535)));

                buzzer_set_level(Rp > RP_TRESHOLD ? BUZZER_ON:BUZZER_OFF);
                ESP_LOGW(LDC1101_TAG, "Inductive Linear Position: %d", RP_Data);
                
            break;
        }

        vTaskDelay(100/portTICK_PERIOD_MS);
    }
}

void button_task(void* arg)
{
    uint32_t io_num;
    while(1)
    {
        // Entra neste if quando botão é precionado
        if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            
            sensor_state = !sensor_state; //toggle sensor

            led_set_level(sensor_state == SENSOR_OCIOSO ? LED_OFF:LED_ON);

            vTaskDelay(100/portTICK_PERIOD_MS);
        }
    }
}

void app_main(void)
{
    systemInit();
    applicationInit();

    xTaskCreate(button_task, "button_task", 4096, NULL, 5, NULL);
    xTaskCreate(ldc1101_task, "ldc1101_task", 4096, NULL, 10, NULL);
}
