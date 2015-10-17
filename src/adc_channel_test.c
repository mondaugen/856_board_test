#include "adc.h"
#include "adc_channel_test.h" 
#include <stdint.h> 

/* A test for the adc_channel and an example of how to use it in an application
 * */

//typedef struct __adc_channel {
//    uint16_t cur_val;
//    uint16_t prev_val;
//    uint16_t[ADC_CHANNEL_RAW_VALUE_STRIDE] *raw_vals;
//    uint32_t n_raw_vals;
//    /* The function that is used to calculate cur_val from raw_vals */
//    void update (struct __adc_channel *); 
//} adc_channel_t;
//
//
//typedef struct __adc_channel_do_set {
//    adc_channel_t *chan;
//    adc_channel_do_func func;
//    adc_channel_do_data_t *data;
//    struct __adc_channel_do_set_t *next;
//} adc_channel_do_set_t;

adc_channel_t adc_test_channels[TOTAL_NUM_ADC_CHANNELS];
static adc_channel_do_set_t adc_channel_do_sets[TOTAL_NUM_ADC_CHANNELS];
adc_channel_test_do_data_t adc_channel_values[TOTAL_NUM_ADC_CHANNELS];

/* This is just a dummy function to see that the thing works. */
static void adc_channel_test_do_func(adc_channel_t *chan,
                                     adc_channel_do_data_t *data)
{
    adc_channel_test_do_data_t *true_data =
        (adc_channel_test_do_data_t*)data;
    true_data->adc_channel_value = chan->cur_val;
}

static void adc_channel_test_do_data_init(adc_channel_test_do_data_t *data)
{
    adc_channel_do_data_init((adc_channel_do_data_t*)data,
                             adc_channel_do_style_CHANGED,
                             100,
                             0);
    data->adc_channel_value = 0;
}

void adc_channel_test_setup(void)
{
    int n;
    adc_channel_setup();
    for (n = 0; n < TOTAL_NUM_ADC_CHANNELS; n++) {
        adc_channel_test_do_data_init(&adc_channel_values[n]);
        adc_channel_init(&adc_test_channels[n],
                         adc_data_starts[n],
                         NUM_CHANNELS_PER_ADC,
                         ADC_AVG_SIZE);
        adc_channel_do_set_init(&adc_channel_do_sets[n],
                                &adc_test_channels[n],
                                adc_channel_test_do_func,
                                (adc_channel_do_data_t*)&adc_channel_values[n]);
        adc_channel_do_set_add(&adc_channel_do_sets[n]);
    }
}

