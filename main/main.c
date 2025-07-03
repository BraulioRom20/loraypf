#include <stdio.h>
#include <string.h>
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define PIN_SPI_MOSI 21
#define PIN_SPI_CLK  28
#define PIN_SPI_CS   32
#define PIN_SPI_DC   34
#define PIN_SPI_RST  35
#define PIN_SPI_MISO 33

static const char *TAG = "LoRaTest";

spi_device_handle_t lora_spi;

void lora_spi_init() {
    spi_bus_config_t buscfg = {
        .mosi_io_num = PIN_SPI_MOSI,
        .miso_io_num = PIN_SPI_MISO,
        .sclk_io_num = PIN_SPI_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1
    };

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 26*1000*1000,
        .mode = 0,
        .spics_io_num = PIN_SPI_CS,
        .queue_size = 7,
    };

    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));
    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &devcfg, &lora_spi));
}

uint8_t lora_read_register(uint8_t reg) {
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    uint8_t tx_data[2] = { reg & 0x7F, 0x00 };
    uint8_t rx_data[2];

    t.length = 2 * 8;
    t.tx_buffer = tx_data;
    t.rx_buffer = rx_data;

    spi_device_transmit(lora_spi, &t);
    return rx_data[1];
}

void app_main(void) {
    lora_spi_init();
    vTaskDelay(pdMS_TO_TICKS(100)); 

    uint8_t version = lora_read_register(0x42); 
    ESP_LOGI(TAG, "LoRa chip version: 0x%02X", version);
}
