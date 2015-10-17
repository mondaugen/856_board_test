#ifndef ADC_CHANNEL_TEST_H
#define ADC_CHANNEL_TEST_H 
#include <stdint.h> 
#include "adc_channel.h" 

typedef struct __adc_channel_test_do_data_t {
    adc_channel_do_data_t head;
    uint16_t adc_channel_value;
} adc_channel_test_do_data_t;

extern adc_channel_t adc_test_channels[];
extern adc_channel_test_do_data_t adc_channel_values[];

void adc_channel_test_setup(void);

#endif /* ADC_CHANNEL_TEST_H */
