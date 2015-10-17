#include "adc_channel.h" 
#include "synth_adc_control.h" 
#include "synth_control.h"

#define SYNTH_ADC_SETUP(name,data_start_idx,func)\
    static void synth_adc_ ## name ## _control_setup(void)\
    {\
        static adc_channel_t channel;\
        static adc_channel_do_set_t do_set;\
        static adc_channel_do_data_t channel_data;\
        adc_channel_do_data_init(&channel_data,\
                                 adc_channel_do_style_CHANGED,\
                                 32,\
                                 0);\
        adc_channel_init(&channel,\
                         adc_data_starts[data_start_idx],\
                         NUM_CHANNELS_PER_ADC,\
                         ADC_AVG_SIZE);\
        adc_channel_do_set_init(&do_set,\
                                &channel,\
                                func,\
                                &channel_data);\
        adc_channel_do_set_add(&do_set);\
    }

#define SYNTH_ADC_CONTROL_FLOAT(name)\
   static void synth_adc_ ## name ## _control(adc_channel_t *chan,\
                                               adc_channel_do_data_t *data)\
    {\
         synth_control_set_ ## name (((float)chan->cur_val)/((float)ADC_MAX));\
    }


SYNTH_ADC_CONTROL_FLOAT(envelopeTime);
SYNTH_ADC_SETUP(envelopeTime,SYNTH_ADC_ENV_IDX,synth_adc_envelopeTime_control);
SYNTH_ADC_CONTROL_FLOAT(sustainTime);
SYNTH_ADC_SETUP(sustainTime,SYNTH_ADC_SUS_IDX,synth_adc_sustainTime_control);
SYNTH_ADC_CONTROL_FLOAT(offset);
SYNTH_ADC_SETUP(offset,SYNTH_ADC_OFFSET_IDX,synth_adc_offset_control);

static void synth_adc_pos_control(adc_channel_t *chan,
                                  adc_channel_do_data_t *data)
{
    switch (synth_control_get_posMode()) {
        case SynthControlPosMode_ABSOLUTE:
            synth_control_set_startPoint(((float)chan->cur_val)/((float)ADC_MAX));
            break;
        case SynthControlPosMode_UNKNOWN:
        case SynthControlPosMode_STRIDE:
            synth_control_set_positionStride(
                    ((float)chan->cur_val)/((float)ADC_MAX));
            break;
    }
}

SYNTH_ADC_SETUP(pos,SYNTH_ADC_POS_IDX,synth_adc_pos_control);

static void synth_adc_eventDelta_control(adc_channel_t *chan,
                                         adc_channel_do_data_t *data)
{
    switch (synth_control_get_deltaButtonMode()) {
        case SynthControlDeltaButtonMode_EVENT_DELTA_QUANT:
            synth_control_set_eventDelta_quant(
                    ((float)chan->cur_val)/((float)ADC_MAX));
            break;
        case SynthControlDeltaButtonMode_EVENT_DELTA_FREE:
            synth_control_set_eventDelta_free(
                    ((float)chan->cur_val)/((float)ADC_MAX));
            break;
        case SynthControlDeltaButtonMode_INTERMITTENCY:
            synth_control_set_intermittency(
                    ((float)chan->cur_val)/((float)ADC_MAX));
            break;
    }
}

SYNTH_ADC_SETUP(eventDelta,SYNTH_ADC_EVENTDELTA_IDX,
        synth_adc_eventDelta_control);

static void synth_adc_pitch_control(adc_channel_t *chan,
                                    adc_channel_do_data_t *data)
{
    switch (synth_control_get_pitchMode()) {
        case SynthControlPitchMode_CHROM:
            synth_control_set_pitch_chrom(
                    ((float)chan->cur_val)/((float)ADC_MAX));
            break;
        case SynthControlPitchMode_4TH5TH:
             synth_control_set_pitch_4ths5ths(
                    ((float)chan->cur_val)/((float)ADC_MAX));
            break;
        case SynthControlPitchMode_ARP:
            synth_control_set_pitch_arp(
                    ((float)chan->cur_val)/((float)ADC_MAX));
            break;
    }
}

SYNTH_ADC_SETUP(pitch,SYNTH_ADC_PITCH_IDX,synth_adc_pitch_control);

static void synth_adc_gain_control(adc_channel_t *chan,
                                   adc_channel_do_data_t *data)
{
    switch (synth_control_get_gainMode()) {
        case SynthControlGainMode_FADE:
            synth_control_set_fade(
                    ((float)chan->cur_val)/((float)ADC_MAX));
            break;
        case SynthControlGainMode_WET:
            synth_control_set_wet(
                    ((float)chan->cur_val)/((float)ADC_MAX));
            break;
    }
}

SYNTH_ADC_SETUP(gain,SYNTH_ADC_GAIN_IDX,synth_adc_gain_control);

static void synth_adc_tempo_control(adc_channel_t *chan,
                                   adc_channel_do_data_t *data)
{
    if (synth_control_get_editingWhichParams() == 0) {
        if (synth_control_get_noteDeltaFromBuffer()) {
            synth_control_tempoNudge(((float)chan->cur_val)/((float)ADC_MAX));
        } else {
            synth_control_set_tempo(((float)chan->cur_val)/((float)ADC_MAX));
        }
    } else {
        synth_control_set_repeats(((float)chan->cur_val)/((float)ADC_MAX));
    }
}

SYNTH_ADC_SETUP(tempo,SYNTH_ADC_TEMPO_IDX,synth_adc_tempo_control);

void synth_adc_control_setup(void)
{
    adc_channel_setup();
    synth_adc_envelopeTime_control_setup();
    synth_adc_sustainTime_control_setup();
    synth_adc_offset_control_setup();
    synth_adc_pos_control_setup();
    synth_adc_eventDelta_control_setup();
    synth_adc_pitch_control_setup();
    synth_adc_gain_control_setup();
    synth_adc_tempo_control_setup();
}

