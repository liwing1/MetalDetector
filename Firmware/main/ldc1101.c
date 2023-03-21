#include "ldc1101.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"

#define PIN_NOT_USED    -1

#define PIN_SPI_MISO    GPIO_NUM_12
#define PIN_SPI_MOSI    GPIO_NUM_13
#define PIN_SPI_CLK     GPIO_NUM_14
#define PIN_SPI_CS      GPIO_NUM_15

spi_device_handle_t spi_handle;

spi_bus_config_t spi_bus_config = {
    .mosi_io_num = PIN_SPI_MOSI,
    .miso_io_num = PIN_SPI_MISO,
    .sclk_io_num = PIN_SPI_CLK,
    .quadwp_io_num = PIN_NOT_USED,
    .quadhd_io_num = PIN_NOT_USED,
    .data4_io_num = PIN_NOT_USED,
    .data5_io_num = PIN_NOT_USED,
    .data6_io_num = PIN_NOT_USED,
    .data7_io_num = PIN_NOT_USED,
    .max_transfer_sz = 32,
};

spi_device_interface_config_t spi_device_interface_config = {
    .command_bits = 1,          //1:READ; 0:WRITE
    .address_bits = 7,
    .dummy_bits = 8,
    .mode = 2,                  //POL:1; PHA:0
    .duty_cycle_pos = 128,      //50%
    .clock_speed_hz = 8000000,
    .input_delay_ns = 0,
    .spics_io_num = PIN_SPI_CS,
    .pre_cb = NULL,
    .post_cb = NULL,
};

void init_spi(void)
{
    spi_bus_initialize(SPI2_HOST, &spi_bus_config, SPI_DMA_CH_AUTO);
    spi_bus_add_device(SPI2_HOST, &spi_device_interface_config, &spi_handle);
}