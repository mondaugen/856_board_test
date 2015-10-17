#include "flash_commanding.h" 
#include "stm32f4xx.h" 

volatile uint32_t flash_state = 0;
static char *   data_to_write = NULL;
static uint32_t data_to_write_size = 0;

#define FLASH_DMA_BASE DMA2_Stream6 
#define FLASH_DMA_BASE_IRQHandler DMA2_Stream6_IRQHandler
#define FLASH_DMA_BASE_IRQn DMA2_Stream6_IRQn
#define DMA_HISR_TCIF() DMA_HISR_TCIF ## 6 
#define DMA_HIFCR_CTCIF() DMA_HIFCR_CTCIF ## 6 
#define DMA_HISR_TEIF() DMA_HISR_TEIF ## 6 
#define DMA_HIFCR_CTEIF() DMA_HIFCR_CTEIF ## 6 

void flash_request_write(char *what, uint32_t length)
{
    if (flash_state & FLASH_CMD_BUSY) {
        /* A write or an erase is still in progress */
        return;
    }
    if (flash_state 
            & (FLASH_CMD_WRITE_IN_PROGRESS | FLASH_CMD_ERASE_IN_PROGRESS)) {
        return; /* Already erasing or writing */
    }
    if (FLASH_DMA_BASE->CR & DMA_SxCR_EN) {
        return; /* DMA is not done transferring. */
    }
    if (FLASH->SR & FLASH_SR_BSY) {
        return; /* FLASH busy */
    }
    if (what && length) {
        data_to_write = what;
        data_to_write_size = length;
        /* Request erasure which will then be followed by a write */
        flash_state |= FLASH_CMD_WRITE_REQUEST | FLASH_CMD_ERASE_REQUEST;
    }
    if ((flash_state & FLASH_CMD_WRITE_REQUEST)
            && (flash_state & FLASH_CMD_ERASE_REQUEST)) {
        /* Both of these are required to start the erase_then_write procedure */
        /* Start erasing */
        /* Unlock flash */
        FLASH->KEYR = 0x45670123;
        FLASH->KEYR = 0xcdef89ab;
        /* Reset program size register */
        FLASH->CR &= ~FLASH_CR_PSIZE;
#if FLASH_ACCESS_SIZE == 4
        FLASH->CR = 0x2 << 8;
#else
#error "Bad value for FLASH_ACCESS_SIZE."
#endif 
        /* Set sector erase bit and select sector */
        FLASH->CR &= ~FLASH_CR_SNB;
#if FLASH_SECTOR >= 12
        FLASH->CR |= ((0x1 << 4) | (FLASH_SECTOR - 12)) << 3;
#else
        FLASH->CR |= FLASH_SECTOR << 3;
#endif  
        FLASH->CR |= FLASH_CR_SER;
        /* Set interrupt on done erasing */
        NVIC_EnableIRQ(FLASH_IRQn);
        FLASH->CR |= FLASH_CR_EOPIE;
        /* Set erase in progress bit */
        flash_state |= FLASH_CMD_ERASE_IN_PROGRESS;
        /* Request has been acknowledged to erase, clear this bit */
        flash_state &= ~FLASH_CMD_ERASE_REQUEST;
        /* Start erasing */
        FLASH->CR |= FLASH_CR_STRT;
    }
}

void __attribute__((optimize("O0"))) FLASH_IRQHandler(void)
{
    NVIC_ClearPendingIRQ(FLASH_IRQn);
    if (FLASH->SR & FLASH_SR_EOP) {
        /* Clear by writing one to this bit */
        FLASH->SR |= FLASH_SR_EOP;
        /* Turn off flash interrupt */
        FLASH->CR &= ~FLASH_CR_EOPIE;
        NVIC_DisableIRQ(FLASH_IRQn);
        if (flash_state & FLASH_CMD_ERASE_IN_PROGRESS) {
            /* Erasure just finished. */
            /* Reset sector erase bit */
            FLASH->CR &= ~FLASH_CR_SER;
            if (flash_state & FLASH_CMD_WRITE_REQUEST) {
                /* Start a write to flash */
                flash_state |= FLASH_CMD_WRITE_IN_PROGRESS;
                /* start writing */
                /* Set program bit */
                FLASH->CR |= FLASH_CR_PG;
                /* Set up DMA to write to flash */
                /* Turn on DMA2 clock */
                RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;
                /* Reset control register */
                FLASH_DMA_BASE->CR = 0x00000000;
                /* Set to channel 0, low priority, memory and peripheral datum
                 * size 32-bits, transfer complete interrupt enable, memory
                 * increment, peripheral (other memory) increment. */
                FLASH_DMA_BASE->CR |= (0 << 25)
                    | (0x0 << 16)
                    | (0x2 << 13)
                    | (0x2 << 11)
                    | (0x1 << 4)
                    | (0x2 << 6) /* memory to memory transfer */
                    | DMA_SxCR_MINC
                    | DMA_SxCR_PINC
                    | (0x0 << 23) /* no burst */
                    | (0x0 << 21) /* no burst */
                    | DMA_SxCR_TEIE; /* Transfer error interrupt enable */
                FLASH_DMA_BASE->FCR &= ~DMA_SxFCR_FTH;
                /* Set FIFO threshold to 1/4 */
                FLASH_DMA_BASE->FCR |= (0x0 << 0);
                /* Set peripheral address to data we want to write */
                FLASH_DMA_BASE->PAR = (uint32_t)data_to_write;
                /* Set memory address to some place */
                FLASH_DMA_BASE->M0AR = (uint32_t)FLASH_START_ADDR;
                /* Set number of items to transfer divided by 4 because each
                 * datum 32 bits wide */
                FLASH_DMA_BASE->NDTR = data_to_write_size / 4; 
    
                /* Enable DMA2_Stream0 interrupt */
                NVIC_EnableIRQ(FLASH_DMA_BASE_IRQn);

                /* Enable DMA2 */
                FLASH_DMA_BASE->CR |= DMA_SxCR_EN;

                /* request to write has been acknowledged, clear this bit */
                flash_state &= ~FLASH_CMD_WRITE_REQUEST;
            }
            /* Erase is done, clear that bit */
            flash_state &= ~FLASH_CMD_ERASE_IN_PROGRESS;
        }
    }
}

/* Called when write operation is finished */
void __attribute__((optimize("O0"))) FLASH_DMA_BASE_IRQHandler(void)
{
    NVIC_ClearPendingIRQ(FLASH_DMA_BASE_IRQn);
    if (DMA2->HISR & DMA_HISR_TCIF()) {
        /* Clear interrupt */
        DMA2->HIFCR |= DMA_HIFCR_CTCIF();
        /* Disable DMA interrupt */
        NVIC_DisableIRQ(FLASH_DMA_BASE_IRQn);
        /* Disable DMA. */
        FLASH_DMA_BASE->CR &= ~DMA_SxCR_EN;
        if (flash_state & FLASH_CMD_WRITE_IN_PROGRESS) {
            /* Toggle GPIO */
//            GPIOG->ODR ^= (1 << GPIOG_PIN);
            /* wait for flash to be free */
//            while (FLASH->SR & FLASH_SR_BSY);
            /* Reset program bit */
            FLASH->CR &= ~FLASH_CR_PG;
            /* lock that flash */
            FLASH->CR |= FLASH_CR_LOCK;
            /* Set data_to_write to NULL */
            data_to_write = NULL;
            data_to_write_size = 0;
            /* Reset flash writing flag */
            flash_state &= ~FLASH_CMD_WRITE_IN_PROGRESS;
        }
    }
    if (DMA2->HISR & DMA_HISR_TEIF()) {
        /* Transfer error occurred */
        /* Clear interrupt */
        DMA2->HIFCR |= DMA_HIFCR_CTEIF();
    }
}
