#ifndef SIGNAL_CHAIN_H
#define SIGNAL_CHAIN_H 
#include "mm_bus.h"
#include "mm_trapenvedsampleplayer.h" 
#include "mm_sigchain.h" 
#include "mm_wavtab_recorder.h" 
#include "mm_bussplitter.h"
#include "mm_busmerger.h" 
#include "mm_envelope.h" 

#define NUM_NOTES 8 

extern MMBus *inBus, *outBus, *fbBus;
extern MMSigChain sigChain;
extern MMTrapEnvedSamplePlayer spsps[NUM_NOTES];
extern MMWavTabRecorder wtr;
extern MMBusSplitter fbBusSplitter;
/* Insert the fbBusSplitter after this node to turn it off */
extern MMSigProc *fbOffNode; 
/* Insert the fbBusSplitter after this node to turn it on */
extern MMSigProc *fbOnNode;

void signal_chain_setup(void);

#endif /* SIGNAL_CHAIN_H */
