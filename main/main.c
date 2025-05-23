/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

// GPIO
#include "driver/gpio.h"
#include "hal/gpio_types.h"

// Sensors
#include "sensor_handler.h"

#define TAG "main"

#define GPIO_OUTPUT_IO    2   // GPIO 2
#define GPIO_OUTPUT_PIN_SEL  (1ULL<<GPIO_OUTPUT_IO)

#define NVS_ADC_NAMESPACE "adc_ns"
#define NVS_ADC_KEY "adc_val"

void blink_task(void *pvParameter)
{
    // Configure GPIO
    gpio_config_t io_conf = {
        .pin_bit_mask = GPIO_OUTPUT_PIN_SEL,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = 0,
        .pull_down_en = 0,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    while (1) {
        // Set GPIO ON
        gpio_set_level(GPIO_OUTPUT_IO, 1);
        vTaskDelay(1000 / portTICK_PERIOD_MS);

        // Set GPIO OFF
        gpio_set_level(GPIO_OUTPUT_IO, 0);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "%s Starting ADC Example", __func__);
    
    ESP_LOGI(TAG, "%s Starting Blink Task with FreeRTOS", __func__);
    xTaskCreate(blink_task, "blink_task", 2048, NULL, 5, NULL);

    sensor_init();
    
}
