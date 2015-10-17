#include "leds.h" 

void leds_setup(void)
{
    RCC->AHB1ENR |= LED1_ENR | LED3_ENR | LED5_ENR | LED7_ENR;
    LED1_PORT->MODER &= ~(0x3 << LED1_PORT_PIN*2);
    LED1_PORT->MODER |= (0x1 << LED1_PORT_PIN*2);
    LED3_PORT->MODER &= ~(0x3 << LED3_PORT_PIN*2);
    LED3_PORT->MODER |= (0x1 << LED3_PORT_PIN*2);
    LED5_PORT->MODER &= ~(0x3 << LED5_PORT_PIN*2);
    LED5_PORT->MODER |= (0x1 << LED5_PORT_PIN*2);
    LED7_PORT->MODER &= ~(0x3 << LED7_PORT_PIN*2);
    LED7_PORT->MODER |= (0x1 << LED7_PORT_PIN*2);
}

void led1_set(void)
{
    LED1_PORT->ODR |= 0x1 << LED1_PORT_PIN;
}
    
void led1_reset(void)
{
    LED1_PORT->ODR &= ~(0x1 << LED1_PORT_PIN);
}

void led1_tog(void)
{
    LED1_PORT->ODR ^= 0x1 << LED1_PORT_PIN;
}

void led3_set(void)
{
    LED3_PORT->ODR |= 0x1 << LED3_PORT_PIN;
}
    
void led3_reset(void)
{
    LED3_PORT->ODR &= ~(0x1 << LED3_PORT_PIN);
}

void led3_tog(void)
{
    LED3_PORT->ODR ^= 0x1 << LED3_PORT_PIN;
}

void led5_set(void)
{
    LED5_PORT->ODR |= 0x1 << LED5_PORT_PIN;
}
    
void led5_reset(void)
{
    LED5_PORT->ODR &= ~(0x1 << LED5_PORT_PIN);
}

void led5_tog(void)
{
    LED5_PORT->ODR ^= 0x1 << LED5_PORT_PIN;
}

void led7_set(void)
{
    LED7_PORT->ODR |= 0x1 << LED7_PORT_PIN;
}
    
void led7_reset(void)
{
    LED7_PORT->ODR &= ~(0x1 << LED7_PORT_PIN);
}

void led7_tog(void)
{
    LED7_PORT->ODR ^= 0x1 << LED7_PORT_PIN;
}
