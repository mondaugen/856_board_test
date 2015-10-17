#ifndef MIDI_SETUP_H
#define MIDI_SETUP_H 
#include "uart_midi_lowlevel.h"
#include "mm_midirouter_standard.h" 

extern MIDI_Router_Standard midiRouter;

int midi_setup(void *data);

#endif /* MIDI_SETUP_H */
