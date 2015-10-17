#include "midi_setup.h" 

MIDI_Router_Standard midiRouter;

int midi_setup(void *data)
{
    MIDI_Router_Standard_init(&midiRouter);
    return(midi_hw_setup(NULL));
}

void midi_hw_process_msg(MIDIMsg *msg)
{
    MIDI_Router_handleMsg(&midiRouter.router, msg);
}
