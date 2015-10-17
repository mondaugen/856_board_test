#include "uart_midi_lowlevel.h"
#include "system_init.h" 

static char midiBuffer[MIDI_BUF_SIZE];
static int MIDIlastIndex = 0;

midi_hw_err_t midi_hw_setup(midi_hw_setup_t *params)
{
    /* Enable clocks */
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN | RCC_AHB1ENR_DMA1EN;
    /* Setup GPIOD6
     * Alternate function = UASRT2 (AF7),
     * no pull
     * fast speed
     *  */
    GPIOD->AFR[0] &= ~(0xf << (4*6));
    GPIOD->AFR[0] |= (0x7 << (4*6));
    GPIOD->MODER &= ~GPIO_MODER_MODER6;
    GPIOD->MODER |= 0x2 << (2*6);
    GPIOD->PUPDR &= ~GPIO_PUPDR_PUPDR6;
    GPIOD->OSPEEDR &= ~GPIO_OSPEEDER_OSPEEDR6;
    
    GPIOD->OSPEEDR |= 0x3 << (2*6);
    /* Setup USART2
     * baudrate = MIDI_BAUD_RATE
     * DIV = (CPU_FREQ / AHB_PRESCALAR / APB1_PRESCALAR)/(MIDI_BAUD_RATE)
     * setup to receive and enable
     * enable DMA mode for reception */
//    USART2->BRR = 180000000 / 1 / 4 / MIDI_BAUD_RATE;
    USART2->BRR = get_SystemCoreClock() / get_AHBPresc() 
        / get_APBPresc(1) / MIDI_BAUD_RATE;
    USART2->CR1 = 0x2004;
    USART2->CR3 = 0x40;
    /* Setup DMA
     * Channel 4
     * peripheral = USART2->DR
     * memory = midiBuffer
     * dir = peripheral to memory
     * buffer size is MIDI_BUF_SIZE
     * no peripheral inc
     * enable memory inc
     * peripheral size is byte
     * memory size is byte
     * circular mode
     * high priority
     * no FIFO (triggers when 1/4 full)
     * single memory burst 
     * single peripheral burst
     */
    DMA1_Stream5->CR = (0x4 << 25) 
        | (0x2 << 16) 
        | (0x1 << 10)
        | (0x1 << 8);
    DMA1_Stream5->PAR = (uint32_t)&USART2->DR;
    DMA1_Stream5->M0AR = (uint32_t)midiBuffer;
    DMA1_Stream5->NDTR = (uint16_t)MIDI_BUF_SIZE;
    DMA1_Stream5->FCR &= ~0x3;
    /* Enable DMA */
    DMA1_Stream5->CR |= 0x1;
    return(0);
}

midi_hw_err_t midi_hw_cleanup(midi_hw_cleanup_t *params)
{
	return 0;
}

void midi_hw_process_input(midi_hw_process_t *params)
{
    /*
    if (!MIDITimeToProcessBuffer) {
        return;
    }
    MIDITimeToProcessBuffer = 0;
    */
    int length = MIDI_DMA_STRUCT->NDTR;
    int topIndex = MIDI_BUF_SIZE - length;
    if (topIndex < MIDIlastIndex) {
        topIndex += MIDI_BUF_SIZE;
    }
    int numItems = topIndex - MIDIlastIndex;
    while (numItems--) {
            midi_hw_process_byte(midiBuffer[MIDIlastIndex]);
        /* increase last index so the function starts at the correct point next
         * interrupt */
        MIDIlastIndex = (MIDIlastIndex + 1) % MIDI_BUF_SIZE; 
    }
}
