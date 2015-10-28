#include "leds.h" 

static uint32_t leds_enrs[] = {
    LED1_ENR,
    LED3_ENR,
    LED5_ENR,
    LED7_ENR,   
    AUX_LED1_ENR,
    AUX_LED2_ENR,
    AUX_LED3_ENR,
    0
};

static GPIO_TypeDef *leds_ports[] = {
    LED1_PORT,
    LED3_PORT,
    LED5_PORT,
    LED7_PORT,   
    AUX_LED1_PORT,
    AUX_LED2_PORT,
    AUX_LED3_PORT,
    0
};

static uint32_t leds_port_pins[] = {
    LED1_PORT_PIN,
    LED3_PORT_PIN,
    LED5_PORT_PIN,
    LED7_PORT_PIN,   
    AUX_LED1_PORT_PIN,
    AUX_LED2_PORT_PIN,
    AUX_LED3_PORT_PIN,
    0
};

void leds_setup(void)
{
    uint32_t *ptr;
    ptr = leds_enrs;
    while (*ptr != 0) {
        RCC->AHB1ENR |= *ptr;
        ptr++;
    }
    ptr = leds_port_pins;
    GPIO_TypeDef **gpio_ptr = leds_ports;
    while (*ptr != 0) {
        (*gpio_ptr)->MODER &= ~(0x3 << (*ptr)*2);
        (*gpio_ptr)->MODER |= (0x1 << (*ptr)*2);
        gpio_ptr++;
        ptr++;
    }
}

void aux_led1_set(void)
{
    AUX_LED1_PORT->ODR |= 0x1 << AUX_LED1_PORT_PIN;
}
    
void aux_led1_reset(void)
{
    AUX_LED1_PORT->ODR &= ~(0x1 << AUX_LED1_PORT_PIN);
}

void aux_led1_tog(void)
{
    AUX_LED1_PORT->ODR ^= 0x1 << AUX_LED1_PORT_PIN;
}

void aux_led2_set(void)
{
    AUX_LED2_PORT->ODR |= 0x1 << AUX_LED2_PORT_PIN;
}
    
void aux_led2_reset(void)
{
    AUX_LED2_PORT->ODR &= ~(0x1 << AUX_LED2_PORT_PIN);
}

void aux_led2_tog(void)
{
    AUX_LED2_PORT->ODR ^= 0x1 << AUX_LED2_PORT_PIN;
}

void aux_led3_set(void)
{
    AUX_LED3_PORT->ODR |= 0x1 << AUX_LED3_PORT_PIN;
}
    
void aux_led3_reset(void)
{
    AUX_LED3_PORT->ODR &= ~(0x1 << AUX_LED3_PORT_PIN);
}

void aux_led3_tog(void)
{
    AUX_LED3_PORT->ODR ^= 0x1 << AUX_LED3_PORT_PIN;
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
