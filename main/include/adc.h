#pragma once
#include "hal/adc_types.h"

#define ADC_UNIT ADC_UNIT_1
#define ADC_BIT_WIDTH ADC_BITWIDTH_12
#define ADC_ATTEN ADC_ATTEN_DB_12

// Чтение сырого значения ADC
int read_adc_raw(adc_channel_t channel);
float read_adc_voltage(adc_channel_t channel);
void adc_init(adc_channel_t channel);