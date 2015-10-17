#ifndef I2S_LOWLEVEL_H
#define I2S_LOWLEVEL_H

#include <stdint.h>
#include <stddef.h> 

//#define CODEC_DMA_HARDFAULT_ON_I2S_ERR 
#define CODEC_DMA_DIRECT_MODE 

#ifdef  TEST_CODEC_UNDERRUN
 #define CODEC_SAMPLE_RATE 44100//16000//44100//32000//
#else
 #define CODEC_SAMPLE_RATE 32000//44100//16000//44100//32000//
#endif

#define CODEC_NUM_CHANNELS 2

#define AUDIO_HW_SAMPLE_T_MAX 32767 

typedef int16_t audio_hw_sample_t;
typedef int     audio_hw_err_t;

typedef uint32_t audio_hw_setup_t;

/* number of items in one half of the buffer (because we do our own double
 * buffering on a single array, addressing with no offset or an offset of one
 * half the total length of the array. */
#ifdef TEST_CODEC_UNDERRUN
 #define CODEC_DMA_BUF_LEN 128
#else
 #define CODEC_DMA_BUF_LEN 512
#endif

#define UINT16_TO_FLOAT(x) ((float)((int32_t)x - 0x8000)/((float)0x8000))
#define FLOAT_TO_UINT16(x) ((uint16_t)((x + 1.) * 0x8000))
#define INT16_TO_FLOAT(x) ((float)x/(float)32768)
#define FLOAT_TO_INT16(x) ((int16_t)(x * 32768))

#include "audio_hw.h" 

#endif /* I2S_LOWLEVEL_H */
