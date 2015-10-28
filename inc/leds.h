#ifndef LEDS_H
#define LEDS_H 

#include "stm32f4xx.h"

#define LED1_ENR      RCC_AHB1ENR_GPIOEEN 
#define LED3_ENR      RCC_AHB1ENR_GPIOEEN 
#define LED5_ENR      RCC_AHB1ENR_GPIOEEN 
#define LED7_ENR      RCC_AHB1ENR_GPIOEEN 
#define AUX_LED1_ENR  RCC_AHB1ENR_GPIOGEN 
#define AUX_LED2_ENR  RCC_AHB1ENR_GPIOAEN 
#define AUX_LED3_ENR  RCC_AHB1ENR_GPIOAEN 

#define LED1_PORT     GPIOE
#define LED3_PORT     GPIOE
#define LED5_PORT     GPIOE
#define LED7_PORT     GPIOE
#define AUX_LED1_PORT GPIOG
#define AUX_LED2_PORT GPIOA
#define AUX_LED3_PORT GPIOA

#define LED1_PORT_PIN 3 
#define LED3_PORT_PIN 2 
#define LED5_PORT_PIN 5 
#define LED7_PORT_PIN 4 
#define AUX_LED1_PORT_PIN 9 
#define AUX_LED2_PORT_PIN 9
#define AUX_LED3_PORT_PIN 10

void leds_setup(void);
void led1_set(void);
void led1_reset(void);
void led3_set(void);
void led3_reset(void);
void led5_set(void);
void led5_reset(void);
void led7_set(void);
void led7_reset(void);
void led1_tog(void);
void led3_tog(void);
void led5_tog(void);
void led7_tog(void);
void aux_led1_set(void);
void aux_led1_reset(void);
void aux_led1_tog(void);
void aux_led2_set(void);
void aux_led2_reset(void);
void aux_led2_tog(void);
void aux_led3_set(void);
void aux_led3_reset(void);
void aux_led3_tog(void);

#endif /* LEDS_H */
