#include <stddef.h>
#include <string.h>
#include <math.h> 
#include "audio_setup.h" 
#include "wavetables.h"
#include <stdlib.h> 
#include "mm_windows.h" 
#include "fmc.h"
#include "err.h" 

MMWavTab WaveTable;
MMWavTab soundSample;

/* Areas in memory where samples are recorded */
MMSample *sampleTableAreas[NUM_SAMPLE_TABLES];
MMWavTab    sampleTable[NUM_SAMPLE_TABLES];
size_t      soundSampleMaxLength;
WavTabAreaPair theSound;
WavTabAreaPair recordingSound;
MMSample     *hannWindowTable;
size_t       hannWindowTableLength;
size_t       zeroxSearchMaxLength;
#ifdef WAVETABLES_IN_SRAM
 #define SRAM_WAVETABLE_SIZE 32000/4
    MMSample sramSampleTableData[NUM_SAMPLE_TABLES*SRAM_WAVETABLE_SIZE]
        __attribute__((section(".big_data")));
#endif /* WAVETABLES_IN_SRAM */


void SampleTable_init(void)
{
#ifdef WAVETABLES_IN_SRAM
    int n;
    for (n = 0; n < NUM_SAMPLE_TABLES; n++) {
        sampleTable[n].samplerate = audio_hw_get_sample_rate(NULL);
        ((MMArray*)&sampleTable[n])->length = SRAM_WAVETABLE_SIZE;
        sampleTableAreas[n] = ((MMSample*)sramSampleTableData)
            + ((MMArray*)&sampleTable[n])->length*n;
        ((MMArray*)&sampleTable[n])->data = sampleTableAreas[n];
        memset(((MMArray*)&sampleTable[n])->data,0,
                sizeof(MMSample) * ((MMArray*)&sampleTable[n])->length);
    }
    theSound.wavtab = &sampleTable[0];
    theSound.area = sampleTableAreas[0];
    recordingSound.wavtab = &sampleTable[1];
    recordingSound.area   = sampleTableAreas[1];
    soundSampleMaxLength = SRAM_WAVETABLE_SIZE;
#else
    int n;
    for (n = 0; n < NUM_SAMPLE_TABLES; n++) {
        sampleTable[n].samplerate = audio_hw_get_sample_rate(NULL);
        ((MMArray*)&sampleTable[n])->length = SAMPLE_TABLE_LENGTH_SEC 
            * sampleTable[n].samplerate;
        sampleTableAreas[n] = ((MMSample*)SDRAM_BANK_ADDR)
            + ((MMArray*)&sampleTable[n])->length*n;
        if (((uint32_t)(sampleTableAreas[n]
                + ((MMArray*)&sampleTable[n])->length))
            > (SDRAM_BANK_ADDR + SDRAM_LENGTH)) {
            THROW_ERR("Sample tables do not fit in SDRAM.");
        }
        ((MMArray*)&sampleTable[n])->data = sampleTableAreas[n];
        memset(((MMArray*)&sampleTable[n])->data,0,
                sizeof(MMSample) * ((MMArray*)&sampleTable[n])->length);
    }
    theSound.wavtab = &sampleTable[0];
    theSound.area = sampleTableAreas[0];
    recordingSound.wavtab = &sampleTable[1];
    recordingSound.area   = sampleTableAreas[1];
    soundSampleMaxLength = SAMPLE_TABLE_LENGTH_SEC 
        * audio_hw_get_sample_rate(NULL);
#endif /* WAVETABLES_IN_SRAM */
}

void HannWindowTable_init(MMSample len_sec)
{
    size_t N = (MMSample)audio_hw_get_sample_rate(NULL) * len_sec;
    hannWindowTable = (MMSample*)malloc(sizeof(MMSample) * N);
    MM_hann_fill(hannWindowTable,N);
    hannWindowTableLength = N;
}

void ZeroxSearch_init(MMSample len_sec)
{
    zeroxSearchMaxLength = (MMSample)audio_hw_get_sample_rate(NULL) * len_sec;
}
