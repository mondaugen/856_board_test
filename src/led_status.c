#include "synth_control.h" 
#include "leds.h"

void led_status_update(void)
{
   if (synth_control_get_recordState()) {
      led1_set();
   } else {
      led1_reset();
   }
   if (synth_control_get_schedulerState()) {
       led3_set();
   } else {
       led3_reset();
   }
   if (synth_control_get_feedbackState()) {
       led5_set();
   } else {
       led5_reset();
   }
}

