#include <stdio.h>
#include <string.h>
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


//only pins of the lora module.
#define PIN_SPI_CS   32
#define PIN_SPI_DC   34
#define PIN_SPI_RST  35
//#define PIN_SPI_MISO 33

static const char *TAG = "LoRaTest";

spi_device_handle_t lora_spi;

void lora_spi_init() {

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 26*1000*1000,
        .mode = 0,
        .spics_io_num = PIN_SPI_CS,
        .queue_size = 7,
    };

    //ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));
    ESP_ERROR_CHECK(spi_bus_add_device(HSPI_HOST, &devcfg, &lora_spi));
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


// Cadena a enviar
char *mensaje = "Hola receptor, soy el transmisor LoRa!";

// Función para reiniciar el módulo
void lora_reset() {
    gpio_set_level(LORA_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(LORA_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(10));
}

// Función para enviar una cadena
void lora_send_packet(spi_device_handle_t spi, const char *data) {
    int length = strlen(data);
    // Set payload length
    lora_spi_transfer(spi, 0x22, length, true);

    // FIFO TX base address
    lora_spi_transfer(spi, 0x0E, 0x00, true);
    lora_spi_transfer(spi, 0x0D, 0x00, true); // FIFO addr ptr

    // Cargar datos en FIFO
    for (int i = 0; i < length; i++) {
        lora_spi_transfer(spi, 0x00, data[i], true);
    }

    // Cambiar a modo TX
    lora_spi_transfer(spi, 0x01, 0x83, true); // RegOpMode = TX

    // Esperar hasta que se haya enviado (se puede usar IRQ, pero lo hacemos simple)
    vTaskDelay(pdMS_TO_TICKS(100));
    ESP_LOGI(TAG, "Mensaje enviado: %s", data);
}

void app_main(void) {
    spi_device_handle_t spi;

    // Configurar pines CS y RST como salida
    gpio_set_direction(LORA_CS, GPIO_MODE_OUTPUT);
    gpio_set_direction(LORA_RST, GPIO_MODE_OUTPUT);

    gpio_set_level(LORA_CS, 1);
    gpio_set_level(LORA_RST, 1);

    lora_reset();
    lora_spi_init(&spi);

    // Configuración básica LoRa (modo standby, frecuencia, potencia, etc.)
    lora_spi_transfer(spi, 0x01, 0x81, true); // RegOpMode: LoRa + standby

    // Frecuencia 433 MHz (para SX1278)
    lora_spi_transfer(spi, 0x06, 0x6C, true);
    lora_spi_transfer(spi, 0x07, 0x80, true);
    lora_spi_transfer(spi, 0x08, 0x00, true);

    // Potencia de transmisión
    lora_spi_transfer(spi, 0x09, 0xFF, true); // Max power

    // Envía cada 3 segundos
    while (1) {
        lora_send_packet(spi, mensaje);
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}

