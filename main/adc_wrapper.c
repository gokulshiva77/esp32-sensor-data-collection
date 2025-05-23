#include "adc_wrapper.h"
#include "esp_log.h"

#define TAG "ADC_WRAPPER"

#define MAX_ADC_UNITS 2
#define MAX_ADC_CHANNELS 10
#define INVALID_VALUE -1
#define VALID 1

// Store handles and configs for each unit/channel
static adc_oneshot_unit_handle_t adc_handles[MAX_ADC_UNITS] = {NULL};
static adc_oneshot_unit_init_cfg_t init_configs[MAX_ADC_UNITS] = {
    { .unit_id = ADC_UNIT_1, .ulp_mode = ADC_ULP_MODE_DISABLE },
    { .unit_id = ADC_UNIT_2, .ulp_mode = ADC_ULP_MODE_DISABLE }
};
static adc_oneshot_chan_cfg_t chan_configs[MAX_ADC_UNITS][MAX_ADC_CHANNELS] = {0};
static bool channel_initialized[MAX_ADC_UNITS][MAX_ADC_CHANNELS] = {0};

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
    int unit_idx = (int)unit;
    if (validate_adc_channel(unit, channel) == INVALID_VALUE) {
        ESP_LOGE(TAG, "%s Invalid ADC unit %d or channel %d", __func__, unit, channel);
        return;
    }
    if (!adc_handles[unit_idx]) {
        ESP_LOGI(TAG, "%s ADC unit %d initialized", __func__, unit);
        esp_err_t status = adc_oneshot_new_unit(&init_configs[unit_idx], &adc_handles[unit_idx]);
        RETURN_IF_ERROR(status, "Failed to initialize ADC unit %d: %s", unit);
    }
    // Set default config for this channel only if not already initialized
    if (!channel_initialized[unit_idx][channel]) {
        chan_configs[unit_idx][channel].bitwidth = ADC_BITWIDTH_DEFAULT;
        chan_configs[unit_idx][channel].atten = ADC_ATTEN_DB_12;
        esp_err_t status = adc_oneshot_config_channel(adc_handles[unit_idx], channel, &chan_configs[unit_idx][channel]);
        RETURN_IF_ERROR(status, "Failed to configure ADC unit %d channel %d: %s", unit, channel);
        channel_initialized[unit_idx][channel] = true;
        ESP_LOGI(TAG, "%s ADC unit %d channel %d configured", __func__, unit, channel);
    }
}

int adc_read_channel(adc_unit_t unit, adc_channel_t channel)
{
    int unit_idx = (int)unit;
    if (validate_adc_channel(unit, channel) == INVALID_VALUE) {
        ESP_LOGE(TAG, "%s Invalid ADC unit %d or channel %d", __func__, unit, channel);
        return INVALID_VALUE;
    }
    if (!adc_handles[unit_idx]) {
        ESP_LOGI(TAG, "%s ADC unit %d initialized", __func__, unit);
        esp_err_t status = adc_oneshot_new_unit(&init_configs[unit_idx], &adc_handles[unit_idx]);
        RETURN_IF_ERROR_WITH_VALUE(status, INVALID_VALUE, "Failed to initialize ADC unit %d: %s", unit);
    }
    int adc_raw = INVALID_VALUE;
    esp_err_t status = adc_oneshot_read(adc_handles[unit_idx], channel, &adc_raw);
    if (status != ESP_OK) {
        ESP_LOGE(TAG, "%s Failed to read ADC unit %d channel %d: %s", __func__, unit, channel, esp_err_to_name(status));
        return INVALID_VALUE;
    }
    ESP_LOGI(TAG, "%s ADC%d Channel[%d] Raw Data: %d", __func__, unit, channel, adc_raw);
    return adc_raw;
}

void adc_deinit(adc_unit_t unit, adc_channel_t channel)
{
    int unit_idx = (int)unit;
    if (validate_adc_channel(unit, channel) == INVALID_VALUE) {
        ESP_LOGE(TAG, "%s Invalid ADC unit %d or channel %d", __func__, unit, channel);
        return;
    }
    channel_initialized[unit_idx][channel] = false;
    ESP_LOGI(TAG, "%s ADC unit %d channel %d deinitialized", __func__, unit, channel);
    // Check if any channel is still initialized for this unit
    bool any_channel_active = false;
    for (int ch = 0; ch < MAX_ADC_CHANNELS; ++ch) {
        if (channel_initialized[unit_idx][ch]) {
            any_channel_active = true;
            break;
        }
    }
    if (!any_channel_active && adc_handles[unit_idx]) {
        esp_err_t status = adc_oneshot_del_unit(adc_handles[unit_idx]);
        RETURN_IF_ERROR(status, "Failed to deinit ADC unit %d: %s", unit);
        adc_handles[unit_idx] = NULL;
        ESP_LOGI(TAG, "%s ADC unit %d fully deinitialized", __func__, unit);
    }
}

int validate_adc_channel(adc_unit_t unit, adc_channel_t channel)
{
    int unit_idx = unit;
    if (unit_idx < 0 || unit_idx >= MAX_ADC_UNITS) {
        ESP_LOGE(TAG, "%s Invalid ADC unit %d", __func__, unit);
        return INVALID_VALUE;
    }
    if (channel < 0 || channel >= MAX_ADC_CHANNELS) {
        ESP_LOGE(TAG, "%s Invalid ADC channel %d", __func__, channel);
        return INVALID_VALUE;
    }
    return VALID;
}
