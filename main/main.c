/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

// ADC
#include "adc_wrapper.h"

// GPIO
#include "driver/gpio.h"
#include "hal/gpio_types.h"

// Storage
#include "storage_handler.h"

#define TAG "main"

#define GPIO_OUTPUT_IO    2   // GPIO 2
#define GPIO_OUTPUT_PIN_SEL  (1ULL<<GPIO_OUTPUT_IO)

#define NVS_ADC_NAMESPACE "adc_ns"
#define NVS_ADC_KEY "adc_val"

void adc_task(void *pvParameter)
{
    adc_unit_t unit = ADC_UNIT_1;
    adc_channel_t channel = ADC_CHANNEL_0; //GPIO 36

    adc_init(unit, channel);
    while (1) {
        int value = adc_read_channel(unit, channel);
        if (value < 0) {
            ESP_LOGE(TAG, "%s Failed to read ADC value for Unit - %d, Channel - %d", __func__, unit, channel);
        } else {
            ESP_LOGI(TAG, "%s ADC Unit - %d, Channel [%d] : %d", __func__, unit, channel, value);
            
            // Write ADC value to storage handler (queue-based)
            esp_err_t err = storage_handler_write(NVS_ADC_NAMESPACE, NVS_ADC_KEY, value);
            if (err == ESP_OK) {
                ESP_LOGI(TAG, "%s Stored ADC value %d in NVS", __func__, value);
            } else {
                ESP_LOGE(TAG, "%s Failed to store ADC value in NVS", __func__);
            }

            // Read back the value from storage handler
            int32_t read_value = 0;
            err = storage_handler_read(NVS_ADC_NAMESPACE, NVS_ADC_KEY, &read_value);
            if (err == ESP_OK) {
                ESP_LOGI(TAG, "%s Read ADC value from NVS: %ld", __func__, read_value);
            } else {
                ESP_LOGE(TAG, "%s Failed to read ADC value from NVS", __func__);
            }
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

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

    ESP_LOGI(TAG, "%s Starting Storage Handler Task", __func__);
    storage_handler_start();
    
    ESP_LOGI(TAG, "%s Starting Blink Task with FreeRTOS", __func__);
    xTaskCreate(blink_task, "blink_task", 2048, NULL, 5, NULL);

    ESP_LOGI(TAG, "%s Starting ADC Task with FreeRTOS", __func__);
    xTaskCreate(adc_task, "adc_task", 2048, NULL, 5, NULL);
}
