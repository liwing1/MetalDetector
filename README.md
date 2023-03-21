# Processo Seletivo HealthGo - Projeto Detector de Metais

# Objetivo

Projetar um detector de metais para garantir a segurança dos pacientes antes de exames.

Condições:

    - Chave liga/desliga;
    - LED de indicação de alimentação do sistema;
    - Botão para habilitar/desabilitar detecção;
    - LED de indicção de detecção habilitada;
    - Buzzer de sinalização na presença de metal.

# Hardware

O primeiro passo para a realização do projeto é montar um diagrama em alto nível de abstração:

![diagrama](https://i.imgur.com/TwxJRvM.png)

Com essas informações já é possível construir uma lista de componentes que será necessário para o funcionamento de cada bloco.

## 1. Regulação de Tensão

Tendo em vista que todos os componentes dependem de redução da tensão para funcionarem, foi primeiramente projetado o regulador de tensão.

![power](https://i.imgur.com/LNw0EGw.png)

Optou-se pela utilização do LM296 devido a sua grande disponibilidade no mercado e alta performance. Vale ainda salientar que o LED de indicação de alimentação foi posicionado após a regulação da tensão. Assim, fica evidente que os demais componentes estão sendo alimentado corretamente.

## 2. Microcontrolador

A tarefa de ler os registros de um sensor através de comunicação serial é muito simples. No entanto, por se tratar da segurança de pacientes, deve-se ser cauteloso com a escolha do processador.

![uc](https://i.imgur.com/0ShyawA.png)

O ESP32-WROOM-32 foi escolhido por sua robustez tanto no hardware quanto no software.

## 3. Sensor 

O sensor indutivo foi previamente definido, incluindo a impendância indutiva que é acoplada ao chip. Para que seja atingida a frequência de ressonância de 3MHz no tanque LC, a capacitância adicionada em paralelo deve ser calculada através da equação 1 fornecida no datasheet do fabricante:

![fsensor](https://i.imgur.com/jfGsVyS.png)

O valor obtido foi de 390.89 pF. Entretando, o valor comercial mais próximo é de 390 pF, que está dentro da faixa de erro do próprio capacitor.

![sensor](https://i.imgur.com/o50XD2u.png)

Nota-se ainda que além da comunicação serial com o microcontrolador, é necessário também o fornecimento de um sinal de clock externo que serve como referência interna para a amostragem do sinal (a recomendação do fabricante é 16MHz).

# Firmware

Da mesma forma que o hardware é separado de forma modular através de blocos funcionais, é interessante que o software mantenha a mesma estrutura. Assim, o código pode ser reaproveitado com mais facilidade.

Ademais, apesar de o Arduino ser uma das plataformas de desenvolvimento mais populares, optou-se pela utilização do framework oficial da Espressif, o ESP-IDF. Neste ambiente a inclusão de bibliotecas é mais simples graças ao CMake. Além disso a API permite um controle mais preciso sobre os periféricos.

## 1. Porte da Lib

Para importar a biblioteca fornecida ao ESP-IDF foi necessário transforma-la em um _component_ através do arquivo CMakeLists.txt:
```C
idf_component_register(SRCS "ldc1101_driver.c" "ldc1101_hal.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES driver)
```

Após esse passo o biblioteca ja passa a ser compilável. No entanto, por ser um código genérico para qualquer arquitetura, é necessário adicionar a camada de abstração de hardware (HAL) específica do ESP.

```C
void hal_spiMap(T_HAL_P spiObj)
{
    T_HAL_SPI_OBJ tmp = (T_HAL_SPI_OBJ)spiObj;

    spi_bus_config_t spi_bus_config = {
        .mosi_io_num = tmp->mosi_io_num,
        .miso_io_num = tmp->miso_io_num,
        .sclk_io_num = tmp->sclk_io_num,
        .quadwp_io_num = PIN_NOT_USED,
        .quadhd_io_num = PIN_NOT_USED,
        .max_transfer_sz = 32,
    };

    spi_device_interface_config_t spi_device_interface_config = {
        .mode = 2,                  //POL:1; PHA:0
        .clock_speed_hz = tmp->clk_speed_hz,
        .queue_size = 1,
        .spics_io_num = PIN_NOT_USED,
        .pre_cb = NULL,
        .post_cb = NULL,
    };

    spi_bus_initialize(SPI2_HOST, &spi_bus_config, SPI_DMA_CH_AUTO);
    spi_bus_add_device(SPI2_HOST, &spi_device_interface_config, &spi_handle);
}

void hal_spiWrite(uint8_t *pBuf, uint16_t nBytes)
{
    spi_transaction_t transaction = {
        .tx_buffer = pBuf,
        .length = nBytes
    };
    spi_device_polling_transmit(spi_handle, &transaction);
}

void hal_spiRead(uint8_t *pBuf, uint16_t nBytes)
{
    spi_transaction_t transaction = {
        .rx_buffer = pBuf,
        .length = nBytes
    };
    spi_device_polling_transmit(spi_handle, &transaction);
}
```

## 2. Inicialização
Depois de inicializar os periféricos de interação com o usuário e o clock externo, o ESP pode finalmente se comunicar com o sensor. Utilizando-se da biblioteca fornecida pela MIKROE, a configuração e leitura do sensor fica muito simplificada.

```C
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
```

## 3. Laço Principal

Graças ao FreeRTOS foi possível subdividir o programa em duas tarefas que são processadas de forma independente.

```C
void button_task(void* arg)
{
    uint32_t io_num;
    while(1)
    {
        // Entra neste if quando botão é precionado
        if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            
            sensor_state = !sensor_state; //toggle sensor

            led_set_level(sensor_state == SENSOR_OCIOSO ? LED_OFF:LED_ON);
        }
        vTaskDelay(100/portTICK_PERIOD_MS);
    }
}
```

Essa primeira tarefa monitora o status da fila, que só é preenchida quando o botão é pressionado, o que aciona a interrupção:

```C
static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}
```

Assim, a indicação do LED é habilitada/desabilitada e o estado do sensor também é trocado.

```C
void ldc1101_task()
{   
    while(1)
    {
        uint16_t RP_Data = 0;
        float Rp = 0;

        switch(sensor_state)
        {
            case SENSOR_OCIOSO:
                buzzer_set_level(BUZZER_OFF);

            break;

            case SENSOR_DETECT:
                RP_Data = ldc1101_getRPData();
                // Calcular Rp pela eq 4
                Rp = (12000 * 3000)/((12000 * (1-RP_Data/65535)) + (3000 * (RP_Data/65535)));

                buzzer_set_level(Rp < RP_TRESHOLD ? BUZZER_ON:BUZZER_OFF);
                ESP_LOGW(LDC1101_TAG, "Inductive Linear Position: %d", RP_Data);
                
            break;
        }

        vTaskDelay(100/portTICK_PERIOD_MS);
    }
}
```

É nesta task que ocorre a leitura do sensor de fato. Para determinar se o metal foi detectado estimou-se o valor máximo de Rp quando p sensor está interagindo com algum material condutor. Apartir disso definiu-se que o treshold é o valor intermediário entre o Rp máximo e o Rp mínimo. 

Mas é importante ressaltar que apesar de esses parâmetros serem fornecidos pelo fábricante, não se deve deixar de realizar ajustes práticos. Pois capacitancias parasitas e variação da temperatura podem afetar drasticamente as medições.