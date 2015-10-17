#include "err.h" 
#include "audio_setup.h"
#include "midi_setup.h" 
#include "wavetables.h" 
#include "signal_chain.h" 
#include "synth_control.h" 
#include "synth_control_presets.h" 
#include "scheduling.h" 
#include "leds.h" 
#include "switches.h" 
#include "adc.h" 
#include "leds.h" 
#include "switch_control.h" 
#include "synth_switch_control.h" 
#include "adc_channel.h" 
#include "synth_adc_control.h" 

#define INITIAL_COUNT 1000000L 

int main (void)
{
    if (audio_setup(NULL)) {
        THROW_ERR("Error setting up audio.");
    }
    audio_start();
    while(1) {
    }
}
