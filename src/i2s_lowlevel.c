/* Implementation based on test/pcm.c found in the alsa-lib repository. */

#include "i2s_lowlevel.h" 
#include "stm32f4xx.h"
#include "audio_hw.h" 
#include <string.h>

#define WM8778_CODEC_ADDR  ((uint8_t)0x34)
#define CODEC_I2C_TIMEOUT       ((uint32_t)1000000) 

#ifdef CODEC_DMA_HARDFAULT_ON_I2S_ERR
extern void HardFault_Handler(void);
#endif

/* Where data to be transferred to CODEC reside */
static int16_t codecDmaTxBuf[CODEC_DMA_BUF_LEN * 2]
    __attribute__((section(".small_data"),aligned(1024)));
/* Where data from CODEC reside */
static int16_t codecDmaRxBuf[CODEC_DMA_BUF_LEN * 2]
    __attribute__((section(".small_data"),aligned(1024)));
/* Which half of transmit buffer we are at currently */
static int16_t * volatile codecDmaTxPtr
    __attribute__((section(".small_data"))) = NULL;
/* Which half of receive buffer we are at currently */
static int16_t * volatile codecDmaRxPtr
    __attribute__((section(".small_data"))) = NULL;

/* The audio_hw_io_t structure */
static audio_hw_io_t audiohwio;
/* The sample rate */
static unsigned int rate = 0;

static void codec_i2c_setup(void);
static void codec_config_via_i2c(void);

unsigned int audio_hw_get_sample_rate(void *data) {
    return rate;
}

unsigned int audio_hw_get_block_size(void *data) {
    return CODEC_DMA_BUF_LEN/CODEC_NUM_CHANNELS;
}

unsigned int audio_hw_get_num_input_channels(void *data) {
    return CODEC_NUM_CHANNELS;
}

unsigned int audio_hw_get_num_output_channels(void *data) {
    return CODEC_NUM_CHANNELS;
}

static void __attribute__((optimize("O0"))) i2s_codec_config_pins_setup(void)
{
    /* Turn on clock for GPIOB and GPIOC pins */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOCEN;
    /* Set pins to output mode */
    GPIOB->MODER |= (1 << (10 * 2)) | (1 << (11 * 2));
    GPIOC->MODER |= (1 << (13 * 2));
    /* No pull-up or pull-down */
    GPIOB->PUPDR &= ~((3 << (10 * 2)) | (3 << (11 * 2)));
    GPIOC->PUPDR &= ~(3 << (13 * 2));
    /* set PC13 to high to enable I2S on the codec */
    GPIOC->ODR |= (1 << 13);
    /* set PB10 and PB11 to low to set word size to 16-bits and turn off
     * de-emphasis resp. */
    GPIOB->ODR &= ~((1 << 10) | (1 << 11));

    /* for debugging */
}

/* This assumes i2s is configured as master */
static int __attribute__((optimize("O0"))) i2s_clock_setup(uint32_t sr)
{
    /* Disable PLLI2S */
    RCC->CR &= ((uint32_t)(~RCC_CR_PLLI2SON));

    /* PLLI2S clock used as I2S clock source */
    RCC->CFGR &= ~RCC_CFGR_I2SSRC;

    /* Configure PLLI2S */
    /* see stm32f4 reference manual, p. 894 */
    switch(sr) {
        case 44100:
            /* Set I2SN and I2SR prescalars */
            RCC->PLLI2SCFGR = (271 << 6) | (2 << 28);
            /* Enable Master clock and set ODD bit and I2SDIV */
            SPI3->I2SPR     = ((0x2 << 8) | 0x6); // 44.1Khz
            /* Enable master clock for extended block. */
            I2S3ext->I2SPR  = ((0x2 << 8) | 0x6);
            break;
        case 16000:
            RCC->PLLI2SCFGR = (213 << 6) | (2 << 28);
            SPI3->I2SPR     = ((0x2 << 8) | 13); // 16KHz
            I2S3ext->I2SPR  = ((0x2 << 8) | 13);
            break;
        case 32000:
            RCC->PLLI2SCFGR = (213 << 6) | (2 << 28);
            SPI3->I2SPR     = ((0x3 << 8) | 0x6); // 32Khz
            I2S3ext->I2SPR  = ((0x3 << 8) | 0x6);
            break;
        default:
            return -1; /* bad sampling rate */
    }

    /* Enable PLLI2S */
    RCC->CR |= ((uint32_t)RCC_CR_PLLI2SON);

    /* Wait till PLLI2S is ready */
    while((RCC->CR & RCC_CR_PLLI2SRDY) == 0);

    return 0;
}

static void __attribute__((optimize("O0"))) i2s_error_setup(void)
{
    /* Setup pin that can be used to indicate I2S error */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOGEN;
    /* Set up on PG9 */
    GPIOG->MODER &= ~(0x3 << (9*2));
    GPIOG->MODER |= 0x1 << (9*2);
    GPIOG->OSPEEDR |= 0x3 << (9*2);
    GPIOG->PUPDR &= ~(0x3 << (9*2));
    GPIOG->ODR &= ~(1 << 9);
    SPI3->CR2 |= SPI_CR2_ERRIE;
    I2S3ext->CR2 |= SPI_CR2_ERRIE;
    NVIC_EnableIRQ(SPI3_IRQn);
}


/* Pass a pointer to a uint32_t containing the desired sampling rate. */
audio_hw_err_t audio_hw_setup(audio_hw_setup_t *params)
{
    uint32_t sr = *((uint32_t*)params);
    /* Zero the buffers */
    memset(codecDmaTxBuf,0,sizeof(int16_t)*CODEC_DMA_BUF_LEN*2);
    memset(codecDmaRxBuf,0,sizeof(int16_t)*CODEC_DMA_BUF_LEN*2);

    /* Configure codec */
    i2s_codec_config_pins_setup();

    /* Turn on GPIO clock for I2S3 pins */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOCEN;
    /* Configure GPIO */
    /* Configure PA15 to Alternate Function */
    GPIOA->MODER &= ~(0x3 << 30);
    GPIOA->MODER |= (0x2 << 30);
    /* Configure PC7, PC10-12 to Alternate Function */
    GPIOC->MODER &= ~((0x3 << 20) | (0x3 << 22) | (0x3 << 24) | (0x3 << 14));
    GPIOC->MODER |= (0x2 << 20) | (0x2 << 22) | (0x2 << 24) | (0x2 << 14);
    /* Set pins to high speed */
    GPIOA->OSPEEDR |= (0x3 << 30);
    GPIOC->OSPEEDR |= (0x3 << 20) | (0x3 << 22) | (0x3 << 24) | (0x3 << 14);
    /* Pins have no-pull up nor pull-down */
    GPIOA->PUPDR &= ~(0x3 << 30);
    GPIOC->PUPDR &= ~((0x3 << 20) | (0x3 << 22) | (0x3 << 24) | (0x3 << 14));
    /* A15 Alternate function 6 */
    GPIOA->AFR[1] &= ~(0xf << 28);
    GPIOA->AFR[1] |= (0x6 << 28);
    /* C7 Alternate function 6 */
    GPIOC->AFR[0] &= ~(0xf << 28);
    GPIOC->AFR[0] |= (0x6 << 28);
    /* C10,12, Alternate function 6, C11 alternate function 5 */
    GPIOC->AFR[1] &= ~((0xf << 8) | (0xf << 12) | (0xf << 16));
    GPIOC->AFR[1] |= ((0x6 << 8) | (0x5 << 12) | (0x6 << 16));

    /* Turn on DMA1 clock */
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;

    /* Set up memory to peripheral DMA */
    /* Disable DMA peripheral */
    if (DMA1_Stream7->CR & DMA_SxCR_EN) {
        DMA1_Stream7->CR &= ~(DMA_SxCR_EN);
    }
    /* Wait until free */
    while (DMA1_Stream7->CR & DMA_SxCR_EN);
    /* Set peripheral address to SPI3 data register */
    DMA1_Stream7->PAR = (uint32_t)&(SPI3->DR);
    /* Set memory address to transmit buffer */
    DMA1_Stream7->M0AR = (uint32_t)codecDmaTxBuf;
    /* Inform DMA peripheral of buffer length. This is two times the defined
     * value because we trigger on HALF and FULL transfer */
    DMA1_Stream7->NDTR = (uint32_t)(CODEC_DMA_BUF_LEN * 2);
    /* Set up DMA control register: */
    /* Channel 0 */
    DMA1_Stream7->CR &= ~DMA_SxCR_CHSEL;
    /* Priority VERY HIGH */
    DMA1_Stream7->CR &= ~DMA_SxCR_PL;
    DMA1_Stream7->CR |= 0x3 << 16;
#ifdef CODEC_DMA_DIRECT_MODE
    DMA1_Stream7->FCR &= ~DMA_SxFCR_DMDIS;
#else
    DMA1_Stream7->FCR |= DMA_SxFCR_DMDIS;
    /* PBURST one burst of 8 beats */
    DMA1_Stream7->CR &= ~DMA_SxCR_PBURST;
    DMA1_Stream7->CR |= 0x2 << 21;
    /* MBURST one burst of 8 beats */
    DMA1_Stream7->CR &= ~DMA_SxCR_MBURST;
    DMA1_Stream7->CR |= 0x2 << 23;
#endif /* CODEC_DMA_DIRECT_MODE */
    /* No Double Buffer Mode (we do this ourselves with the HALF and FULL
     * transfer) */
    DMA1_Stream7->CR &= ~DMA_SxCR_DBM;
    /* Memory datum size 16-bit */
    DMA1_Stream7->CR &= ~DMA_SxCR_MSIZE;
    DMA1_Stream7->CR |= 0x1 << 13;
    /* Peripheral datum size 16-bit */
    DMA1_Stream7->CR &= ~DMA_SxCR_PSIZE;
    DMA1_Stream7->CR |= 0x1 << 11;
    /* Memory incremented after each transfer */
    DMA1_Stream7->CR |= DMA_SxCR_MINC;
    /* No peripheral address increment */
    DMA1_Stream7->CR &= ~DMA_SxCR_PINC;
    /* Circular buffer mode */
    DMA1_Stream7->CR |= DMA_SxCR_CIRC;
    /* Memory to peripheral mode (this is the transmitting peripheral) */
    DMA1_Stream7->CR &= ~DMA_SxCR_DIR;
    DMA1_Stream7->CR |= 0x1 << 6;
    /* DMA is the flow controller (DMA will keep transferring items from memory
     * to peripheral until disabled) */
    DMA1_Stream7->CR &= ~DMA_SxCR_PFCTRL;
    // /* Only trigger interrupt on RX line */
    ///* Enable interrupt on transfer complete */
    //DMA1_Stream7->CR |= DMA_SxCR_TCIE;
    ///* Enable interrupt on transfer half complete */
    //DMA1_Stream7->CR |= DMA_SxCR_HTIE;
    /* Interrupt on transfer error */
    DMA1_Stream7->CR |= DMA_SxCR_TEIE;
    /* Interrupt on direct mode error */
    DMA1_Stream7->CR |= DMA_SxCR_DMEIE;
    
    /* Set up peripheral to memory DMA */
    /* Disable DMA peripheral */
    if (DMA1_Stream0->CR & DMA_SxCR_EN) {
        DMA1_Stream0->CR &= ~(DMA_SxCR_EN);
    }
    /* Wait until free */
    while (DMA1_Stream0->CR & DMA_SxCR_EN);
    /* Set peripheral address to I2S3_ext data register */
    DMA1_Stream0->PAR = (uint32_t)&(I2S3ext->DR);
    /* Set memory address to receive buffer */
    DMA1_Stream0->M0AR = (uint32_t)codecDmaRxBuf;
    /* Inform DMA peripheral of buffer length. This is two times the defined
     * value because we trigger on HALF and FULL transfer */
    DMA1_Stream0->NDTR = (uint32_t)(CODEC_DMA_BUF_LEN * 2);
    /* Set up DMA control register: */
    /* Channel 3 */
    DMA1_Stream0->CR &= ~DMA_SxCR_CHSEL;
    DMA1_Stream0->CR |= 0x3 << 25;
    /* Priority VERY HIGH */
    DMA1_Stream0->CR &= ~DMA_SxCR_PL;
    DMA1_Stream0->CR |= 0x3 << 16;
#ifdef CODEC_DMA_DIRECT_MODE
    DMA1_Stream0->FCR &= ~DMA_SxFCR_DMDIS;
#else
    DMA1_Stream0->FCR |= DMA_SxFCR_DMDIS;
    /* PBURST one burst of 8 beats */
    DMA1_Stream0->CR &= ~DMA_SxCR_PBURST;
    DMA1_Stream0->CR |= 0x2 << 21;
    /* MBURST one burst of 8 beats */
    DMA1_Stream0->CR &= ~DMA_SxCR_MBURST;
    DMA1_Stream0->CR |= 0x2 << 23;
#endif /* CODEC_DMA_DIRECT_MODE */
    /* No Double Buffer Mode (we do this ourselves with the HALF and FULL
     * transfer) */
    DMA1_Stream0->CR &= ~DMA_SxCR_DBM;
    /* Memory datum size 16-bit */
    DMA1_Stream0->CR &= ~DMA_SxCR_MSIZE;
    DMA1_Stream0->CR |= 0x1 << 13;
    /* Peripheral datum size 16-bit */
    DMA1_Stream0->CR &= ~DMA_SxCR_PSIZE;
    DMA1_Stream0->CR |= 0x1 << 11;
    /* Memory incremented after each transfer */
    DMA1_Stream0->CR |= DMA_SxCR_MINC;
    /* No peripheral address increment */
    DMA1_Stream0->CR &= ~DMA_SxCR_PINC;
    /* Circular buffer mode */
    DMA1_Stream0->CR |= DMA_SxCR_CIRC;
    /* Peripheral to memory mode (this is the receiving peripheral) */
    DMA1_Stream0->CR &= ~DMA_SxCR_DIR;
    /* DMA is the flow controller (DMA will keep transferring items from
     * peripheral to memory until disabled) */
    DMA1_Stream0->CR &= ~DMA_SxCR_PFCTRL;
    /* Enable interrupt on transfer complete */
    DMA1_Stream0->CR |= DMA_SxCR_TCIE;
    /* Enable interrupt on transfer half complete */
    DMA1_Stream0->CR |= DMA_SxCR_HTIE;
    /* Interrupt on transfer error */
    DMA1_Stream0->CR |= DMA_SxCR_TEIE;
    /* Interrupt on direct mode error */
    DMA1_Stream0->CR |= DMA_SxCR_DMEIE;

    /* Turn on I2S3 clock (SPI3) */
    RCC->APB1ENR |= RCC_APB1ENR_SPI3EN;
    /* set up clock dividers for desired sampling rate */
    if (i2s_clock_setup(sr)) {
        return -1;
    }
    /* Store sample rate */
    rate = sr;
    /* CKPOL = 0, I2SMOD = 1, I2SEN = 0 (don't enable yet), I2SSTD = 00
     * (Phillips), DATLEN = 00 (16-bit), CHLEN = 0 (16-bit) I2SCFGR = 10 (Master
     * transmit) */
    SPI3->I2SCFGR = 0xa00;
    /* TXDMAEN = 1 (Transmit buffer empty DMA request enable), other bits off */
    SPI3->CR2 = SPI_CR2_TXDMAEN ;
    /* Set up duplex instance the same as SPI3, except configure as slave
     * receive and trigger interrupt when receive buffer full */
    /* same as above but I2SCFG = 01 (slave receive) */
    I2S3ext->I2SCFGR = 0x900;
    /* RXDMAEN = 1 (Receive buffer not empty DMA request enable), other bits off */
    I2S3ext->CR2 = SPI_CR2_RXDMAEN ;
    
    /* Enable I2S error interrupts */
    i2s_error_setup();

    /* Enable the DMA peripherals */
    /* Set up I2C communication */
    codec_i2c_setup();
   
    /* Audio should now be configured but is not yet enabled (must call
     * audio_hw_start). */

    return 0;
}

audio_hw_err_t audio_hw_start(audio_hw_setup_t *params)
{
    audiohwio.length = audio_hw_get_block_size(NULL);
    audiohwio.nchans_in = audio_hw_get_num_input_channels(NULL);
    audiohwio.nchans_out = audio_hw_get_num_output_channels(NULL);
    codec_config_via_i2c();

    /* clear possible Interrupt flags */
    DMA1->HIFCR |= 0x00000f80;
    DMA1_Stream7->CR |= DMA_SxCR_EN;
    /* clear possible Interrupt flags */
    DMA1->LIFCR &= 0x0000001f;
    DMA1_Stream0->CR |= DMA_SxCR_EN;

    /* Enable DMA interrupts */
    NVIC_EnableIRQ(DMA1_Stream7_IRQn);
    NVIC_EnableIRQ(DMA1_Stream0_IRQn);

    /* Turn on I2S3 and its extended block */
    SPI3->I2SCFGR |= 0x400;
    /* Wait for word select line to go high before enabling extended block as it
     * is running as slave. (See STM32F429 errata sheet). */
    while (!(GPIOA->IDR & (1 << 15)));
    I2S3ext->I2SCFGR |= 0x400;

    /* Wait for them to be enabled (to show they are ready) */
    while(!((DMA1_Stream7->CR & DMA_SxCR_EN) && (DMA1_Stream0->CR & DMA_SxCR_EN)));

    return 0;
}

void DMA1_Stream0_IRQHandler(void)
{
    NVIC_ClearPendingIRQ(DMA1_Stream0_IRQn);
    uint32_t dma1_lisr = DMA1->LISR;
    if (dma1_lisr & DMA_LISR_TEIF0) {
        /* Transfer error */
#ifdef CODEC_DMA_HARDFAULT_ON_I2S_ERR
        HardFault_Handler();
#endif
        DMA1->LIFCR |= DMA_LIFCR_CTEIF0;
    }
    if (dma1_lisr & DMA_LISR_DMEIF0) {
        /* Direct mode error */
#ifdef CODEC_DMA_HARDFAULT_ON_I2S_ERR
        HardFault_Handler();
#endif
        DMA1->LIFCR |= DMA_LIFCR_CDMEIF0;
    }
    /* If transfer complete on stream 0 (peripheral to memory), set current rx
     * pointer to half of the buffer */
    if (dma1_lisr & DMA_LISR_TCIF0) {
        /* clear flag */
        DMA1->LIFCR |= DMA_LIFCR_CTCIF0;
        codecDmaTxPtr = codecDmaTxBuf + CODEC_DMA_BUF_LEN;
        codecDmaRxPtr = codecDmaRxBuf + CODEC_DMA_BUF_LEN;
    }
    /* If half of transfer complete on stream 0 (peripheral to memory), set
     * current rx pointer to beginning of the buffer */
    if (dma1_lisr & DMA_LISR_HTIF0) {
        /* clear flag */
        DMA1->LIFCR |= DMA_LIFCR_CHTIF0;
        codecDmaTxPtr = codecDmaTxBuf;
        codecDmaRxPtr = codecDmaRxBuf;
    }
    audiohwio.in = codecDmaRxPtr;
    audiohwio.out = codecDmaTxPtr;
    audio_hw_io(&audiohwio);
}

void DMA1_Stream7_IRQHandler(void)
{
    NVIC_ClearPendingIRQ(DMA1_Stream7_IRQn);
    uint32_t dma1_hisr = DMA1->LISR;
    if (dma1_hisr & DMA_HISR_TEIF7) {
        /* Transfer error */
#ifdef CODEC_DMA_HARDFAULT_ON_I2S_ERR
        HardFault_Handler();
#endif
        DMA1->HIFCR |= DMA_HIFCR_CTEIF7;
    }
    if (dma1_hisr & DMA_HISR_DMEIF7) {
        /* Direct mode error */
#ifdef CODEC_DMA_HARDFAULT_ON_I2S_ERR
        HardFault_Handler();
#endif
        DMA1->HIFCR |= DMA_HIFCR_CDMEIF7;
    }

}

void SPI3_IRQHandler (void)
{
    NVIC_ClearPendingIRQ(SPI3_IRQn);
    /* Disable SPI interrupts */
    NVIC_DisableIRQ(SPI3_IRQn);
    uint32_t spi3_sr = SPI3->SR, i2s3ext_sr = I2S3ext->SR;
    static volatile uint32_t num_i2s3ext = 0;
    if (spi3_sr & SPI_SR_OVR) {
#ifdef CODEC_DMA_HARDFAULT_ON_I2S_ERR
        HardFault_Handler();
#endif
    }
    if (spi3_sr & SPI_SR_UDR) {
#ifdef CODEC_DMA_HARDFAULT_ON_I2S_ERR
        HardFault_Handler();
#endif
        /* ... */
    }
    /* Check frame error */
    if (spi3_sr & 0x100) {
#ifdef CODEC_DMA_HARDFAULT_ON_I2S_ERR
        HardFault_Handler();
#endif
#ifdef CODEC_DMA_TRIGGER_ON_SPI3_FRAME_ERR
        GPIOG->ODR |= 1 << 9;
#endif
        /* ... */
    }
    if (i2s3ext_sr & SPI_SR_OVR) {
#ifdef CODEC_DMA_HARDFAULT_ON_I2S_ERR
        HardFault_Handler();
#endif
        /* ... */
    }
    if (i2s3ext_sr & SPI_SR_OVR) {
#ifdef CODEC_DMA_HARDFAULT_ON_I2S_ERR
        HardFault_Handler();
#endif
        /* ... */
    }
    /* Check frame error */
    if (i2s3ext_sr & 0x100) {
        num_i2s3ext++;
#ifdef CODEC_DMA_HARDFAULT_ON_I2S_ERR
        HardFault_Handler();
#endif
#ifdef CODEC_DMA_TRIGGER_ON_I2S3_FRAME_ERR
        GPIOG->ODR |= 1 << 9;
//        GPIOG->ODR &= ~(1 << 9);
#endif
        /* Disable I2S */
        I2S3ext->I2SCFGR &= ~SPI_I2SCFGR_I2SE;
        SPI3->I2SCFGR &= ~SPI_I2SCFGR_I2SE;
        /* Wait until disabled */
        while ((I2S3ext->I2SCFGR & SPI_I2SCFGR_I2SE) 
                || (SPI3->I2SCFGR & SPI_I2SCFGR_I2SE));
        /* Reconfigure codec */
        codec_config_via_i2c();
        /* Enable I2S master */
        SPI3->I2SCFGR |= SPI_I2SCFGR_I2SE;
        /* Wait until correct level is detected on WS line (high for I2S mode)
         * for extended block which is running in slave mode */
        while (!(GPIOA->IDR & (1 << 15)));
        /* Enable I2S slave */
        I2S3ext->I2SCFGR |= SPI_I2SCFGR_I2SE;
#ifdef CODEC_DMA_TRIGGER_ON_I2S3_FRAME_ERR
        GPIOG->ODR &= ~(1 << 9);
//        GPIOG->ODR &= ~(1 << 9);
#endif
    }
    /* Enable SPI interrupts */
    NVIC_EnableIRQ(SPI3_IRQn);
}

static int codec_i2c_check_flags(uint32_t flags)
{
    uint32_t tmp;
    tmp = (I2C2->SR1 << 16) | (I2C2->SR2);
    if ((tmp & flags) == flags) {
        return 1;
    }
    return 0;
}

static void codec_i2c_swrst(void)
{
    I2C2->CR1 |= I2C_CR1_SWRST;
    while (codec_i2c_check_flags(0x00c00000));
    I2C2->CR1 &= ~I2C_CR1_SWRST;
}

static void codec_prog_reg_i2c(uint8_t addr,
                               uint8_t reg_addr,
                               uint16_t reg_val)
{
//    uint32_t tmp1, tmp2;
#ifdef     CODEC_I2C_PROG_START_TRIGGER
    GPIOG->ODR |= 1 << 9;
    GPIOG->ODR &= ~(1 << 9);
#endif  
    uint32_t timeout;
    /* Wait while busy */
    timeout = CODEC_I2C_TIMEOUT;
    while (codec_i2c_check_flags(0x2) && --timeout);
    if (timeout == 0) {
        codec_i2c_swrst();
    }
    /* Send start condition */
    I2C2->CR1 |= I2C_CR1_START;
    /* Wait for master mode select */
    timeout = CODEC_I2C_TIMEOUT;
    while ((!codec_i2c_check_flags(0x10003)) && --timeout);
    if (timeout == 0) {
        codec_i2c_swrst();
    }
    /* Send address */
    I2C2->DR = addr;
    /* Wait for master mode selected. */
    timeout = CODEC_I2C_TIMEOUT;
    while ((!codec_i2c_check_flags(0x00820007)) && --timeout);
    if (timeout == 0) {
        codec_i2c_swrst();
    }
    /* Send bytes */
    uint8_t byte1, byte2;
    byte1 = (reg_addr << 1) | ((reg_val >> 8) & (0x1));
    byte2 = (uint8_t)(reg_val & 0xff);
    /* Write byte */
    I2C2->DR = byte1;
    /* Wait for I2C to finish transmitting */
    timeout = CODEC_I2C_TIMEOUT;
    while ((!codec_i2c_check_flags(0x00800007)) && --timeout);
    if (timeout == 0) {
        codec_i2c_swrst();
    }
    /* Write byte */
    I2C2->DR = byte2;
    /* Wait for I2C to finish transmitting */
    timeout = CODEC_I2C_TIMEOUT;
    while ((!codec_i2c_check_flags(0x00800007)) && --timeout);
    if (timeout == 0) {
        codec_i2c_swrst();
    }
    /* Wait for buffer to be empty */
    timeout = CODEC_I2C_TIMEOUT;
    while ((!codec_i2c_check_flags(0x00040000)) && --timeout);
    if (timeout == 0) {
        codec_i2c_swrst();
    }
    /* Send stop condition */
    I2C2->CR1 |= I2C_CR1_STOP;
}

static void codec_i2c_setup(void)
{
    /* Enable GPIOB, I2C2 clock */
    RCC->APB1ENR |= RCC_APB1ENR_I2C2EN;
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOCEN;
    /* Enable CE pin to set codec's address */
    GPIOC->MODER &= ~(0x3 << (2 * 13));
    /* Set to output */
    GPIOC->MODER |= (0x1 << (2 * 13));
    /* Set to Open/Drain */
//    GPIOC->OTYPER |= (0x1 << 13);
    /* High speed (?) */
    GPIOC->OSPEEDR &= ~(0x3 << (2* 13));
    GPIOC->OSPEEDR |= (0x2 << (2* 13));
    /* Pull down */
//    GPIOC->PUPDR &= ~(0x3 << (2 * 13));
//    GPIOC->PUPDR |= (0x2 << (2 * 13));
    /* Set low */
    GPIOC->ODR &= ~(0x1 << 13);
    /* Enable I2C2 Pins, set to AF */
    /* PB10,PB11 */
    GPIOB->MODER &= ~((0x3 << (2* 10)) | (0x3 << (2*11)));
    GPIOB->MODER |= ((0x2 << (2* 10)) | (0x2 << (2*11)));
    /* High speed (?) */
    GPIOB->OSPEEDR &= ~((0x3 << (2* 10)) | (0x3 << (2*11)));
    GPIOB->OSPEEDR |= ((0x2 << (2* 10)) | (0x2 << (2*11)));
    /* I2C Functions are alternate function 4 */
    GPIOB->AFR[1] &= ~((0xf << (4*2)) | (0xf << (4*3)));
    GPIOB->AFR[1] |= ((0x4 << (4*2)) | (0x4 << (4*3)));
    /* Pull-up */
    GPIOB->PUPDR &= ~((0x3 << (2 * 10)) | (0x3 << (2 * 11)));
    GPIOB->PUPDR |= ((0x2 << (2 * 10)) | (0x2 << (2 * 11)));
    /* Open / Drain */
    GPIOB->OTYPER |= ((0x1 << 10) | (0x1 << 11));
    /* Set clock equal to APB1 clock */
    I2C2->CR2 &= ~(0x1f);
    I2C2->CR2 |= (uint32_t)45; /* 45Mhz */
    /* Set up rise time based on clock and codec's I2C characteristics. */
    I2C2->TRISE &= ~(0xf3);
    /* no more than 14 clocks fit into the minimum rise time */
    I2C2->TRISE |= (uint32_t)14; /* See p. 853 STM32F429 reference manual. */
    /* Set clock control register (see p. 852 ibid)*/
    I2C2->CCR = (uint32_t)452; /* SCL is about 50Khz */
    /* Enable acknowledge */
//    I2C2->CR1 |= I2C_CR1_ACK;
    /* Enable peripheral */
    I2C2->CR1 |= I2C_CR1_PE;
}

static void codec_config_via_i2c(void)
{
    /* Set ADC, DAC to I2S 16-bit */
    codec_prog_reg_i2c(WM8778_CODEC_ADDR,0x17,0x0000);
    codec_prog_reg_i2c(WM8778_CODEC_ADDR,0xa,0x0002);
    codec_prog_reg_i2c(WM8778_CODEC_ADDR,0xb,0x0042);
    codec_prog_reg_i2c(WM8778_CODEC_ADDR,0x5,0x00ff);
    /* invert MCLK */
    /* That's it */
}
