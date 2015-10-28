#include "switches.h"
#include "switch_control.h"
#include "synth_control.h" 
#include "err.h" 

int fsw1_num_togs = 0;

void sdt_FSW1_tog_func(switch_debouncer_t *sd)
{
    fsw1_num_togs++;
}

static void sdt_test_FSW1_tog_setup(void)\
{
    static mom_state_t mom_state = {
        FSW1_TOG_ADDR,
        FSW1_TOG_PORT_PIN,
        FSW1_ADDR,
        FSW1_PORT_PIN
    };
    static switch_debouncer_t debouncer;
    static switch_control_t control;
    switch_debouncer_init(&debouncer,
            sdt_FSW1_tog_func,
            1,
            &mom_state);
    switch_control_debounce_init(&control,&debouncer);
    switch_control_add(&control);
}

void sdt_setup(void)
{
    sdt_test_FSW1_tog_setup();
}
