/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    usart.h
  * @brief   This file contains all the function prototypes for
  *          the usart.c file
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
#ifndef __USART_H__
#define __USART_H__

#include "tx_api.h"
#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "string.h"

/* USER CODE BEGIN Includes */
#include "fdcan.h"

/* USER CODE END Includes */

extern UART_HandleTypeDef huart1;

extern UART_HandleTypeDef huart2;

extern UART_HandleTypeDef huart6;

extern UART_HandleTypeDef huart10;

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

void MX_USART1_UART_Init(void);
void MX_USART2_UART_Init(void);
void MX_USART6_UART_Init(void);
void MX_USART10_UART_Init(void);

/* USER CODE BEGIN Prototypes */
#define SERIAL_RX_BUFFER_SIZE  256
enum serial_rx_type {
	SERIAL_RX_BLOCK = 1,
	SERIAL_RX_DMA_IDLE,
};

struct serial_t {
	UART_HandleTypeDef 	*huart;
  TX_SEMAPHORE rx_semaphore;
	uint8_t	rx_type;
  uint16_t size;
  uint8_t rx_buffer_mirror[SERIAL_RX_BUFFER_SIZE];
	uint8_t	rx_buffer[SERIAL_RX_BUFFER_SIZE]; 
};

extern struct serial_t rs422;
extern struct serial_t rs485;
extern struct serial_t rs232;
extern struct serial_t usb_c;

void serial_start(struct serial_t *serial, UART_HandleTypeDef *huart, uint8_t rx_type);
void serial_dma_write(struct serial_t *serial, uint8_t *pData, uint8_t len);
void serial_block_write(struct serial_t *serial, uint8_t *pData, uint8_t len);
void serial_485_block_write(struct serial_t *serial, uint8_t *pData, uint8_t len);
uint8_t serial_get_data(struct serial_t *serial, struct converter_protocol *protocol);
void serial_data_packet(struct converter_protocol *protocol, struct fdcan_tx_frame *tx_frame);

unsigned int CRC16(uint8_t *puchMsg,unsigned int usDataLen);
uint8_t checkRxCRC(uint8_t *rxBuf, uint8_t rxLen, uint16_t rxCRC);


/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __USART_H__ */

