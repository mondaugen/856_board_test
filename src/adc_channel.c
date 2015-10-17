#include "adc_channel.h" 

static adc_channel_do_set_t *_do_sets;

void adc_channel_do_set_init(adc_channel_do_set_t *set,
                             adc_channel_t *chan,
                             adc_channel_do_func_t func,
                             adc_channel_do_data_t *do_data)
{
    set->chan = chan;
    set->func = func;
    set->data = do_data;
    set->next = NULL;
}

void adc_channel_do_set_add(adc_channel_do_set_t *set)
{
    set->next = _do_sets;
    _do_sets = set;
}

void adc_channels_update(void)
{
    adc_channel_do_set_t *set = _do_sets;
    while (set) {
        set->chan->update(set->chan);
        set = set->next;
    }
}

void adc_channel_do_all_sets(void)
{
    adc_channel_do_set_t *set = _do_sets;
    while (set) {
        adc_channel_do(set->chan,set->data,set->func);
        set = set->next;
    }
}

void adc_channel_update_avg(adc_channel_t *chan)
{
    uint32_t n, result = 0;
    for (n = 0; n < chan->n_raw_vals; n++) {
        result += chan->raw_vals[n*chan->raw_val_stride];
    }
    chan->cur_val = (adc_channel_datatype_t)(result / chan->n_raw_vals);
}

/* Inititalizes adc_channel struct. By default given the averaging update
 * function. */
void adc_channel_init(adc_channel_t *chan,
                      volatile adc_channel_datatype_t *raw_vals,
                      uint32_t raw_val_stride,
                      uint32_t nraw_vals)
{
    chan->cur_val = 0;
    chan->raw_vals = raw_vals;
    chan->raw_val_stride = raw_val_stride;
    chan->n_raw_vals = nraw_vals;
    chan->update = adc_channel_update_avg;
}

void adc_channel_setup(void)
{
    _do_sets = NULL;
}

void adc_channel_do_data_init(adc_channel_do_data_t *data,
                              adc_channel_do_style_t style,
                              adc_channel_datatype_t threshold,
                              adc_channel_datatype_t init_val)
{
    data->style = style;
    data->threshold = threshold;
    data->prev_val = init_val;
}

void adc_channel_do(adc_channel_t *chan,
                    adc_channel_do_data_t *data,
                    adc_channel_do_func_t what)
{
    switch (data->style) {
        case adc_channel_do_style_CHANGED:
            if (abs(chan->cur_val - data->prev_val) < data->threshold) {
                /* Not enough change since last time something was done */
                return;
            }
            break;
        case adc_channel_do_style_ALWAYS:
            break;
    }
    what(chan,data);
    data->prev_val = chan->cur_val;
}

