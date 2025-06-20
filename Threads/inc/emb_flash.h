#ifndef __EMB_FLASH_H__
#define __EMB_FLASH_H__

#include "main.h"

// h7内部flash分为两个bank，每个bank有8个sector，每个sector大小为128k，这里只用bank1
//sector first address
#define ADDR_SECTOR_0	0x08000000
#define ADDR_SECTOR_1	(ADDR_SECTOR_0 + 128*1024)
#define ADDR_SECTOR_2	(ADDR_SECTOR_1 + 128*1024)
#define ADDR_SECTOR_3	(ADDR_SECTOR_2 + 128*1024)
#define ADDR_SECTOR_4	(ADDR_SECTOR_3 + 128*1024)
#define ADDR_SECTOR_5	(ADDR_SECTOR_4 + 128*1024)
#define ADDR_SECTOR_6	(ADDR_SECTOR_5 + 128*1024)
#define ADDR_SECTOR_7	(ADDR_SECTOR_6 + 128*1024)


HAL_StatusTypeDef emb_flash_erase(uint32_t address, uint32_t size, uint32_t *sector_error);
HAL_StatusTypeDef emb_flash_write(uint32_t address, uint32_t *data, uint32_t size);
void emb_flash_read(uint32_t address, uint32_t *data, uint32_t size);


#endif
