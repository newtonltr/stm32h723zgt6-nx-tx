/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <math.h>
#include <string.h>
#include "eth.h"
#include "tx_api.h"
#include "nx_api.h"
#include "tx_thread.h"

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
extern ETH_HandleTypeDef heth;

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define PHY_RST_Pin GPIO_PIN_10
#define PHY_RST_GPIO_Port GPIOB
#define USART1_CK_Pin GPIO_PIN_8
#define USART1_CK_GPIO_Port GPIOD

/* USER CODE BEGIN Private defines */

// 默认mac地址和ip地址

#define DEFAULT_IP_ADDR0                        192
#define DEFAULT_IP_ADDR1                        168
#define DEFAULT_IP_ADDR2                        1
#define DEFAULT_IP_ADDR3                        111

#define FLASH_HEAD 0x55
#define FLASH_TAIL 0xAA
struct socket_param_t
{
  uint8_t flash_head;
  uint8_t mac_address[6];
  uint32_t ip_address;
  uint8_t flash_tail;
};

extern struct socket_param_t socket_param_data;
extern const uint32_t socket_param_data_address;
extern uint8_t default_mac_address[6];
extern uint32_t default_ip_address;

void sleep_s(uint32_t s);
void sleep_ms(uint32_t ms);
void sleep_us(uint32_t us);


/* USER CODE BEGIN Private defines */
#define CONVERTER_PROTOCOL_HEAD 0XAA
#define CONVERTER_PROTOCOL_TAIL 0X55

enum frame_type_list {
  CAN_DATA_FRAME,
  CAN_REMOTE_FRAME,
};

enum comm_type_list {
  COMM_TYPE_UART,
  COMM_TYPE_TCP,
};

__packed struct can_pack_protocol_head {
  uint8_t head;
  uint32_t id;
  uint8_t frame_type;
  uint8_t len;
};

__packed struct converter_protocol {
  struct can_pack_protocol_head header;
  uint8_t data[8];
  uint16_t crc;  
};

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
