#ifndef FLASH_COMMANDING_H
#define FLASH_COMMANDING_H 
#include <stdint.h>
#include <stddef.h> 

#define FLASH_START_ADDR 0x0810c000
#define FLASH_END_ADDR   0x08110000 
#define FLASH_SECTOR     15
/* Number of bytes that can be written or read at one time */
#define FLASH_ACCESS_SIZE 4

#define FLASH_CMD_WRITE_REQUEST     ((uint32_t)0x00000001)
#define FLASH_CMD_ERASE_REQUEST     ((uint32_t)0x00000002)
#define FLASH_CMD_WRITE_IN_PROGRESS ((uint32_t)0x00000004)
#define FLASH_CMD_ERASE_IN_PROGRESS ((uint32_t)0x00000008)
/* To check if busy do if (flash_state & FLASH_CMD_BUSY) { ... } */
#define FLASH_CMD_BUSY (FLASH_CMD_WRITE_REQUEST\
            |FLASH_CMD_ERASE_REQUEST\
            |FLASH_CMD_WRITE_IN_PROGRESS\
            |FLASH_CMD_ERASE_IN_PROGRESS)
            
extern volatile uint32_t flash_state;
void flash_request_write(char *what, uint32_t length);

#endif /* FLASH_COMMANDING_H */
