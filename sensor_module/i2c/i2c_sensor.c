#include "esp_log.h"
#include "esp_err.h"

#include "i2c_sensor.h"

#define I2C_MASTER_NUM_DEFAULT I2C_NUM_0
#define I2C_MASTER_SCL_IO_DEFAULT 22
#define I2C_MASTER_SDA_IO_DEFAULT 21
#define I2C_MASTER_FREQ_HZ_DEFAULT 100000
#define I2C_SENSOR_ADDR_DEFAULT 0x68 // Example address

#define I2C_TIMEOUT_MS 100

static const char *TAG = "i2c_sensor";

static i2c_master_bus_handle_t i2c_bus_handle = NULL;
static i2c_master_dev_handle_t i2c_dev_handle = NULL;
static uint16_t i2c_device_address = 0x00;

// Local Function Prototypes

void i2c_sensor_init(i2c_port_num_t i2c_port, gpio_num_t sda_pin_num, gpio_num_t scl_pin_num, uint16_t device_address)
{
    // Init Bus Config
    i2c_master_bus_config_t bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = i2c_port,
        .sda_io_num = sda_pin_num,
        .scl_io_num = scl_pin_num,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    // Create a new I2C Master bus
    esp_err_t ret = i2c_new_master_bus(&bus_config, &i2c_bus_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C param config failed: %s", esp_err_to_name(ret));
        return;
    }
    // Create a new I2C device
    i2c_device_address = device_address;
    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_7,
        .device_address = i2c_device_address,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ_DEFAULT,
    };
    // Add the device to the bus
    ret = i2c_master_bus_add_device(i2c_bus_handle, &dev_config, &i2c_dev_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C driver install failed: %s", esp_err_to_name(ret));
    }
}

int i2c_sensor_read(uint8_t *write_data, size_t write_len, uint8_t *read_data, size_t read_len)
{
    if(i2c_dev_handle == NULL || write_data == NULL || read_data == NULL) {
        ESP_LOGE(TAG, "Invalid parameters for I2C read");
        return EXIT_FAILURE;
    }
    esp_err_t ret = i2c_master_transmit_receive(i2c_dev_handle, write_data, write_len, read_data, read_len, I2C_TIMEOUT_MS);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C read failed: %s", esp_err_to_name(ret));
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int i2c_sensor_write(uint8_t *write_data, size_t write_len)
{
    if(i2c_dev_handle == NULL || write_data == NULL ) {
        ESP_LOGE(TAG, "Invalid parameters for I2C write");
        return EXIT_FAILURE;
    }
    esp_err_t ret = i2c_master_transmit(i2c_dev_handle, write_data, write_len, I2C_TIMEOUT_MS);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C write failed: %s", esp_err_to_name(ret));
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

void i2c_sensor_deinit(void)
{
    if(i2c_dev_handle == NULL || i2c_bus_handle == NULL) {
        ESP_LOGE(TAG, "Invalid parameters for I2C de init");
        return;
    }
    i2c_del_master_bus(i2c_bus_handle);
    i2c_dev_handle = NULL;
    i2c_bus_handle = NULL;
}

int get_i2c_sensor_data(void)
{
    // temperature read data
    uint8_t temp_write_data[2] = {MPU6050_REG_TEMP_OUT_H}; //Write temp Reg Data
    uint8_t temp_read_data[2] = {0x00, 0x00};  // Example read data

    i2c_sensor_read(temp_write_data, sizeof(temp_write_data), temp_read_data, sizeof(temp_read_data));
    int16_t temperature = (int16_t)(temp_read_data[0] << 8) | temp_read_data[1]; // Combine two bytes into one integer
    float temp = (temperature / 340.0) + 36.53; // Convert to Celsius
    ESP_LOGI(TAG, "I2C Sensor Temperature: %d %f ", temperature, temp);

    // gyro read data
    uint8_t gyro_write_data[2] = {MPU6050_REG_GYRO_XOUT_H}; //Write gyro Reg Data

    return 1; // Combine two bytes into one integer
}