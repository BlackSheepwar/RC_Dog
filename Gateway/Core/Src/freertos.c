/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
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

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "debounce.h"
#include "bsp_can.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
typedef StaticTask_t osStaticThreadDef_t;
typedef StaticQueue_t osStaticMessageQDef_t;
typedef StaticSemaphore_t osStaticSemaphoreDef_t;
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for OLED_T */
osThreadId_t OLED_THandle;
uint32_t OLED_TBuffer[ 128 ];
osStaticThreadDef_t OLED_TControlBlock;
const osThreadAttr_t OLED_T_attributes = {
  .name = "OLED_T",
  .cb_mem = &OLED_TControlBlock,
  .cb_size = sizeof(OLED_TControlBlock),
  .stack_mem = &OLED_TBuffer[0],
  .stack_size = sizeof(OLED_TBuffer),
  .priority = (osPriority_t) osPriorityAboveNormal6,
};
/* Definitions for RX_T */
osThreadId_t RX_THandle;
uint32_t RX_TBuffer[ 200 ];
osStaticThreadDef_t RX_TControlBlock;
const osThreadAttr_t RX_T_attributes = {
  .name = "RX_T",
  .cb_mem = &RX_TControlBlock,
  .cb_size = sizeof(RX_TControlBlock),
  .stack_mem = &RX_TBuffer[0],
  .stack_size = sizeof(RX_TBuffer),
  .priority = (osPriority_t) osPriorityRealtime3,
};
/* Definitions for TX_T */
osThreadId_t TX_THandle;
uint32_t TX_TBuffer[ 128 ];
osStaticThreadDef_t TX_TControlBlock;
const osThreadAttr_t TX_T_attributes = {
  .name = "TX_T",
  .cb_mem = &TX_TControlBlock,
  .cb_size = sizeof(TX_TControlBlock),
  .stack_mem = &TX_TBuffer[0],
  .stack_size = sizeof(TX_TBuffer),
  .priority = (osPriority_t) osPriorityAboveNormal6,
};
/* Definitions for KEY_T */
osThreadId_t KEY_THandle;
uint32_t KEY_TBuffer[ 80 ];
osStaticThreadDef_t KEY_TControlBlock;
const osThreadAttr_t KEY_T_attributes = {
  .name = "KEY_T",
  .cb_mem = &KEY_TControlBlock,
  .cb_size = sizeof(KEY_TControlBlock),
  .stack_mem = &KEY_TBuffer[0],
  .stack_size = sizeof(KEY_TBuffer),
  .priority = (osPriority_t) osPriorityRealtime4,
};
/* Definitions for PAR_T */
osThreadId_t PAR_THandle;
uint32_t PAR_TBuffer[ 80 ];
osStaticThreadDef_t PAR_TControlBlock;
const osThreadAttr_t PAR_T_attributes = {
  .name = "PAR_T",
  .cb_mem = &PAR_TControlBlock,
  .cb_size = sizeof(PAR_TControlBlock),
  .stack_mem = &PAR_TBuffer[0],
  .stack_size = sizeof(PAR_TBuffer),
  .priority = (osPriority_t) osPriorityHigh6,
};
/* Definitions for CAN_F0_T */
osThreadId_t CAN_F0_THandle;
uint32_t CAN_F0_TBuffer[ 128 ];
osStaticThreadDef_t CAN_F0_TControlBlock;
const osThreadAttr_t CAN_F0_T_attributes = {
  .name = "CAN_F0_T",
  .cb_mem = &CAN_F0_TControlBlock,
  .cb_size = sizeof(CAN_F0_TControlBlock),
  .stack_mem = &CAN_F0_TBuffer[0],
  .stack_size = sizeof(CAN_F0_TBuffer),
  .priority = (osPriority_t) osPriorityRealtime6,
};
/* Definitions for CAN_F1_T */
osThreadId_t CAN_F1_THandle;
uint32_t CAN_F1_TBuffer[ 128 ];
osStaticThreadDef_t CAN_F1_TControlBlock;
const osThreadAttr_t CAN_F1_T_attributes = {
  .name = "CAN_F1_T",
  .cb_mem = &CAN_F1_TControlBlock,
  .cb_size = sizeof(CAN_F1_TControlBlock),
  .stack_mem = &CAN_F1_TBuffer[0],
  .stack_size = sizeof(CAN_F1_TBuffer),
  .priority = (osPriority_t) osPriorityRealtime5,
};
/* Definitions for RX_Q */
osMessageQueueId_t RX_QHandle;
uint8_t RX_QBuffer[ 8 * sizeof( uint8_t ) ];
osStaticMessageQDef_t RX_QControlBlock;
const osMessageQueueAttr_t RX_Q_attributes = {
  .name = "RX_Q",
  .cb_mem = &RX_QControlBlock,
  .cb_size = sizeof(RX_QControlBlock),
  .mq_mem = &RX_QBuffer,
  .mq_size = sizeof(RX_QBuffer)
};
/* Definitions for KEY_Q */
osMessageQueueId_t KEY_QHandle;
uint8_t KEY_QBuffer[ 8 * sizeof( Debounce_Event_packet_t ) ];
osStaticMessageQDef_t KEY_QControlBlock;
const osMessageQueueAttr_t KEY_Q_attributes = {
  .name = "KEY_Q",
  .cb_mem = &KEY_QControlBlock,
  .cb_size = sizeof(KEY_QControlBlock),
  .mq_mem = &KEY_QBuffer,
  .mq_size = sizeof(KEY_QBuffer)
};
/* Definitions for CAN_F0_Q */
osMessageQueueId_t CAN_F0_QHandle;
uint8_t CAN_F0_QBuffer[ 8 * sizeof( BSP_CAN_Packet_t ) ];
osStaticMessageQDef_t CAN_F0_QControlBlock;
const osMessageQueueAttr_t CAN_F0_Q_attributes = {
  .name = "CAN_F0_Q",
  .cb_mem = &CAN_F0_QControlBlock,
  .cb_size = sizeof(CAN_F0_QControlBlock),
  .mq_mem = &CAN_F0_QBuffer,
  .mq_size = sizeof(CAN_F0_QBuffer)
};
/* Definitions for CAN_F1_Q */
osMessageQueueId_t CAN_F1_QHandle;
uint8_t CAN_F1_QBuffer[ 8 * sizeof( BSP_CAN_Packet_t ) ];
osStaticMessageQDef_t CAN_F1_QControlBlock;
const osMessageQueueAttr_t CAN_F1_Q_attributes = {
  .name = "CAN_F1_Q",
  .cb_mem = &CAN_F1_QControlBlock,
  .cb_size = sizeof(CAN_F1_QControlBlock),
  .mq_mem = &CAN_F1_QBuffer,
  .mq_size = sizeof(CAN_F1_QBuffer)
};
/* Definitions for TX_BS */
osSemaphoreId_t TX_BSHandle;
osStaticSemaphoreDef_t TX_BSControlBlock;
const osSemaphoreAttr_t TX_BS_attributes = {
  .name = "TX_BS",
  .cb_mem = &TX_BSControlBlock,
  .cb_size = sizeof(TX_BSControlBlock),
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void Task_OLED(void *argument);
void Task_RX(void *argument);
void Task_TX(void *argument);
void Task_KEY(void *argument);
void Task_PAR(void *argument);
void Task_CAN_F0(void *argument);
void Task_CAN_F1(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* Create the semaphores(s) */
  /* creation of TX_BS */
  TX_BSHandle = osSemaphoreNew(1, 1, &TX_BS_attributes);

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of RX_Q */
  RX_QHandle = osMessageQueueNew (8, sizeof(uint8_t), &RX_Q_attributes);

  /* creation of KEY_Q */
  KEY_QHandle = osMessageQueueNew (8, sizeof(Debounce_Event_packet_t), &KEY_Q_attributes);

  /* creation of CAN_F0_Q */
  CAN_F0_QHandle = osMessageQueueNew (8, sizeof(BSP_CAN_Packet_t), &CAN_F0_Q_attributes);

  /* creation of CAN_F1_Q */
  CAN_F1_QHandle = osMessageQueueNew (8, sizeof(BSP_CAN_Packet_t), &CAN_F1_Q_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of OLED_T */
  OLED_THandle = osThreadNew(Task_OLED, NULL, &OLED_T_attributes);

  /* creation of RX_T */
  RX_THandle = osThreadNew(Task_RX, NULL, &RX_T_attributes);

  /* creation of TX_T */
  TX_THandle = osThreadNew(Task_TX, NULL, &TX_T_attributes);

  /* creation of KEY_T */
  KEY_THandle = osThreadNew(Task_KEY, NULL, &KEY_T_attributes);

  /* creation of PAR_T */
  PAR_THandle = osThreadNew(Task_PAR, NULL, &PAR_T_attributes);

  /* creation of CAN_F0_T */
  CAN_F0_THandle = osThreadNew(Task_CAN_F0, NULL, &CAN_F0_T_attributes);

  /* creation of CAN_F1_T */
  CAN_F1_THandle = osThreadNew(Task_CAN_F1, NULL, &CAN_F1_T_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_Task_OLED */
/**
  * @brief  Function implementing the OLED_T thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_Task_OLED */
__weak void Task_OLED(void *argument)
{
  /* USER CODE BEGIN Task_OLED */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END Task_OLED */
}

/* USER CODE BEGIN Header_Task_RX */
/**
* @brief Function implementing the RX_T thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Task_RX */
__weak void Task_RX(void *argument)
{
  /* USER CODE BEGIN Task_RX */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END Task_RX */
}

/* USER CODE BEGIN Header_Task_TX */
/**
* @brief Function implementing the TX_T thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Task_TX */
__weak void Task_TX(void *argument)
{
  /* USER CODE BEGIN Task_TX */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END Task_TX */
}

/* USER CODE BEGIN Header_Task_KEY */
/**
* @brief Function implementing the KEY_T thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Task_KEY */
__weak void Task_KEY(void *argument)
{
  /* USER CODE BEGIN Task_KEY */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END Task_KEY */
}

/* USER CODE BEGIN Header_Task_PAR */
/**
* @brief Function implementing the PAR_T thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Task_PAR */
__weak void Task_PAR(void *argument)
{
  /* USER CODE BEGIN Task_PAR */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END Task_PAR */
}

/* USER CODE BEGIN Header_Task_CAN_F0 */
/**
* @brief Function implementing the CAN_F0_T thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Task_CAN_F0 */
__weak void Task_CAN_F0(void *argument)
{
  /* USER CODE BEGIN Task_CAN_F0 */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END Task_CAN_F0 */
}

/* USER CODE BEGIN Header_Task_CAN_F1 */
/**
* @brief Function implementing the CAN_F1_T thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Task_CAN_F1 */
__weak void Task_CAN_F1(void *argument)
{
  /* USER CODE BEGIN Task_CAN_F1 */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END Task_CAN_F1 */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

