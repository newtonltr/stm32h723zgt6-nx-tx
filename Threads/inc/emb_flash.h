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


// HardFault故障信息存储相关定义
#define HARDFAULT_MAGIC_NUMBER    0xDEADBEBF
#define HARDFAULT_STORAGE_ADDR    ADDR_SECTOR_7

// HardFault故障信息数据结构
typedef struct {
    uint32_t magic_number;    // 魔数标识 0xDEADBEEF
    uint32_t timestamp;       // 时间戳（运行时间）
    uint32_t sp_value;        // 堆栈指针值  
    uint32_t r0_value;        // r0寄存器值
    uint32_t r1_value;        // r1寄存器值
    uint32_t r2_value;        // r2寄存器值
    uint32_t r3_value;        // r3寄存器值
    uint32_t r12_value;       // r12寄存器值
    uint32_t lr_value;        // 链接寄存器值
    uint32_t pc_value;        // 程序计数器值
    uint32_t xpsr_value;       // xpsr寄存器值
    uint32_t fault_count;     // 故障计数器
    uint32_t reserved[2];     // 保留字段，保持32字节对齐
} hardfault_info_t;


/**
 * @brief 读取HardFault故障信息
 * @param fault_info 指向接收故障信息的结构体指针
 * @return 0: 成功读取有效数据, -1: 没有有效数据或数据损坏
 */
 int HardFault_ReadInfo(hardfault_info_t *fault_info);

 /**
  * @brief 写入HardFault故障信息到Flash
  * @param fault_info 指向要写入的故障信息结构体指针
  * @return 0: 写入成功, -1: 写入失败
  */
 int HardFault_WriteInfo(const hardfault_info_t *fault_info);
 

HAL_StatusTypeDef emb_flash_erase(uint32_t address, uint32_t size, uint32_t *sector_error);
HAL_StatusTypeDef emb_flash_write(uint32_t address, uint32_t *data, uint32_t size);
void emb_flash_read(uint32_t address, uint32_t *data, uint32_t size);


#endif
