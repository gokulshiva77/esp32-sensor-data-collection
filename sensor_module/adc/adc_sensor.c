#include "adc_sensor.h"
#include "esp_log.h"

#define TAG "ADC_SENSOR"

#define INVALID_VALUE -1
#define VALID 1

// Store handles and configs for each unit/channel
static adc_oneshot_unit_handle_t adc_handle = NULL;
static adc_oneshot_unit_init_cfg_t init_config = { .unit_id = ADC_UNIT_1, .ulp_mode = ADC_ULP_MODE_DISABLE };
static adc_oneshot_chan_cfg_t chan_config = {0};
static bool channel_initialized = false;

static adc_unit_t adc_unit = ADC_UNIT_1; // Default unit
static adc_channel_t adc_channel = ADC_CHANNEL_0; // Default channel

#define RETURN_IF_ERROR(status, fmt, ...) \
    do { \
        if ((status) != ESP_OK) { \
            ESP_LOGE(TAG, "%s " fmt, __func__, ##__VA_ARGS__, esp_err_to_name(status)); \
            return; \
        } \
    } while (0)

#define RETURN_IF_ERROR_WITH_VALUE(status, value, fmt, ...) \
    do { \
        if ((status) != ESP_OK) { \
            ESP_LOGE(TAG, "%s " fmt, __func__, ##__VA_ARGS__, esp_err_to_name(status)); \
            return value; \
        } \
    } while (0)

void adc_init(adc_unit_t unit, adc_channel_t channel)
{
    if (validate_adc_channel(unit, channel) == INVALID_VALUE) {
        ESP_LOGE(TAG, "%s Invalid ADC unit %d or channel %d", __func__, unit, channel);
        return;
    }
    adc_unit = unit;
    adc_channel = channel;

    if (!adc_handle) {
        ESP_LOGI(TAG, "%s ADC unit %d initialized", __func__, unit);
        esp_err_t status = adc_oneshot_new_unit(&init_config, &adc_handle);
        RETURN_IF_ERROR(status, "Failed to initialize ADC unit %d: %s", unit);
    }
    if (!channel_initialized) {
        chan_config.bitwidth = ADC_BITWIDTH_12;
        chan_config.atten = ADC_ATTEN_DB_0;
        esp_err_t status = adc_oneshot_config_channel(adc_handle, channel, &chan_config);
        RETURN_IF_ERROR(status, "Failed to configure ADC unit %d channel %d: %s", unit, channel);
        channel_initialized = true;
        ESP_LOGI(TAG, "%s ADC unit %d channel %d configured", __func__, unit, channel);
    }
}

int adc_read_channel(adc_unit_t unit, adc_channel_t channel)
{
    if (validate_adc_channel(unit, channel) == INVALID_VALUE) {
        ESP_LOGE(TAG, "%s Invalid ADC unit %d or channel %d", __func__, unit, channel);
        return INVALID_VALUE;
    }
    if (!adc_handle) {
        ESP_LOGI(TAG, "%s ADC unit %d initialized", __func__, unit);
        esp_err_t status = adc_oneshot_new_unit(&init_config, &adc_handle);
        RETURN_IF_ERROR_WITH_VALUE(status, INVALID_VALUE, "Failed to initialize ADC unit %d: %s", unit);
    }
    int adc_raw = INVALID_VALUE;
    esp_err_t status = adc_oneshot_read(adc_handle, channel, &adc_raw);
    if (status != ESP_OK) {
        ESP_LOGE(TAG, "%s Failed to read ADC unit %d channel %d: %s", __func__, unit, channel, esp_err_to_name(status));
        return INVALID_VALUE;
    }
    ESP_LOGI(TAG, "%s ADC%d Channel[%d] Raw Data: %d", __func__, unit, channel, adc_raw);
    return adc_raw;
}

void adc_deinit(adc_unit_t unit, adc_channel_t channel)
{
    if (validate_adc_channel(unit, channel) == INVALID_VALUE) {
        ESP_LOGE(TAG, "%s Invalid ADC unit %d or channel %d", __func__, unit, channel);
        return;
    }
    channel_initialized = false;
    ESP_LOGI(TAG, "%s ADC unit %d channel %d deinitialized", __func__, unit, channel);
    if (adc_handle) {
        esp_err_t status = adc_oneshot_del_unit(adc_handle);
        RETURN_IF_ERROR(status, "Failed to deinit ADC unit %d: %s", unit);
        adc_handle = NULL;
        ESP_LOGI(TAG, "%s ADC unit %d fully deinitialized", __func__, unit);
    }
}

int validate_adc_channel(adc_unit_t unit, adc_channel_t channel)
{
    if ((unit < ADC_UNIT_1) || (unit > ADC_UNIT_2)) {
        ESP_LOGE(TAG, "%s Invalid ADC unit %d", __func__, unit);
        return INVALID_VALUE;
    }
    if ((channel < ADC_CHANNEL_0) || (channel > ADC_CHANNEL_9)) {
        ESP_LOGE(TAG, "%s Invalid ADC channel %d", __func__, channel);
        return INVALID_VALUE;
    }
    return VALID;
}

int get_adc_sensor_data(void)
{
    int adc_value = adc_read_channel(adc_unit, adc_channel);
    if (adc_value == INVALID_VALUE) {
        ESP_LOGE(TAG, "%s Failed to read ADC sensor data", __func__);
    }
    return adc_value;
}
