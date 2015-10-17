#include <stdint.h>
#include <stddef.h> 
#include "switch_control_test.h" 
#include "switches.h"
#include "switch_control.h" 
#include "leds.h" 

switch_control_t switch_control[4];
uint32_t         switch_data[] = {1,3,5,7};
uint32_t         footswitch_data[] = {1,2};

static void switch_control_test_func(switch_control_t *sc)
{
    /* Turn light on when switch state goes low. */
    uint32_t data = *((uint32_t*)sc->data);
    switch (data) {
        case 1:
            if (!switch_control_get_state(sc)) {
                led1_set();
            } else {
                led1_reset();
            }
            break;
        case 3:
            if (!switch_control_get_state(sc)) {
                led3_set();
            } else {
                led3_reset();
            }
            break;
        case 5:
            if (!switch_control_get_state(sc)) {
                led5_set();
            } else {
                led5_reset();
            }
            break;
        case 7:
            if (!switch_control_get_state(sc)) {
                led7_set();
            } else {
                led7_reset();
            }
            break;
    }
}

static void switch_control_test_fsw_func(switch_control_t *sc)
{
    /* Turn light on when switch state goes low. */
    static uint32_t left_toggle, right_toggle;
    uint32_t data = *((uint32_t*)sc->data);
    switch (data) {
        case 1:
            if (switch_control_get_state(sc)) {
                left_toggle ^= 0x1;
                switch_control_set_state(sc,0x0);
            }
            break;
        case 2:
            if (switch_control_get_state(sc)) {
                right_toggle ^= 0x1;
                switch_control_set_state(sc,0x0);
            }
            break;
    }
    if (left_toggle) {
        led1_reset();
        led3_set();
    } else {
        led1_set();
        led3_reset();
    }
    if (right_toggle) {
        led5_reset();
        led7_set();
    } else {
        led5_set();
        led7_reset();
    }
}

typedef struct __fsw_state_t {
    volatile uint32_t *req_state_addr;
    uint32_t           req_state_bit;
    volatile uint32_t *pin_state_addr;
    uint32_t           pin_state_bit;
} fsw_state_t;

static uint32_t get_fsw_req_state(switch_debouncer_t *sd)
{
    fsw_state_t* data = (fsw_state_t*)sd->data;
    return (*(data->req_state_addr) >> data->req_state_bit) & 0x1;
}

static uint32_t get_fsw_pin_state(switch_debouncer_t *sd)
{
    /* Because the pins are active when low, we reverse polarity here */
    fsw_state_t* data = (fsw_state_t*)sd->data;
    return !((*(data->pin_state_addr) >> data->pin_state_bit) & 0x1);
}

static void reset_fsw_req_state(switch_debouncer_t *sd)
{
    fsw_state_t* data = (fsw_state_t*)sd->data;
    *(data->req_state_addr) &= ~(0x1 << data->req_state_bit);
}

static void fsw1_func(switch_debouncer_t *sd)
{
    led1_tog();
}

static void fsw2_func(switch_debouncer_t *sd)
{
    led3_tog();
}

static void msw3_top_func(switch_debouncer_t *sd)
{
    led5_tog();
}

static void msw3_btm_func(switch_debouncer_t *sd)
{
    led7_tog();
}

void switch_control_test_setup(void)
{
#if defined SWITCH_CONTROL_TEST_1
    switch_control_init(&switch_control[0],
                        SW1_TOP_ADDR,
                        SW1_TOP_PORT_PIN,
                        switch_control_test_func,
                        (void*)&switch_data[0]);
    switch_control_init(&switch_control[1],
                        SW1_BTM_ADDR,
                        SW1_BTM_PORT_PIN,
                        switch_control_test_func,
                        (void*)&switch_data[1]);
    switch_control_init(&switch_control[2],
                        SW2_TOP_ADDR,
                        SW2_TOP_PORT_PIN,
                        switch_control_test_func,
                        (void*)&switch_data[2]);
    switch_control_init(&switch_control[3],
                        SW2_BTM_ADDR,
                        SW2_BTM_PORT_PIN,
                        switch_control_test_func,
                        (void*)&switch_data[3]);
#elif defined SWITCH_CONTROL_TEST_2
    switch_control_init(&switch_control[0],
                        SW3_TOP_ADDR,
                        SW3_TOP_PORT_PIN,
                        switch_control_test_func,
                        (void*)&switch_data[0]);
    switch_control_init(&switch_control[1],
                        SW3_BTM_ADDR,
                        SW3_BTM_PORT_PIN,
                        switch_control_test_func,
                        (void*)&switch_data[1]);
    switch_control_init(&switch_control[2],
                        SW4_TOP_ADDR,
                        SW4_TOP_PORT_PIN,
                        switch_control_test_func,
                        (void*)&switch_data[2]);
    switch_control_init(&switch_control[3],
                        SW4_BTM_ADDR,
                        SW4_BTM_PORT_PIN,
                        switch_control_test_func,
                        (void*)&switch_data[3]);
#elif defined SWITCH_CONTROL_TEST_3
    switch_control_init(&switch_control[0],
                        SW5_TOP_ADDR,
                        SW5_TOP_PORT_PIN,
                        switch_control_test_func,
                        (void*)&switch_data[0]);
    switch_control_init(&switch_control[1],
                        SW5_BTM_ADDR,
                        SW5_BTM_PORT_PIN,
                        switch_control_test_func,
                        (void*)&switch_data[1]);
    switch_control_init(&switch_control[2],
                        SW6_TOP_ADDR,
                        SW6_TOP_PORT_PIN,
                        switch_control_test_func,
                        (void*)&switch_data[2]);
    switch_control_init(&switch_control[3],
                        SW6_BTM_ADDR,
                        SW6_BTM_PORT_PIN,
                        switch_control_test_func,
                        (void*)&switch_data[3]);
#elif defined SWITCH_CONTROL_TEST_4
    switch_control_init(&switch_control[0],
                        SW7_TOP_ADDR,
                        SW7_TOP_PORT_PIN,
                        switch_control_test_func,
                        (void*)&switch_data[0]);
    switch_control_init(&switch_control[1],
                        SW7_BTM_ADDR,
                        SW7_BTM_PORT_PIN,
                        switch_control_test_func,
                        (void*)&switch_data[1]);
    switch_control_init(&switch_control[2],
                        SW8_TOP_ADDR,
                        SW8_TOP_PORT_PIN,
                        switch_control_test_func,
                        (void*)&switch_data[2]);
    switch_control_init(&switch_control[3],
                        SW8_BTM_ADDR,
                        SW8_BTM_PORT_PIN,
                        switch_control_test_func,
                        (void*)&switch_data[3]);
#elif defined SWITCH_CONTROL_TEST_5
    static mom_state_t fsw_states[] = {
        {FSW1_TOG_ADDR, FSW1_TOG_PORT_PIN, FSW1_ADDR, FSW1_PORT_PIN},
        {FSW2_TOG_ADDR, FSW2_TOG_PORT_PIN, FSW2_ADDR, FSW2_PORT_PIN},
        {MSW3_TOP_TOG_ADDR, MSW3_TOP_TOG_PORT_PIN, MSW3_TOP_ADDR,     
            MSW3_TOP_PORT_PIN},
        {MSW3_BTM_TOG_ADDR, MSW3_BTM_TOG_PORT_PIN, MSW3_BTM_ADDR, 
            MSW3_BTM_PORT_PIN}
    };
    static switch_debouncer_t fsw_debouncers[4];
    switch_debouncer_init(&fsw_debouncers[0],fsw1_func,1,&fsw_states[0]);
    switch_debouncer_init(&fsw_debouncers[1],fsw2_func,1,&fsw_states[1]);
    switch_debouncer_init(&fsw_debouncers[2],msw3_top_func,1,&fsw_states[2]);
    switch_debouncer_init(&fsw_debouncers[3],msw3_btm_func,1,&fsw_states[3]);
    /* The addresses and pins passed here are just dummy values. */
    switch_control_debounce_init(&switch_control[0],(void*)&fsw_debouncers[0]);
    switch_control_debounce_init(&switch_control[1],(void*)&fsw_debouncers[1]);
    switch_control_debounce_init(&switch_control[2],(void*)&fsw_debouncers[2]);
    switch_control_debounce_init(&switch_control[3],(void*)&fsw_debouncers[3]);
#elif defined SWITCH_CONTROL_TEST_6
    switch_control_init(&switch_control[0],
                        FSW1_ADDR,
                        FSW1_PORT_PIN,
                        switch_control_test_func,
                        (void*)&switch_data[0]);
    switch_control_init(&switch_control[1],
                        FSW2_ADDR,
                        FSW2_PORT_PIN,
                        switch_control_test_func,
                        (void*)&switch_data[1]);
#endif /* SWITCH_CONTROL_TEST_{1-5} */
#if defined(SWITCH_CONTROL_TEST_1) || defined(SWITCH_CONTROL_TEST_2) \
    || defined(SWITCH_CONTROL_TEST_3) || defined(SWITCH_CONTROL_TEST_4) \
    || defined(SWITCH_CONTROL_TEST_5)
    int n;
    for (n = 0; n < 4; n++) {
        switch_control_add(&switch_control[n]);
    }
#endif
#if defined(SWITCH_CONTROL_TEST_6)
    int n;
    for (n = 0; n < 2; n++) {
        switch_control_add(&switch_control[n]);
    }
#endif
}
