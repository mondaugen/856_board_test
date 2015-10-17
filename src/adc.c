#include "adc.h" 
#include "stm32f4xx.h"

static volatile uint16_t adc1_values[ADC1_DMA_NUM_VALS_TRANS];
static volatile uint16_t adc3_values[ADC3_DMA_NUM_VALS_TRANS];
/* Stores the address of the first datum converted on the ADC channels */
uint16_t volatile *adc_data_starts[TOTAL_NUM_ADC_CHANNELS];

void __attribute__((optimize("O0"))) adc_setup_dma_scan(void)
{
    /* Enable ADC 1 and 3 Clock */
    RCC->APB2ENR |= RCC_APB2ENR_ADC3EN | RCC_APB2ENR_ADC1EN;
    /* Set clock prescalar to 4 */
    ADC->CCR &= ~ADC_CCR_ADCPRE;
    ADC->CCR |= (0x1 << 16);

    /* Set 4 conversions in conversion sequence on ADC 1*/
    ADC1->SQR1 &= ~ADC_SQR1_L;
    ADC1->SQR1 |= (3 << 20);

    /* Knob 1 -> PA6  -> ADC1,2   Channel 6  */
    /* Setup GPIO for analog mode */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    GPIOA->MODER &= ~GPIO_MODER_MODER6;
    GPIOA->MODER |= 0x3 << (6*2);
    /* Set sample time to 480 cycles */
    ADC1->SMPR2 &= ~ADC_SMPR2_SMP6;
    ADC1->SMPR2 |= 0x7 << (6*3);
    /* 1st conversion */
    ADC1->SQR3 &= ~ADC_SQR3_SQ1;
    ADC1->SQR3 |= 6 << (0*5);
    /* First datum of this ADC goes at... */
    adc_data_starts[0] = &adc1_values[0];

    /* Knob 2 -> PA4  -> ADC1,2   Channel 4  */
    GPIOA->MODER &= ~GPIO_MODER_MODER4;
    GPIOA->MODER |= 0x3 << (4*2);
    /* Set sample time to 480 cycles */
    ADC1->SMPR2 &= ~ADC_SMPR2_SMP4;
    ADC1->SMPR2 |= 0x7 << (4*3);
    /* 2nd conversion */
    ADC1->SQR3 &= ~ADC_SQR3_SQ2;
    ADC1->SQR3 |= 4 << (1*5);
    adc_data_starts[1] = &adc1_values[1];

    /* Knob 3 -> PA5  -> ADC1,2   Channel 5  */
    GPIOA->MODER &= ~GPIO_MODER_MODER5;
    GPIOA->MODER |= 0x3 << (5*2);
    /* Set sample time to 480 cycles */
    ADC1->SMPR2 &= ~ADC_SMPR2_SMP5;
    ADC1->SMPR2 |= 0x7 << (5*3);
    /* 3rd conversion */
    ADC1->SQR3 &= ~ADC_SQR3_SQ3;
    ADC1->SQR3 |= 5 << (2*5);
    adc_data_starts[2] = &adc1_values[2];

    /* Knob 4 -> PA3  -> ADC1,2   Channel 3  */
    GPIOA->MODER &= ~GPIO_MODER_MODER3;
    GPIOA->MODER |= 0x3 << (3*2);
    /* Set sample time to 480 cycles */
    ADC1->SMPR2 &= ~ADC_SMPR2_SMP3;
    ADC1->SMPR2 |= 0x7 << (3*3);
    /* 4th conversion */
    ADC1->SQR3 &= ~ADC_SQR3_SQ4;
    ADC1->SQR3 |= 3 << (3*5);
    adc_data_starts[3] = &adc1_values[3];

    /* Set 4 conversions in conversion sequence on ADC 3*/
    ADC3->SQR1 &= ~ADC_SQR1_L;
    ADC3->SQR1 |= (3 << 20);

    /* Knob 5 -> PC2  -> ADC1,2,3 Channel 12 */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
    GPIOC->MODER &= ~GPIO_MODER_MODER2;
    GPIOC->MODER |= 0x3 << (2*2);
    /* Set sample time to 480 cycles */
    ADC3->SMPR1 &= ~ADC_SMPR1_SMP12;
    ADC3->SMPR1 |= 0x7 << ((12-10)*3);
    /* 1st conversion */
    ADC3->SQR3 &= ~ADC_SQR3_SQ1;
    ADC3->SQR3 |= 12 << (0*5);
    adc_data_starts[4] = &adc3_values[0];

    /* Knob 6 -> PC3  -> ADC1,2,3 Channel 13 */
    GPIOC->MODER &= ~GPIO_MODER_MODER3;
    GPIOC->MODER |= 0x3 << (3*2);
    /* Set sample time to 480 cycles */
    ADC3->SMPR1 &= ~ADC_SMPR1_SMP13;
    ADC3->SMPR1 |= 0x7 << ((13-10)*3);
    /* 2nd conversion */
    ADC3->SQR3 &= ~ADC_SQR3_SQ2;
    ADC3->SQR3 |= 13 << (1*5);
    adc_data_starts[5] = &adc3_values[1];

    /* Knob 7 -> PF10 -> ADC3     Channel 8  */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOFEN;
    GPIOF->MODER &= ~GPIO_MODER_MODER10;
    GPIOF->MODER |= 0x3 << (10*2);
    /* Set sample time to 480 cycles */
    ADC3->SMPR2 &= ~ADC_SMPR2_SMP8;
    ADC3->SMPR2 |= 0x7 << (8*3);
    /* 3rd conversion */
    ADC3->SQR3 &= ~ADC_SQR3_SQ3;
    ADC3->SQR3 |= 8 << (2*5);
    adc_data_starts[6] = &adc3_values[2];

    /* Knob 8 -> PF6  -> ADC3     Channel 4  */
    GPIOF->MODER &= ~GPIO_MODER_MODER6;
    GPIOF->MODER |= 0x3 << (6*2);
    /* Set sample time to 480 cycles */
    ADC3->SMPR2 &= ~ADC_SMPR2_SMP4;
    ADC3->SMPR2 |= 0x7 << (4*3);
    /* 4th conversion */
    ADC3->SQR3 &= ~ADC_SQR3_SQ4;
    ADC3->SQR3 |= 4 << (3*5);
    adc_data_starts[7] = &adc3_values[3];

    /* ADC 1 */
    /* Don't set end of conversion flag after every conversion */
    ADC1->CR2 &= ~ADC_CR2_EOCS;
    /* Set continuous conversion */
    ADC1->CR2 |= ADC_CR2_CONT;
    /* Continue requesting DMA as long as DMA enabled */
    ADC1->CR2 |= ADC_CR2_DDS;
    /* Set scan mode */
    ADC1->CR1 |= ADC_CR1_SCAN;
    /* Enable DMA */
    ADC1->CR2 |= ADC_CR2_DMA;
    /* Enable ADC */
    ADC1->CR2 |= ADC_CR2_ADON;

    /* ADC 3 */
    /* Don't set end of conversion flag after every conversion */
    ADC3->CR2 &= ~ADC_CR2_EOCS;
    /* Set continuous conversion */
    ADC3->CR2 |= ADC_CR2_CONT;
    /* Continue requesting DMA as long as DMA enabled */
    ADC3->CR2 |= ADC_CR2_DDS;
    /* Set scan mode */
    ADC3->CR1 |= ADC_CR1_SCAN;
    /* Enable DMA */
    ADC3->CR2 |= ADC_CR2_DMA;
    /* Enable ADC */
    ADC3->CR2 |= ADC_CR2_ADON;

    /* Turn on DMA2 clock */
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;

    /* ADC 1 DMA Setup */

    /* Reset control register */
    DMA2_Stream4->CR = 0x00000000;
    /* Set to channel 0,
     * low priority,
     * memory and peripheral datum size 16-bits,
     * circular mode
     * transfer complete interrupt enable,
     * memory increment. */
    DMA2_Stream4->CR |= (0 << 25)
        | (0x0 << 16)
        | (0x1 << 13)
        | (0x1 << 11)
        | (0x1 << 10)
        | (0x1 << 8)
        | (0x1 << 4);
    /* Set peripheral address to ADC1's data register */
    DMA2_Stream4->PAR = (uint32_t)&(ADC1->DR);
    /* Set memory address to first half of ADC values */
    DMA2_Stream4->M0AR = (uint32_t)adc1_values;
    /* Set number of items to transfer */
    DMA2_Stream4->NDTR = ADC1_DMA_NUM_VALS_TRANS;
    
    /* Enable DMA2_Stream4 interrupt */
//    NVIC_EnableIRQ(DMA2_Stream4_IRQn);

    /* Enable DMA2, stream 4 */
    DMA2_Stream4->CR |= DMA_SxCR_EN;

    /* Start conversion */
    ADC1->CR2 |= ADC_CR2_SWSTART;

    /* ADC 3 DMA Setup */
    /* Reset control register */
    DMA2_Stream0->CR = 0x00000000;
    /* Set to channel 2,
     * low priority,
     * memory and peripheral datum size 16-bits,
     * circular mode
     * transfer complete interrupt enable,
     * memory increment. */
    DMA2_Stream0->CR |= (2 << 25)
        | (0x0 << 16)
        | (0x1 << 13)
        | (0x1 << 11)
        | (0x1 << 10)
        | (0x1 << 8)
        | (0x1 << 4);
    /* Set peripheral address to ADC3's data register */
    DMA2_Stream0->PAR = (uint32_t)&(ADC3->DR);
    /* Set memory address to second half ADC values */
    DMA2_Stream0->M0AR = (uint32_t)adc3_values;
    /* Set number of items to transfer */
    DMA2_Stream0->NDTR = ADC3_DMA_NUM_VALS_TRANS;
    
    /* Enable DMA2_Stream0 interrupt */
//    NVIC_EnableIRQ(DMA2_Stream0_IRQn);

    /* Enable DMA2, stream 0 */
    DMA2_Stream0->CR |= DMA_SxCR_EN;

    /* Start conversion */
    ADC3->CR2 |= ADC_CR2_SWSTART;
}

/* "ADC 3's" DMA handler */
//void __attribute__((optimize("O0"))) DMA2_Stream0_IRQHandler(void)
//{
//    NVIC_ClearPendingIRQ(DMA2_Stream0_IRQn);
//    if (DMA2->LISR & DMA_LISR_TCIF0) {
//        /* Clear interrupt */
//        DMA2->LIFCR |= DMA_LIFCR_CTCIF0;
//        /* Enable DMA2 */
//        DMA2_Stream0->CR |= DMA_SxCR_EN;
//        /* Reset DMA */
//        ADC3->CR2 &= ~ADC_CR2_DMA;
//        ADC3->CR2 |= ADC_CR2_DMA;
//        /* Start conversion */
//        ADC3->CR2 |= ADC_CR2_SWSTART;
//    }
//}

/* "ADC 1's" DMA Handler */
//void __attribute__((optimize("O0"))) DMA2_Stream4_IRQHandler(void)
//{
//    NVIC_ClearPendingIRQ(DMA2_Stream4_IRQn);
//    if (DMA2->HISR & DMA_HISR_TCIF4) {
//        /* Clear interrupt */
//        DMA2->HIFCR |= DMA_HIFCR_CTCIF4;
//        /* Enable DMA2 */
//        DMA2_Stream4->CR |= DMA_SxCR_EN;
//        /* Reset DMA */
//        ADC1->CR2 &= ~ADC_CR2_DMA;
//        ADC1->CR2 |= ADC_CR2_DMA;
//        /* Start conversion */
//        ADC1->CR2 |= ADC_CR2_SWSTART;
//    }
//}
