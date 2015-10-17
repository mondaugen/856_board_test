#include "switches.h"
#include "switch_control.h"
#include "synth_control.h" 

/* This structure is a subclass of a switch_control_t struct but includes two
 * additional port_addr and port_bit fields so that the state of two pins can be
 * checked. */
typedef struct __synth_switch_control_t {
    switch_control_t head;
    volatile uint32_t *port_addr;
    uint32_t port_bit;
} synth_switch_control_t;

#define SYNTH_SWITCH_CONTROL(type,fun,c0,c1,c2)\
    static void synth_switch_control_ ## type ## _control(switch_control_t *sc)\
    {\
        if (!switch_control_get_state(sc)) {\
            fun(c0);\
        } else if (!switch_control_get_state((synth_switch_control_t*)sc)) {\
            fun(c2);\
        } else {\
            fun(c1);\
        }\
    }

#define SYNTH_SWITCH_SETUP(type,sw)\
    static void synth_switch_control_ ## type ## _setup(void)\
    {\
        static synth_switch_control_t _switch_control;\
        volatile uint32_t *_sw_addrs[] = {\
            sw ## _TOP_ADDR,\
            sw ## _BTM_ADDR};\
        uint32_t _sw_pins[] = {\
            sw ## _TOP_PORT_PIN,\
            sw ## _BTM_PORT_PIN};\
        switch_control_init((switch_control_t*)&_switch_control,\
                _sw_addrs[1],/* Because btm is when the head of switch is up. */\
                _sw_pins[1],\
                synth_switch_control_ ## type ## _control,\
                NULL);\
        _switch_control.port_addr = _sw_addrs[0];\
        _switch_control.port_bit  = _sw_pins[0];\
        switch_control_add((switch_control_t*)&_switch_control);\
    }

#define SYNTH_SWITCH_SETUP_CALL(type)\
    synth_switch_control_ ## type ## _setup()

#define SYNTH_SWITCH_CONTROL_TOG(name)\
    void synth_switch_control_ ## name ## _tog_func(switch_debouncer_t *sd)\
    {\
        synth_control_ ## name ## _tog();\
    }

#define SYNTH_SWITCH_SETUP_TOG(name,sw)\
    static void synth_switch_control_ ## name ## _tog_setup(void)\
    {\
        static mom_state_t mom_state = {\
            sw ## _TOG_ADDR,\
            sw ## _TOG_PORT_PIN,\
            sw ## _ADDR,\
            sw ## _PORT_PIN\
        };\
        static switch_debouncer_t debouncer;\
        static switch_control_t control;\
        switch_debouncer_init(&debouncer,\
                synth_switch_control_ ## name ## _tog_func,\
                1,\
                &mom_state);\
        switch_control_debounce_init(&control,&debouncer);\
        switch_control_add(&control);\
    }

#define SYNTH_SWITCH_SETUP_TOG_CALL(name)\
   synth_switch_control_ ## name ## _tog_setup() 


SYNTH_SWITCH_CONTROL(SynthControlEditingWhichParamsIndex,
        synth_control_set_editingWhichParams,0,1,2);
SYNTH_SWITCH_SETUP(SynthControlEditingWhichParamsIndex,SW2);

SYNTH_SWITCH_CONTROL(SynthControlPresetNumber,
        synth_control_set_presetNumber,0,1,2);
SYNTH_SWITCH_SETUP(SynthControlPresetNumber,
        SW5);
SYNTH_SWITCH_CONTROL(SynthControlPosMode,
        synth_control_set_posMode,
        SynthControlPosMode_ABSOLUTE,
        SynthControlPosMode_STRIDE,
        SynthControlPosMode_UNKNOWN);
SYNTH_SWITCH_SETUP(SynthControlPosMode,SW1);
SYNTH_SWITCH_CONTROL(SynthControlDeltaButtonMode,
        synth_control_set_deltaButtonMode,
        SynthControlDeltaButtonMode_EVENT_DELTA_FREE,
        SynthControlDeltaButtonMode_EVENT_DELTA_QUANT,
        SynthControlDeltaButtonMode_INTERMITTENCY);
SYNTH_SWITCH_SETUP(SynthControlDeltaButtonMode,SW4);
SYNTH_SWITCH_CONTROL(SynthControlPitchMode,
        synth_control_set_pitchMode,
        SynthControlPitchMode_CHROM,
        SynthControlPitchMode_4TH5TH,
        SynthControlPitchMode_ARP);
SYNTH_SWITCH_SETUP(SynthControlPitchMode,SW8);
SYNTH_SWITCH_CONTROL(SynthControlRecMode,
        synth_control_set_recMode,
        SynthControlRecMode_NORMAL,
        SynthControlRecMode_REC_LEN_1_BEAT,
        SynthControlRecMode_REC_LEN_1_BEAT_REC_SCHED);
SYNTH_SWITCH_SETUP(SynthControlRecMode,SW6);
        /* Incase the switch gets caught down momentarily when toggling the
         * feedback, the gain mode will stay in wet mode. */
SYNTH_SWITCH_CONTROL(SynthControlGainMode,
        synth_control_set_gainMode,
        SynthControlGainMode_FADE,
        SynthControlGainMode_WET,
        SynthControlGainMode_WET); 
SYNTH_SWITCH_SETUP(SynthControlGainMode,SW7);
SYNTH_SWITCH_CONTROL_TOG(record);
SYNTH_SWITCH_SETUP_TOG(record,FSW1);
SYNTH_SWITCH_CONTROL_TOG(schedulerState);
SYNTH_SWITCH_SETUP_TOG(schedulerState,FSW2);
SYNTH_SWITCH_CONTROL_TOG(presetRecall);
SYNTH_SWITCH_SETUP_TOG(presetRecall,MSW3_TOP);
SYNTH_SWITCH_CONTROL_TOG(presetStore);
SYNTH_SWITCH_SETUP_TOG(presetStore,MSW3_BTM);
SYNTH_SWITCH_CONTROL_TOG(feedback);
SYNTH_SWITCH_SETUP_TOG(feedback,MSW7_TOP);

void synth_switch_control_setup(void)
{
    SYNTH_SWITCH_SETUP_CALL(SynthControlEditingWhichParamsIndex);
    SYNTH_SWITCH_SETUP_CALL(SynthControlPresetNumber);
    SYNTH_SWITCH_SETUP_CALL(SynthControlPosMode);
    SYNTH_SWITCH_SETUP_CALL(SynthControlDeltaButtonMode);
    SYNTH_SWITCH_SETUP_CALL(SynthControlPitchMode);
    SYNTH_SWITCH_SETUP_CALL(SynthControlRecMode);
    SYNTH_SWITCH_SETUP_CALL(SynthControlGainMode);
    SYNTH_SWITCH_SETUP_TOG_CALL(record);
    SYNTH_SWITCH_SETUP_TOG_CALL(schedulerState);
    SYNTH_SWITCH_SETUP_TOG_CALL(presetRecall);
    SYNTH_SWITCH_SETUP_TOG_CALL(presetStore);
    SYNTH_SWITCH_SETUP_TOG_CALL(feedback);
}
