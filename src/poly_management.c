#include "poly_management.h"
#include "signal_chain.h" 

/* Store the voice states for up to 32 voices */
uint32_t voiceAllocator = 0xffffffff; 

MMPolyManager *pvm;

void poly_management_setup(void)
{
    /* Make poly voice manager */
    pvm = MMPolyManager_new(NUM_NOTES);
    int n;
    for (n = 0; n < NUM_NOTES; n++) {
        /* Make new poly voice and add it to the poly voice manager */
        MMPolyManager_addVoice(pvm, n, (MMPolyVoice*)MMPvtesp_new(&spsps[n])); 
    }
}

#define set_voice_busy(vs,n) (vs) &= ~(1 << (n))
#define set_voice_free(vs,n) (vs) |= (1 << (n))

void pm_yield_params_to_allocator(void *allocator, void *params)
{
    /* caller will give a value of type (MMSample*)
     * to yield to the allocator */
    set_voice_free(*((uint32_t*)allocator),(uint32_t)*((MMSample*)params));
}

void pm_claim_params_from_allocator(void *allocator, void *params)
{
    /* mmpv_tesp will give a value of type (MMSample*)
     * to yield to the allocator */
    set_voice_busy(*((uint32_t*)allocator),(uint32_t)*((MMSample*)params));
}

int pm_get_next_free_voice_number(void)
{
    int n;
    for (n = 0; n < NUM_NOTES; n++) {
        if ((voiceAllocator >> n) & 0x1) {
            return n;
        }
    }
    return -1; /* no voices free */
}

/* Call the function voice_cb on each busy voice. This implementation will pass
 * a pointer to a value of type int to voice_cb */
void pm_do_for_each_busy_voice(void *allocator, void (*voice_cb)(void *params))
{
    int n;
    for (n = 0; n < NUM_NOTES; n++) {
        if (~(*((uint32_t*)allocator) >> n) & 0x1) {
            /* This is a busy voice, call voice_cb on it */
            voice_cb(&n);
        }
    }
}
