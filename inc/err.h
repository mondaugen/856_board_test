#ifndef ERR_H
#define ERR_H 

extern void HardFault_Handler(void);

static inline void __throw_err(char *c)
{
    HardFault_Handler();
}

#define THROW_ERR(MSG) __throw_err(MSG)

#endif /* ERR_H */
