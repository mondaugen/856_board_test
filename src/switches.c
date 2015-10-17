#include <stddef.h> 
#include "switches.h" 

static uint32_t (*sw_get_state_funcs[]) (void) = {
    fsw1_get_state,
    fsw2_get_state,
    sw1_top_get_state,
    sw1_btm_get_state,
    sw2_top_get_state,
    sw2_btm_get_state,
    sw3_top_get_state,
    sw3_btm_get_state,
    sw4_top_get_state,
    sw4_btm_get_state,
    sw5_top_get_state,
    sw5_btm_get_state,
    sw6_top_get_state,
    sw6_btm_get_state,
    sw7_top_get_state,
    sw7_btm_get_state,
    sw8_top_get_state,
    sw8_btm_get_state,
    NULL
};

/* The footswitches are momentary. This pseudo-register is set when the
 * footswitch is pressed the first time, and reset the second time. It can be
 * passed to functions expecting a register. The bits that are set or reset are
 * defined in switches.h */
volatile uint32_t fsw_toggle_states;

void switches_setup(void)
{
    uint32_t enrs[] = {
        FSW1_ENR,
        FSW2_ENR, 
        SW1_TOP_ENR,
        SW1_BTM_ENR,
        SW2_TOP_ENR,
        SW2_BTM_ENR,
        SW3_TOP_ENR,
        SW3_BTM_ENR,
        SW4_TOP_ENR,
        SW4_BTM_ENR,
        SW5_TOP_ENR,
        SW5_BTM_ENR,
        SW6_TOP_ENR,
        SW6_BTM_ENR,
        SW7_TOP_ENR,
        SW7_BTM_ENR,
        SW8_TOP_ENR,
        SW8_BTM_ENR,
    };
        
    GPIO_TypeDef *gpios[] = {
        FSW1_PORT,
        FSW2_PORT,
        SW1_TOP_PORT,
        SW1_BTM_PORT,
        SW2_TOP_PORT,     
        SW2_BTM_PORT,     
        SW3_TOP_PORT,     
        SW3_BTM_PORT,     
        SW4_TOP_PORT,     
        SW4_BTM_PORT,     
        SW5_TOP_PORT,     
        SW5_BTM_PORT,     
        SW6_TOP_PORT,     
        SW6_BTM_PORT,     
        SW7_TOP_PORT,     
        SW7_BTM_PORT,     
        SW8_TOP_PORT,     
        SW8_BTM_PORT,
        NULL
    };    

    uint32_t pins[] = {
        FSW1_PORT_PIN,
        FSW2_PORT_PIN,
        SW1_TOP_PORT_PIN,
        SW1_BTM_PORT_PIN,
        SW2_TOP_PORT_PIN,
        SW2_BTM_PORT_PIN,
        SW3_TOP_PORT_PIN, 
        SW3_BTM_PORT_PIN,
        SW4_TOP_PORT_PIN,
        SW4_BTM_PORT_PIN,
        SW5_TOP_PORT_PIN,
        SW5_BTM_PORT_PIN,
        SW6_TOP_PORT_PIN,
        SW6_BTM_PORT_PIN,
        SW7_TOP_PORT_PIN,
        SW7_BTM_PORT_PIN,
        SW8_TOP_PORT_PIN,
        SW8_BTM_PORT_PIN,
    };

    uint32_t *enr = enrs, *pin = pins;
    GPIO_TypeDef **gpio = gpios;
    while (1) {
        /* Enable GPIO clock */
        RCC->AHB1ENR |= *enr;
        /* Set mode to input */
        (*gpio)->MODER &= ~(0x3 << (*pin)*2);
        (*gpio)->PUPDR &= ~(0x3 << (*pin)*2);
        /* Set to pull-top */
        (*gpio)->PUPDR |= (0x1 << (*pin)*2);
        gpio++;
        if (!(*gpio)) {
            break;
        }
        enr++;
        pin++;
    }

    fsw_toggle_states = 0;
    /* enable SYSCFG */
    RCC->APB2ENR        |= RCC_APB2ENR_SYSCFGEN;
    /* set up EXTI interrupts */
    FSW1_EXTICR         &= ~FSW1_EXTI_PIN;
    FSW1_EXTICR         |= FSW1_EXTI_PIN_PORT;
    FSW2_EXTICR         &= ~FSW2_EXTI_PIN;
    FSW2_EXTICR         |= FSW2_EXTI_PIN_PORT;
    MSW3_TOP_EXTICR     &= ~MSW3_TOP_EXTI_PIN;
    MSW3_TOP_EXTICR     |= MSW3_TOP_EXTI_PIN_PORT;
    MSW3_BTM_EXTICR     &= ~MSW3_BTM_EXTI_PIN;
    MSW3_BTM_EXTICR     |= MSW3_BTM_EXTI_PIN_PORT;
    MSW7_TOP_EXTICR     &= ~MSW7_TOP_EXTI_PIN;
    MSW7_TOP_EXTICR     |= MSW7_TOP_EXTI_PIN_PORT;
    FSW1_EXTI->IMR      |= (0x1 << FSW1_PORT_PIN);
    FSW2_EXTI->IMR      |= (0x1 << FSW2_PORT_PIN);
    MSW3_TOP_EXTI->IMR  |= (0x1 << MSW3_TOP_PORT_PIN);
    MSW3_BTM_EXTI->IMR  |= (0x1 << MSW3_BTM_PORT_PIN);
    MSW7_TOP_EXTI->IMR  |= (0x1 << MSW7_TOP_PORT_PIN);
    FSW1_EXTI->FTSR     |= (0x1 << FSW1_PORT_PIN);
    FSW2_EXTI->FTSR     |= (0x1 << FSW2_PORT_PIN);
    MSW3_TOP_EXTI->FTSR |= (0x1 << MSW3_TOP_PORT_PIN);
    MSW3_BTM_EXTI->FTSR |= (0x1 << MSW3_BTM_PORT_PIN);
    MSW7_TOP_EXTI->FTSR |= (0x1 << MSW7_TOP_PORT_PIN);
    NVIC_EnableIRQ(FSW1_IRQ_N);
    NVIC_EnableIRQ(FSW2_IRQ_N);
    NVIC_EnableIRQ(MSW3_TOP_IRQ_N);
    NVIC_EnableIRQ(MSW3_BTM_IRQ_N);
    NVIC_EnableIRQ(MSW7_TOP_IRQ_N);
}

void FSW1_IRQ_HANDLER (void)
{
    NVIC_ClearPendingIRQ(FSW1_IRQ_N);
    if (FSW1_EXTI->PR & (0x1 << FSW1_PORT_PIN)) {
        FSW1_EXTI->PR |= 0x1 << FSW1_PORT_PIN;
        fsw_toggle_states |= (0x1 << FSW1_TOG_PORT_PIN);
    }
}

void FSW2_IRQ_HANDLER (void)
{
    NVIC_ClearPendingIRQ(FSW2_IRQ_N);
    if (FSW2_EXTI->PR & (0x1 << FSW2_PORT_PIN)) {
        FSW2_EXTI->PR |= 0x1 << FSW2_PORT_PIN;
        fsw_toggle_states |= (0x1 << FSW2_TOG_PORT_PIN);
    }
}

void MSW3_TOP_IRQ_HANDLER (void)
{
    NVIC_ClearPendingIRQ(MSW3_TOP_IRQ_N);
    if (MSW3_TOP_EXTI->PR & (0x1 << MSW3_TOP_PORT_PIN)) {
        MSW3_TOP_EXTI->PR |= 0x1 << MSW3_TOP_PORT_PIN;
        fsw_toggle_states |= (0x1 << MSW3_TOP_TOG_PORT_PIN);
    }
}

void MSW3_BTM_IRQ_HANDLER (void)
{
    NVIC_ClearPendingIRQ(MSW3_BTM_IRQ_N);
    if (MSW3_BTM_EXTI->PR & (0x1 << MSW3_BTM_PORT_PIN)) {
        MSW3_BTM_EXTI->PR |= 0x1 << MSW3_BTM_PORT_PIN;
        fsw_toggle_states |= (0x1 << MSW3_BTM_TOG_PORT_PIN);
    }
}

void MSW7_TOP_IRQ_HANDLER (void)
{
    NVIC_ClearPendingIRQ(MSW7_TOP_IRQ_N);
    if (MSW7_TOP_EXTI->PR & (0x1 << MSW7_TOP_PORT_PIN)) {
        MSW7_TOP_EXTI->PR |= 0x1 << MSW7_TOP_PORT_PIN;
        fsw_toggle_states |= (0x1 << MSW7_TOP_TOG_PORT_PIN);
    }
}

uint32_t fsw1_get_state(void)
{
    return (FSW1_PORT->IDR & (0x1 << FSW1_PORT_PIN)) >> FSW1_PORT_PIN;
}
    
uint32_t fsw2_get_state(void)
{
    return (FSW2_PORT->IDR & (0x1 << FSW2_PORT_PIN)) >> FSW2_PORT_PIN;
}

uint32_t sw1_top_get_state(void)
{
    return (SW1_TOP_PORT->IDR & (0x1 << SW1_TOP_PORT_PIN)) >> SW1_TOP_PORT_PIN;
}

uint32_t sw1_btm_get_state(void)
{
    return (SW1_BTM_PORT->IDR & (0x1 << SW1_BTM_PORT_PIN)) >> SW1_BTM_PORT_PIN;
}

uint32_t sw2_top_get_state(void)
{
    return (SW2_TOP_PORT->IDR & (0x1 << SW2_TOP_PORT_PIN)) >> SW2_TOP_PORT_PIN;
}

uint32_t sw2_btm_get_state(void)
{
    return (SW2_BTM_PORT->IDR & (0x1 << SW2_BTM_PORT_PIN)) >> SW2_BTM_PORT_PIN;
}

uint32_t sw3_top_get_state(void)
{
    return (SW3_TOP_PORT->IDR & (0x1 << SW3_TOP_PORT_PIN)) >> SW3_TOP_PORT_PIN;
}

uint32_t sw3_btm_get_state(void)
{
    return (SW3_BTM_PORT->IDR & (0x1 << SW3_BTM_PORT_PIN)) >> SW3_BTM_PORT_PIN;
}

uint32_t sw4_top_get_state(void)
{
    return (SW4_TOP_PORT->IDR & (0x1 << SW4_TOP_PORT_PIN)) >> SW4_TOP_PORT_PIN;
}

uint32_t sw4_btm_get_state(void)
{
    return (SW4_BTM_PORT->IDR & (0x1 << SW4_BTM_PORT_PIN)) >> SW4_BTM_PORT_PIN;
}

uint32_t sw5_top_get_state(void)
{
    return (SW5_TOP_PORT->IDR & (0x1 << SW5_TOP_PORT_PIN)) >> SW5_TOP_PORT_PIN;
}

uint32_t sw5_btm_get_state(void)
{
    return (SW5_BTM_PORT->IDR & (0x1 << SW5_BTM_PORT_PIN)) >> SW5_BTM_PORT_PIN;
}

uint32_t sw6_top_get_state(void)
{
    return (SW6_TOP_PORT->IDR & (0x1 << SW6_TOP_PORT_PIN)) >> SW6_TOP_PORT_PIN;
}

uint32_t sw6_btm_get_state(void)
{
    return (SW6_BTM_PORT->IDR & (0x1 << SW6_BTM_PORT_PIN)) >> SW6_BTM_PORT_PIN;
}

uint32_t sw7_top_get_state(void)
{
    return (SW7_TOP_PORT->IDR & (0x1 << SW7_TOP_PORT_PIN)) >> SW7_TOP_PORT_PIN;
}

uint32_t sw7_btm_get_state(void)
{
    return (SW7_BTM_PORT->IDR & (0x1 << SW7_BTM_PORT_PIN)) >> SW7_BTM_PORT_PIN;
}

uint32_t sw8_top_get_state(void)
{
    return (SW8_TOP_PORT->IDR & (0x1 << SW8_TOP_PORT_PIN)) >> SW8_TOP_PORT_PIN;
}

uint32_t sw8_btm_get_state(void)
{
    return (SW8_BTM_PORT->IDR & (0x1 << SW8_BTM_PORT_PIN)) >> SW8_BTM_PORT_PIN;
}

/* Fills an array of length NUM_SWITCHES with the switch states. The states are
 * in the same order as the order of the defines in switches.h. */
void get_switch_states(uint32_t *states)
{
    uint32_t (**fun) (void);
    for (fun = sw_get_state_funcs; *fun != NULL; fun++) {
        *states = (*(*fun))();
        states++;
    }
}
