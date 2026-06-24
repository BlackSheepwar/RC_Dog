/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "FreeRTOS.h"
#include "cmsis_os2.h"
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */
extern osMessageQueueId_t UART_RX_QHandle;
extern osMessageQueueId_t UART_TX_QHandle;
extern osMessageQueueId_t KEY_CMD_QHandle;
extern osMessageQueueId_t CAN_RXF0_QHandle;
extern osMessageQueueId_t CAN_RXF1_QHandle;
extern osSemaphoreId_t UART_RX_BSHandle;
extern osSemaphoreId_t CAN_TX_BSHandle;
/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define LED_Pin GPIO_PIN_3
#define LED_GPIO_Port GPIOE
#define SPI4_CS1_Pin GPIO_PIN_3
#define SPI4_CS1_GPIO_Port GPIOC
#define SPI4_CS2_Pin GPIO_PIN_5
#define SPI4_CS2_GPIO_Port GPIOA
#define SPI4_CS3_Pin GPIO_PIN_5
#define SPI4_CS3_GPIO_Port GPIOC
#define SPI4_CS4_Pin GPIO_PIN_1
#define SPI4_CS4_GPIO_Port GPIOB
#define M1_Pin GPIO_PIN_12
#define M1_GPIO_Port GPIOB
#define M0_Pin GPIO_PIN_13
#define M0_GPIO_Port GPIOB
#define SPI3_CS1_Pin GPIO_PIN_15
#define SPI3_CS1_GPIO_Port GPIOD
#define SPI3_CS2_Pin GPIO_PIN_4
#define SPI3_CS2_GPIO_Port GPIOD
#define FLASH2_Pin GPIO_PIN_6
#define FLASH2_GPIO_Port GPIOD
#define FLASH1_Pin GPIO_PIN_6
#define FLASH1_GPIO_Port GPIOB
#define SPI3_CS3_Pin GPIO_PIN_7
#define SPI3_CS3_GPIO_Port GPIOB
#define SPI2_CS_Pin GPIO_PIN_0
#define SPI2_CS_GPIO_Port GPIOE
#define SPI3_CS4_Pin GPIO_PIN_1
#define SPI3_CS4_GPIO_Port GPIOE

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
