#ifndef POLY_MANAGEMENT_H
#define POLY_MANAGEMENT_H 

#include "mm_poly_voice_manage.h"
#include "mmpv_tesp.h" 

extern MMPolyManager *pvm;
extern uint32_t voiceAllocator; 
void pm_yield_params_to_allocator(void *allocator, void *params);
void pm_claim_params_from_allocator(void *allocator, void *params);
int pm_get_next_free_voice_number(void);
void pm_do_for_each_busy_voice(void *allocator, void (*voice_cb)(void *params));

#endif /* POLY_MANAGEMENT_H */
