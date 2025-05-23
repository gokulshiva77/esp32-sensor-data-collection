#include "driver/spi_master.h"
#include "esp_log.h"
#include <string.h>

#ifndef SPI_HOST
#define SPI_HOST    SPI2_HOST // Use SPI2_HOST for ESP-IDF v5+
#endif // SPI_HOST

#define PIN_NUM_MISO 19
#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK  18
#define PIN_NUM_CS   5

#define BME680_SPI_READ_MASK 0x80
#define BME680_REG_TEMP_MSB 0x22
#define BME680_REG_PRESS_MSB 0x1F
#define BME680_REG_HUM_MSB 0x25
#define BME680_REG_STATUS 0x1D

static const char *TAG = "spi_sensor";
static spi_device_handle_t spi_handle = NULL;

void spi_sensor_init(void)
{
    spi_bus_config_t buscfg = {
        .miso_io_num = PIN_NUM_MISO,
        .mosi_io_num = PIN_NUM_MOSI,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 32
    };
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 10*1000*1000, // 10 MHz for BME680
        .mode = 0,
        .spics_io_num = PIN_NUM_CS,
        .queue_size = 1,
    };
    esp_err_t ret = spi_bus_initialize(SPI_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPI bus: %s", esp_err_to_name(ret));
        return;
    }
    ret = spi_bus_add_device(SPI_HOST, &devcfg, &spi_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add SPI device: %s", esp_err_to_name(ret));
    }
}

// Read a register from BME680 over SPI
static esp_err_t bme680_spi_read_reg(uint8_t reg, uint8_t *data, size_t len)
{
    if (!spi_handle) return ESP_FAIL;
    uint8_t tx[1+len];
    uint8_t rx[1+len];
    memset(tx, 0, sizeof(tx));
    tx[0] = reg | BME680_SPI_READ_MASK;
    spi_transaction_t t = {
        .length = 8 * (1 + len),
        .tx_buffer = tx,
        .rx_buffer = rx,
    };
    esp_err_t ret = spi_device_transmit(spi_handle, &t);
    if (ret != ESP_OK) return ret;
    memcpy(data, rx + 1, len);
    return ESP_OK;
}

int spi_sensor_read(void)
{
    if (!spi_handle) {
        ESP_LOGE(TAG, "SPI device not initialized");
        return -1;
    }
    uint8_t temp_raw[3] = {0};
    esp_err_t ret = bme680_spi_read_reg(BME680_REG_TEMP_MSB, temp_raw, 3);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read BME680 temperature registers");
        return -1;
    }
    int32_t temp_adc = ((int32_t)temp_raw[0] << 12) | ((int32_t)temp_raw[1] << 4) | ((int32_t)temp_raw[2] >> 4);
    ESP_LOGI(TAG, "BME680 raw temperature ADC: %ld", temp_adc);
    // NOTE: Real temperature calculation requires calibration data and compensation (not shown here)
    return temp_adc;
}

void spi_sensor_deinit(void)
{
    if (spi_handle) {
        spi_bus_remove_device(spi_handle);
        spi_handle = NULL;
    }
    spi_bus_free(SPI_HOST);
}
