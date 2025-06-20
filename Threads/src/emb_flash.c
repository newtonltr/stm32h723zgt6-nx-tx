#include "emb_flash.h"
#include <stdint.h>

//获取某个地址所在的flash扇区
//addr:flash地址
//返回值:0~7,即addr所在的扇区
uint8_t FLASH_GetFlashSector(uint32_t addr)
{
	if(addr<ADDR_SECTOR_1)return FLASH_SECTOR_0;
	else if(addr<ADDR_SECTOR_2)return FLASH_SECTOR_1;
	else if(addr<ADDR_SECTOR_3)return FLASH_SECTOR_2;
	else if(addr<ADDR_SECTOR_4)return FLASH_SECTOR_3;
	else if(addr<ADDR_SECTOR_5)return FLASH_SECTOR_4;
	else if(addr<ADDR_SECTOR_6)return FLASH_SECTOR_5;
	else if(addr<ADDR_SECTOR_7)return FLASH_SECTOR_6;
	return FLASH_SECTOR_7;	
}


/**
 * @brief  擦除指定地址和大小的Flash扇区
 * @param  address: Flash起始地址 (必须在Bank1范围内: 0x08000000-0x080FFFFF)
 * @param  size: 要擦除的字节大小
 * @param  sector_error: 返回擦除失败的扇区号指针 (0xFFFFFFFF表示全部成功)
 * @retval HAL状态码: HAL_OK=成功, HAL_ERROR=失败
 */
HAL_StatusTypeDef emb_flash_erase(uint32_t address, uint32_t size, uint32_t *sector_error)
{
	// 地址范围检查：确保地址在Bank1有效范围内
	if (address < ADDR_SECTOR_0 || address >= (ADDR_SECTOR_7 + 128*1024))
	{
		*sector_error = 0xFFFFFFFF;
		return HAL_ERROR; // 地址超出Bank1范围
	}
	
	// 判断输入的地址在哪个扇区，并根据size判断一共要擦除多少个扇区
	uint8_t start_sector = FLASH_GetFlashSector(address);
	uint8_t end_sector = FLASH_GetFlashSector(address + size - 1);
	uint8_t erase_sector_num = end_sector - start_sector + 1;
	
	// 配置擦除参数结构体
	FLASH_EraseInitTypeDef erase_init;
	erase_init.TypeErase = FLASH_TYPEERASE_SECTORS;    // 扇区擦除模式
	erase_init.Banks = FLASH_BANK_1;                   // 擦除Bank1
	erase_init.Sector = start_sector;                  // 起始扇区号
	erase_init.NbSectors = erase_sector_num;           // 擦除扇区数量
	erase_init.VoltageRange = FLASH_VOLTAGE_RANGE_3;   // 电压范围：2.7V-3.6V
	
	// 解锁Flash控制寄存器
	HAL_FLASH_Unlock();
	
	// 执行扇区擦除操作
	HAL_StatusTypeDef status = HAL_FLASHEx_Erase(&erase_init, sector_error);
	
	// 锁定Flash控制寄存器
	HAL_FLASH_Lock();
	
	return status;
}


/**
 * @brief  向Flash写入数据 (自动处理32字节对齐要求)
 * @param  address: Flash写入地址 (必须在Bank1范围内，函数会自动32字节对齐)
 * @param  data: 要写入的数据指针 (任意类型数据)
 * @param  size: 要写入的字节数量 (可直接使用sizeof()结果)
 * @retval HAL状态码: HAL_OK=成功, HAL_ERROR=失败
 * @note   STM32H7要求：
 *         1. 写入地址必须32字节对齐 (函数自动处理)
 *         2. 写入数据量必须是32字节的整数倍 (函数自动填充)
 *         3. 写入前必须先擦除对应区域 (函数自动处理)
 */
HAL_StatusTypeDef emb_flash_write(uint32_t address, uint32_t *data, uint32_t size)
{
	// 参数有效性检查
	if (data == NULL || size == 0)
	{
		return HAL_ERROR;
	}
	
	// 地址范围检查：确保地址在Bank1有效范围内
	if (address < ADDR_SECTOR_0 || address >= (ADDR_SECTOR_7 + 128*1024))
	{
		return HAL_ERROR; // 地址超出Bank1范围
	}
	
	// 计算32字节对齐的起始地址 (STM32H7要求32字节对齐)
	uint32_t aligned_address = address & ~0x1F; // 清除低5位，实现32字节对齐
	
	// 计算需要写入的总字节数，并向上对齐到32字节边界
	uint32_t total_bytes = size; // 参数已经是字节数
	uint32_t aligned_bytes = ((total_bytes + 31) / 32) * 32; // 向上取整到32字节倍数
	
	// 计算需要写入的32位字数量（用于内部循环控制）
	uint32_t total_words = (total_bytes + 3) / 4; // 字节数转换为字数，向上取整
	
	// 先擦除目标区域 (Flash写入前必须先擦除)
	uint32_t sector_error;
	HAL_StatusTypeDef erase_status = emb_flash_erase(aligned_address, aligned_bytes, &sector_error);
	if (erase_status != HAL_OK)
	{
		return HAL_ERROR; // 擦除失败
	}
	
	// 创建32字节对齐的缓冲区 (STM32H7每次必须写入32字节)
	uint32_t aligned_buffer[8]; // 32字节 = 8个32位字
	uint32_t words_written = 0;
	
	// 解锁Flash控制寄存器
	HAL_FLASH_Unlock();
	
	// 按32字节块循环写入数据
	while (words_written < total_words)
	{
		// 清空对齐缓冲区 (未使用的部分填充0xFF，这是Flash擦除后的状态)
		for (int i = 0; i < 8; i++)
		{
			aligned_buffer[i] = 0xFFFFFFFF;
		}
		
		// 复制本次要写入的数据到对齐缓冲区
		uint32_t words_to_copy = (total_words - words_written > 8) ? 8 : (total_words - words_written);
		for (uint32_t i = 0; i < words_to_copy; i++)
		{
			aligned_buffer[i] = data[words_written + i];
		}
		
		// 执行32字节Flash写入操作 (STM32H7使用256位编程)
		HAL_StatusTypeDef prog_status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, 
		                                                  aligned_address + words_written * 4, 
		                                                  (uint32_t)aligned_buffer);
		
		// 检查写入是否成功
		if (prog_status != HAL_OK)
		{
			HAL_FLASH_Lock(); // 出错时也要锁定Flash
			return HAL_ERROR;
		}
		
		// 更新已写入的字数和地址
		words_written += 8; // 每次写入8个32位字(32字节)
	}
	
	// 锁定Flash控制寄存器
	HAL_FLASH_Lock();
	
	return HAL_OK;
}

/**
 * @brief  从Flash读取数据 (内存映射方式，高效读取)
 * @param  address: Flash读取地址 (必须在Bank1范围内，建议4字节对齐以提高效率)
 * @param  data: 存储读取数据的缓冲区指针 (任意类型数据)
 * @param  size: 要读取的字节数量 (可直接使用sizeof()结果)
 * @retval 无返回值 (Flash读取操作通常不会失败)
 * @note   优势：
 *         1. 使用内存映射方式，无需解锁Flash
 *         2. 读取速度快，适合频繁读取操作
 *         3. 支持任意地址和大小的读取
 */
void emb_flash_read(uint32_t address, uint32_t *data, uint32_t size)
{
	// 参数有效性检查
	if (data == NULL || size == 0)
	{
		return; // 参数无效，直接返回
	}
	
	// 地址范围检查：确保地址在Bank1有效范围内
	if (address < ADDR_SECTOR_0 || address >= (ADDR_SECTOR_7 + 128*1024))
	{
		return; // 地址超出Bank1范围，直接返回
	}
	
	// 检查读取是否会越界
	if (address + size > (ADDR_SECTOR_7 + 128*1024))
	{
		return; // 读取会越界，直接返回
	}
	
	// 计算需要读取的32位字数量
	uint32_t words_to_read = (size + 3) / 4; // 字节数转换为字数，向上取整
	
	// 通过内存映射方式逐个32位字读取Flash数据
	// STM32H7的Flash通过AXI接口映射到内存空间，可以直接访问
	for (uint32_t i = 0; i < words_to_read; i++)
	{
		// 使用volatile确保编译器不会优化掉内存访问
		// 每次读取一个32位字(4字节)
		*(data + i) = *(volatile uint32_t*)(address + i * 4);
	}
}










