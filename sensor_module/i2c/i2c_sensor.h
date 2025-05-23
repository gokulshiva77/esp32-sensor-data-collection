#ifndef __I2C_SENSOR_H__
#define __I2C_SENSOR_H__

#include "driver/i2c_master.h"
#include "mpu6050.h"

void i2c_sensor_init(i2c_port_num_t i2c_port, gpio_num_t sda_pin_num, gpio_num_t scl_pin_num, uint16_t device_address);
int i2c_sensor_write(uint8_t *write_data, size_t write_len);
int i2c_sensor_read(uint8_t *write_data, size_t write_len, uint8_t *read_data, size_t read_len);
void i2c_sensor_deinit(void);

int get_i2c_sensor_data(void);
#endif // __I2C_SENSOR_H__
