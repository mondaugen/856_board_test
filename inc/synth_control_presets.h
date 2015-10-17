#ifndef SYNTH_CONTROL_PRESETS_H
#define SYNTH_CONTROL_PRESETS_H 

#define NUM_SYNTH_CONTROL_PRESETS 3 
void sc_presets_init(void);
void sc_presets_store(int npreset);
void sc_presets_recall(int npreset);

#endif /* SYNTH_CONTROL_PRESETS_H */
