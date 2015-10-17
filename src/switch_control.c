#include "switch_control.h" 
#include <stddef.h> 
/* Functions for looking at a GPIO port and calling functions on the value */

static switch_control_t * _switch_controls = NULL;

void switch_control_do_all(void)
{
    switch_control_t *tmp;
    tmp = _switch_controls;
    while (tmp) {
        tmp->func(tmp);
        tmp = tmp->next;
    }
}

void switch_control_add(switch_control_t *sc)
{
    sc->next = _switch_controls;
    _switch_controls = sc;
}

void switch_control_init(switch_control_t *sc,
                         volatile uint32_t *port_addr,
                         uint32_t port_bit,
                         void (*func)(switch_control_t*),
                         void *data)
{
    sc->port_addr = port_addr;
    sc->port_bit = port_bit;
    sc->func = func;
    sc->data = data;
    sc->next = NULL;
}

static void switch_control_debounce_func(switch_control_t *sc)
{
    switch_debouncer_t *sd = (switch_debouncer_t*)sc->data;
    if (sd->get_req_state(sd)) {
        if (sd->n_ignores) {
            /* Did this request come too soon after a previous acknowledge? If
             * so, the request is bogus. */
            sd->reset_req_state(sd);
        } else if (sd->get_pin_state(sd) == 0) {
            /* The pin is low. */
            /* Pin has been brought to its rest (reset) position again */
            sd->func(sd);
            sd->n_ignores = sd->init_n_ignores;
            sd->reset_req_state(sd);
        } 
        /* Otherwise we are still waiting for the pin to go low. */
    }
    if (sd->n_ignores) {
        sd->n_ignores--;
    }
}

static uint32_t get_mom_req_state(switch_debouncer_t *sd)
{
    mom_state_t* data = (mom_state_t*)sd->data;
    return (*(data->req_state_addr) >> data->req_state_bit) & 0x1;
}

static uint32_t get_mom_pin_state(switch_debouncer_t *sd)
{
    /* Because the pins are active when low, we reverse polarity here */
    mom_state_t* data = (mom_state_t*)sd->data;
    return !((*(data->pin_state_addr) >> data->pin_state_bit) & 0x1);
}

static void reset_mom_req_state(switch_debouncer_t *sd)
{
    mom_state_t* data = (mom_state_t*)sd->data;
    *(data->req_state_addr) &= ~(0x1 << data->req_state_bit);
}

void switch_debouncer_init(switch_debouncer_t *sd,
                           void (*func)(switch_debouncer_t *),
                           uint32_t init_n_ignores,
                           mom_state_t *state)
{
        sd->get_req_state = get_mom_req_state;
        sd->get_pin_state = get_mom_pin_state;
        sd->reset_req_state = reset_mom_req_state;
        sd->func = func;
        sd->init_n_ignores = init_n_ignores;
        sd->n_ignores = 0;
        sd->data = (void*)state;
}

void switch_control_debounce_init(switch_control_t *sc,
                                  switch_debouncer_t *sd)
{
    switch_control_init(sc,
                        (volatile uint32_t*)NULL,
                        0,
                        switch_control_debounce_func,
                        (void*)sd);
}
