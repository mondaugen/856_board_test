#include "synth_control.h" 
#include "presets_lowlevel.h"
#include <string.h> 


typedef struct __SCPreset {
    NoteParamSet                 noteParamSets[NUM_NOTE_PARAM_SETS];
    MMSample                     tempoBPM; 
    SynthControlPosMode          posMode;
    SynthControlDeltaButtonMode  deltaButtonMode;
    SynthControlPitchMode        pitchMode;
    SynthControlGainMode         gainMode;
} SCPreset;

static presets_lowlevel_handle_t *scpresets_handle;
static SCPreset scpresets[NUM_SYNTH_CONTROL_PRESETS];

/* File is a file in which to store presets. This will obviously depend on the
 * implementation. */
void sc_presets_init(void)
{
    /* (The flash implementation requires no initialization) */
    presets_lowlevel_init(&scpresets_handle,NULL);
    presets_lowlevel_read(scpresets_handle,(void*)scpresets,
            sizeof(scpresets),NULL);
}

void sc_presets_store(int npreset)
{
    memcpy(&scpresets[npreset].noteParamSets,noteParamSets,
            sizeof(NoteParamSet)*NUM_NOTE_PARAM_SETS);
    scpresets[npreset].tempoBPM = tempoBPM; 
    scpresets[npreset].posMode = posMode;
    scpresets[npreset].deltaButtonMode = deltaButtonMode;
    scpresets[npreset].pitchMode = pitchMode;
    scpresets[npreset].gainMode = gainMode;
    /* Store every time, because program could be terminated at any moment */
    presets_lowlevel_write(scpresets_handle,(void*)scpresets,
            sizeof(scpresets),NULL);
}

void sc_presets_recall(int npreset)
{
    /* No need to load from file, this is done only on initialization */
    memcpy(noteParamSets,&scpresets[npreset].noteParamSets,sizeof(NoteParamSet)*NUM_NOTE_PARAM_SETS);
    if (schedulerState == 0) {
        /* Only recall tempo if sequencer not playing */
        tempoBPM = scpresets[npreset].tempoBPM; 
    }
    posMode = scpresets[npreset].posMode;
    deltaButtonMode = scpresets[npreset].deltaButtonMode;
    pitchMode = scpresets[npreset].pitchMode;
    gainMode = scpresets[npreset].gainMode;
}
