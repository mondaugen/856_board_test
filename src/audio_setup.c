#include "audio_setup.h" 
#include "midi_setup.h" 
#include "signal_chain.h" 
#include <math.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <stdint.h> 
#include "synth_control.h" 
#include "scheduling.h" 
#include "switch_control.h" 
#include "adc_channel.h" 
#include "led_status.h" 

int audio_ready = 0;

/* Pass in the name of the audio device as a string */
int audio_setup(void *data)
{
    audio_hw_setup_t ahs;
    ahs = CODEC_SAMPLE_RATE;
    return audio_hw_setup(&ahs);
}

void audio_hw_io(audio_hw_io_t *params)
{
    int n;
    /* Process switches. */
    switch_control_do_all();
#if defined(CODEC_OUTPUT_CONSTANT)
 #define CODEC_CONSTANT 0xaaaa 
    for (n = 0; n < params->length; n++) {
        params->out[n*params->nchans_out] = CODEC_CONSTANT;
    }
#else
    for (n = 0; n < params->length; n++) {
        params->out[n*params->nchans_out] =
            params->in[n*params->nchans_in];
    }
#endif /* defined(CODEC_OUTPUT_CONSTANT) */
}
