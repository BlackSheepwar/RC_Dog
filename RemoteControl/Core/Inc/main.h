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
#include "stm32f1xx_hal.h"

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
extern osSemaphoreId_t UART_RX_BSHandle;
extern osSemaphoreId_t UART_TX_CSHandle;
/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define E1B_Pin GPIO_PIN_0
#define E1B_GPIO_Port GPIOA
#define E1A_Pin GPIO_PIN_1
#define E1A_GPIO_Port GPIOA
#define E2B_Pin GPIO_PIN_2
#define E2B_GPIO_Port GPIOA
#define E2A_Pin GPIO_PIN_3
#define E2A_GPIO_Port GPIOA
#define LED3_Pin GPIO_PIN_12
#define LED3_GPIO_Port GPIOB
#define LED2_Pin GPIO_PIN_13
#define LED2_GPIO_Port GPIOB
#define LED1_Pin GPIO_PIN_14
#define LED1_GPIO_Port GPIOB
#define K6_Pin GPIO_PIN_8
#define K6_GPIO_Port GPIOA
#define K5_Pin GPIO_PIN_11
#define K5_GPIO_Port GPIOA
#define K4_Pin GPIO_PIN_15
#define K4_GPIO_Port GPIOA
#define K3_Pin GPIO_PIN_3
#define K3_GPIO_Port GPIOB
#define K2_Pin GPIO_PIN_4
#define K2_GPIO_Port GPIOB
#define K1_Pin GPIO_PIN_5
#define K1_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
