#ifndef UART_MIDI_LOWLEVEL_H
#define UART_MIDI_LOWLEVEL_H 

#include "stm32f4xx.h" 
#include "i2s_lowlevel.h" 
#include <stdint.h> 

#define MIDI_DMA_STRUCT DMA1_Stream5 

/* TODO: If midi_hw_process_input is called from the audio interrupt this needs
 * to be updated to the time between audio interrupt calls */
#define MIDI_TIMER_PERIOD_MS\
    ((1000*(CODEC_DMA_BUF_LEN/CODEC_NUM_CHANNELS))/CODEC_SAMPLE_RATE)

#define MIDI_BAUD_RATE 31250 
/* The number of bytes per midi timer period, rounded up and then multiplied by
 * two for some extra space */
#define MIDI_BUF_SIZE ((uint32_t)(2 * (MIDI_BAUD_RATE / 1000 / 8 + 1) *\
            MIDI_TIMER_PERIOD_MS))

typedef int midi_hw_err_t;

typedef int midi_hw_process_t;

/* This is just a dummy define as the setup parameters are not currenly used */
typedef int midi_hw_setup_t;

typedef midi_hw_setup_t midi_hw_cleanup_t;

#include "midi_hw.h" 

#endif /* UART_MIDI_LOWLEVEL_H */
