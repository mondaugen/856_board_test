#ifndef SWITCH_CONTROL_H
#define SWITCH_CONTROL_H 

#include <stdint.h> 

typedef struct __switch_control_t {
    /* The address of the GPIO port that toggles */
    volatile uint32_t *port_addr;
    /* Bit of the port */
    uint32_t port_bit;
    /* A function that will be called. It will be passed the state of the pin
     * (uint32_t) and a pointer to some data (void*) that it will maybe want to
     * use. */
    void (*func)(struct __switch_control_t *);
    /* Some auxiliary data that func can have access to */
    void *data;
    /* The next one in the list. This should be NULL if last in the list. */
    struct __switch_control_t *next;
} switch_control_t;

typedef struct __switch_debouncer_t {
    /* Should return 1 if request has been made */
    uint32_t (*get_req_state)(struct __switch_debouncer_t *);
    /* Should return 1 if pin is high, so for a momentary switch that means the
     * state that it is in when untouched. */
    uint32_t (*get_pin_state)(struct __switch_debouncer_t *);
    /* Resets the request state */
    void     (*reset_req_state)(struct __switch_debouncer_t *);
    /* What to do when a request has been verified and acknowledged */
    void     (*func)(struct __switch_debouncer_t *);
    /* The number of ignores that are specified after an acknowledged request */
    uint32_t init_n_ignores;
    /* The number of request that are to be ignored from now on */
    uint32_t n_ignores;
    void     *data;
} switch_debouncer_t;

typedef struct __mom_state_t {
    volatile uint32_t *req_state_addr;
    uint32_t           req_state_bit;
    volatile uint32_t *pin_state_addr;
    uint32_t           pin_state_bit;
} mom_state_t;

void switch_control_do_all(void);
void switch_control_init(switch_control_t *sc,
                         volatile uint32_t *port_addr,
                         uint32_t port_bit,
                         void (*func)(switch_control_t*),
                         void *data);
void switch_control_add(switch_control_t *sc);
void switch_control_debounce_init(switch_control_t *sc,
                                  switch_debouncer_t *sd);
void switch_debouncer_init(switch_debouncer_t *sd,
                           void (*func)(switch_debouncer_t *),
                           uint32_t init_n_ignores,
                           mom_state_t *state);

#define switch_control_get_state(ctl) ((*((ctl)->port_addr) >> (ctl)->port_bit) \
        & 0x1)
#define switch_control_set_state(ctl,state)\
    *((ctl)->port_addr) &= ~(0x1  << (ctl)->port_bit);\
    *((ctl)->port_addr) |= (state << (ctl)->port_bit)

#endif /* SWITCH_CONTROL_H */
