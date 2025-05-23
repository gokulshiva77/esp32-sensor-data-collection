#ifndef __ADC_WRAPPER_H__
#define __ADC_WRAPPER_H__

#include <esp_log.h>
#include <esp_err.h>
#include "esp_adc/adc_oneshot.h"
#include "hal/adc_types.h"

// Initial ADC with unit
void adc_init(adc_unit_t unit, adc_channel_t channel);

// Read the ADC value from the specified channel
int adc_read_channel(adc_unit_t unit, adc_channel_t channel);

// Deinitialize the ADC unit and channel
void adc_deinit(adc_unit_t unit, adc_channel_t channel);

int validate_adc_channel(adc_unit_t unit, adc_channel_t channel);

#endif // __ADC_WRAPPER_H__