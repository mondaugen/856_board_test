/* Implementation of preset recalling when the standard i/o functionality is
 * available like fwrite fread etc. */
#include "presets_lowlevel.h"
#include "flash_commanding.h" 
#include <stdlib.h> 
#include <stdio.h> 
#include <string.h> 

/* There's no need to initialize flash really. */
presets_lowlevel_err_t presets_lowlevel_init(presets_lowlevel_handle_t
        **handle,void *opt)
{
    return(0);
}

/* Size must be a multiple of 4. This reading does not use DMA so it should only
 * be called on initialization. */
presets_lowlevel_err_t presets_lowlevel_read(presets_lowlevel_handle_t *handle,
        void *dest, uint32_t size, void *opt)
{
    if (size % 4) {
        return(-1);
    }
    /* Each datum read is a mulitple of 4 bytes so we divide by 4 to get the
     * number of reads needed */
    size /= 4;
    uint32_t *ptr = (uint32_t*)FLASH_START_ADDR;
    uint32_t *dest_ptr = (uint32_t*)dest;
    while (size--) {
        *dest_ptr++ = *ptr++;
    }
    return(0);
}

presets_lowlevel_err_t presets_lowlevel_write(presets_lowlevel_handle_t *handle,
        void *src, uint32_t size, void *opt)
{
    if (size % 4) {
        return (-1);
    }
    flash_request_write(src,size);
    return(0);
}
