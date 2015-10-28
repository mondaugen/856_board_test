#ifndef ERR_H
#define ERR_H 

#include "stm32f4xx.h" 

#define ERR_PIN_PORT     GPIOG
#define ERR_PIN_BIT      9 
#define ERR_PIN_BUS_ENR   AHB1ENR
#define ERR_PIN_PORT_ENR RCC_AHB1ENR_GPIOGEN;

#define err_pin_setup()\
    RCC->ERR_PIN_BUS_ENR |= ERR_PIN_PORT_ENR;\
    ERR_PIN_PORT->MODER &= ~(0x3 << (ERR_PIN_BIT*2));\
    ERR_PIN_PORT->MODER |= 0x1 << (ERR_PIN_BIT*2);\
    ERR_PIN_PORT->OSPEEDR |= 0x3 << (ERR_PIN_BIT*2);\
    ERR_PIN_PORT->PUPDR &= ~(0x3 << (ERR_PIN_BIT*2));\
    ERR_PIN_PORT->ODR &= ~(1 << ERR_PIN_BIT)

#define err_pin_set()   ERR_PIN_PORT->ODR |= 1 << ERR_PIN_BIT;
#define err_pin_reset() ERR_PIN_PORT->ODR &= ~(1 << ERR_PIN_BIT);
#define err_pin_tog()   ERR_PIN_PORT->ODR ^= 1 << ERR_PIN_BIT;

extern void HardFault_Handler(void);

static inline void __throw_err(char *c)
{
    HardFault_Handler();
}

#define THROW_ERR(MSG) __throw_err(MSG)

#endif /* ERR_H */
