#ifndef AUDIO_SETUP_H
#define AUDIO_SETUP_H 
#include "i2s_lowlevel.h"

#define audio_start() audio_hw_start(NULL) 
int audio_setup(void *data);
extern int audio_ready;

#endif /* AUDIO_SETUP_H */
