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
#include "app_key.h"
#include "bsp_can.h"
#include "app_uart.h"
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
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
uint32_t defaultTaskBuffer[ 128 ];
osStaticThreadDef_t defaultTaskControlBlock;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .cb_mem = &defaultTaskControlBlock,
  .cb_size = sizeof(defaultTaskControlBlock),
  .stack_mem = &defaultTaskBuffer[0],
  .stack_size = sizeof(defaultTaskBuffer),
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for KEY_Debounce_T */
osThreadId_t KEY_Debounce_THandle;
uint32_t KEY_Debounce_TBuffer[ 128 ];
osStaticThreadDef_t KEY_Debounce_TControlBlock;
const osThreadAttr_t KEY_Debounce_T_attributes = {
  .name = "KEY_Debounce_T",
  .cb_mem = &KEY_Debounce_TControlBlock,
  .cb_size = sizeof(KEY_Debounce_TControlBlock),
  .stack_mem = &KEY_Debounce_TBuffer[0],
  .stack_size = sizeof(KEY_Debounce_TBuffer),
  .priority = (osPriority_t) osPriorityRealtime4,
};
/* Definitions for KEY_CMD_T */
osThreadId_t KEY_CMD_THandle;
uint32_t KEY_CMD_TBuffer[ 128 ];
osStaticThreadDef_t KEY_CMD_TControlBlock;
const osThreadAttr_t KEY_CMD_T_attributes = {
  .name = "KEY_CMD_T",
  .cb_mem = &KEY_CMD_TControlBlock,
  .cb_size = sizeof(KEY_CMD_TControlBlock),
  .stack_mem = &KEY_CMD_TBuffer[0],
  .stack_size = sizeof(KEY_CMD_TBuffer),
  .priority = (osPriority_t) osPriorityHigh5,
};
/* Definitions for UART_RX_T */
osThreadId_t UART_RX_THandle;
uint32_t UART_RX_TBuffer[ 256 ];
osStaticThreadDef_t UART_RX_TControlBlock;
const osThreadAttr_t UART_RX_T_attributes = {
  .name = "UART_RX_T",
  .cb_mem = &UART_RX_TControlBlock,
  .cb_size = sizeof(UART_RX_TControlBlock),
  .stack_mem = &UART_RX_TBuffer[0],
  .stack_size = sizeof(UART_RX_TBuffer),
  .priority = (osPriority_t) osPriorityRealtime3,
};
/* Definitions for UART_TX_T */
osThreadId_t UART_TX_THandle;
uint32_t UART_TX_TBuffer[ 128 ];
osStaticThreadDef_t UART_TX_TControlBlock;
const osThreadAttr_t UART_TX_T_attributes = {
  .name = "UART_TX_T",
  .cb_mem = &UART_TX_TControlBlock,
  .cb_size = sizeof(UART_TX_TControlBlock),
  .stack_mem = &UART_TX_TBuffer[0],
  .stack_size = sizeof(UART_TX_TBuffer),
  .priority = (osPriority_t) osPriorityAboveNormal6,
};
/* Definitions for UART_RX_CMD */
osThreadId_t UART_RX_CMDHandle;
uint32_t UART_RX_CMDBuffer[ 128 ];
osStaticThreadDef_t UART_RX_CMDControlBlock;
const osThreadAttr_t UART_RX_CMD_attributes = {
  .name = "UART_RX_CMD",
  .cb_mem = &UART_RX_CMDControlBlock,
  .cb_size = sizeof(UART_RX_CMDControlBlock),
  .stack_mem = &UART_RX_CMDBuffer[0],
  .stack_size = sizeof(UART_RX_CMDBuffer),
  .priority = (osPriority_t) osPriorityRealtime2,
};
/* Definitions for CAN_TX_T */
osThreadId_t CAN_TX_THandle;
uint32_t CAN_TX_TBuffer[ 128 ];
osStaticThreadDef_t CAN_TX_TControlBlock;
const osThreadAttr_t CAN_TX_T_attributes = {
  .name = "CAN_TX_T",
  .cb_mem = &CAN_TX_TControlBlock,
  .cb_size = sizeof(CAN_TX_TControlBlock),
  .stack_mem = &CAN_TX_TBuffer[0],
  .stack_size = sizeof(CAN_TX_TBuffer),
  .priority = (osPriority_t) osPriorityAboveNormal5,
};
/* Definitions for CAN_RXF0_T */
osThreadId_t CAN_RXF0_THandle;
uint32_t CAN_RXF0_TBuffer[ 256 ];
osStaticThreadDef_t CAN_RXF0_TControlBlock;
const osThreadAttr_t CAN_RXF0_T_attributes = {
  .name = "CAN_RXF0_T",
  .cb_mem = &CAN_RXF0_TControlBlock,
  .cb_size = sizeof(CAN_RXF0_TControlBlock),
  .stack_mem = &CAN_RXF0_TBuffer[0],
  .stack_size = sizeof(CAN_RXF0_TBuffer),
  .priority = (osPriority_t) osPriorityRealtime6,
};
/* Definitions for CAN_RXF1_T */
osThreadId_t CAN_RXF1_THandle;
uint32_t CAN_RXF1_TBuffer[ 256 ];
osStaticThreadDef_t CAN_RXF1_TControlBlock;
const osThreadAttr_t CAN_RXF1_T_attributes = {
  .name = "CAN_RXF1_T",
  .cb_mem = &CAN_RXF1_TControlBlock,
  .cb_size = sizeof(CAN_RXF1_TControlBlock),
  .stack_mem = &CAN_RXF1_TBuffer[0],
  .stack_size = sizeof(CAN_RXF1_TBuffer),
  .priority = (osPriority_t) osPriorityRealtime5,
};
/* Definitions for SERVO_T */
osThreadId_t SERVO_THandle;
uint32_t SERVO_TBuffer[ 128 ];
osStaticThreadDef_t SERVO_TControlBlock;
const osThreadAttr_t SERVO_T_attributes = {
  .name = "SERVO_T",
  .cb_mem = &SERVO_TControlBlock,
  .cb_size = sizeof(SERVO_TControlBlock),
  .stack_mem = &SERVO_TBuffer[0],
  .stack_size = sizeof(SERVO_TBuffer),
  .priority = (osPriority_t) osPriorityHigh6,
};
/* Definitions for OLED_T */
osThreadId_t OLED_THandle;
uint32_t OLED_TBuffer[ 256 ];
osStaticThreadDef_t OLED_TControlBlock;
const osThreadAttr_t OLED_T_attributes = {
  .name = "OLED_T",
  .cb_mem = &OLED_TControlBlock,
  .cb_size = sizeof(OLED_TControlBlock),
  .stack_mem = &OLED_TBuffer[0],
  .stack_size = sizeof(OLED_TBuffer),
  .priority = (osPriority_t) osPriorityLow6,
};
/* Definitions for KEY_CMD_Q */
osMessageQueueId_t KEY_CMD_QHandle;
uint8_t KEY_CMD_QBuffer[ 16 * sizeof( App_Key_EventPacket_t ) ];
osStaticMessageQDef_t KEY_CMD_QControlBlock;
const osMessageQueueAttr_t KEY_CMD_Q_attributes = {
  .name = "KEY_CMD_Q",
  .cb_mem = &KEY_CMD_QControlBlock,
  .cb_size = sizeof(KEY_CMD_QControlBlock),
  .mq_mem = &KEY_CMD_QBuffer,
  .mq_size = sizeof(KEY_CMD_QBuffer)
};
/* Definitions for UART_RX_Q */
osMessageQueueId_t UART_RX_QHandle;
uint8_t UART_RX_QBuffer[ 16 * sizeof( uint8_t ) ];
osStaticMessageQDef_t UART_RX_QControlBlock;
const osMessageQueueAttr_t UART_RX_Q_attributes = {
  .name = "UART_RX_Q",
  .cb_mem = &UART_RX_QControlBlock,
  .cb_size = sizeof(UART_RX_QControlBlock),
  .mq_mem = &UART_RX_QBuffer,
  .mq_size = sizeof(UART_RX_QBuffer)
};
/* Definitions for UART_TX_Q */
osMessageQueueId_t UART_TX_QHandle;
uint8_t UART_TX_QBuffer[ 16 * sizeof( APP_TxFrame_t ) ];
osStaticMessageQDef_t UART_TX_QControlBlock;
const osMessageQueueAttr_t UART_TX_Q_attributes = {
  .name = "UART_TX_Q",
  .cb_mem = &UART_TX_QControlBlock,
  .cb_size = sizeof(UART_TX_QControlBlock),
  .mq_mem = &UART_TX_QBuffer,
  .mq_size = sizeof(UART_TX_QBuffer)
};
/* Definitions for CAN_RXF0_Q */
osMessageQueueId_t CAN_RXF0_QHandle;
uint8_t CAN_RXF0_QBuffer[ 16 * sizeof( BSP_CAN_Packet_t ) ];
osStaticMessageQDef_t CAN_RXF0_QControlBlock;
const osMessageQueueAttr_t CAN_RXF0_Q_attributes = {
  .name = "CAN_RXF0_Q",
  .cb_mem = &CAN_RXF0_QControlBlock,
  .cb_size = sizeof(CAN_RXF0_QControlBlock),
  .mq_mem = &CAN_RXF0_QBuffer,
  .mq_size = sizeof(CAN_RXF0_QBuffer)
};
/* Definitions for CAN_RXF1_Q */
osMessageQueueId_t CAN_RXF1_QHandle;
uint8_t CAN_RXF1_QBuffer[ 16 * sizeof( BSP_CAN_Packet_t ) ];
osStaticMessageQDef_t CAN_RXF1_QControlBlock;
const osMessageQueueAttr_t CAN_RXF1_Q_attributes = {
  .name = "CAN_RXF1_Q",
  .cb_mem = &CAN_RXF1_QControlBlock,
  .cb_size = sizeof(CAN_RXF1_QControlBlock),
  .mq_mem = &CAN_RXF1_QBuffer,
  .mq_size = sizeof(CAN_RXF1_QBuffer)
};
/* Definitions for CAN_TX_BS */
osSemaphoreId_t CAN_TX_BSHandle;
osStaticSemaphoreDef_t CAN_TX_BSControlBlock;
const osSemaphoreAttr_t CAN_TX_BS_attributes = {
  .name = "CAN_TX_BS",
  .cb_mem = &CAN_TX_BSControlBlock,
  .cb_size = sizeof(CAN_TX_BSControlBlock),
};
/* Definitions for UART_RX_BS */
osSemaphoreId_t UART_RX_BSHandle;
osStaticSemaphoreDef_t UART_RX_BSControlBlock;
const osSemaphoreAttr_t UART_RX_BS_attributes = {
  .name = "UART_RX_BS",
  .cb_mem = &UART_RX_BSControlBlock,
  .cb_size = sizeof(UART_RX_BSControlBlock),
};
/* Definitions for UART_TX_CS */
osSemaphoreId_t UART_TX_CSHandle;
osStaticSemaphoreDef_t UART_TX_CSControlBlock;
const osSemaphoreAttr_t UART_TX_CS_attributes = {
  .name = "UART_TX_CS",
  .cb_mem = &UART_TX_CSControlBlock,
  .cb_size = sizeof(UART_TX_CSControlBlock),
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void Task_KEY_Debounce(void *argument);
void TASK_KEY_CMD(void *argument);
void Task_UART_RX(void *argument);
void Task_UART_TX(void *argument);
void Task_UART_RX_CMD(void *argument);
void Task_CAN_TX(void *argument);
void Task_CAN_RXF0(void *argument);
void Task_CAN_RXF1(void *argument);
void Task_SERVO_T(void *argument);
void Task_OLED(void *argument);

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
  /* creation of CAN_TX_BS */
  CAN_TX_BSHandle = osSemaphoreNew(1, 1, &CAN_TX_BS_attributes);

  /* creation of UART_RX_BS */
  UART_RX_BSHandle = osSemaphoreNew(1, 1, &UART_RX_BS_attributes);

  /* creation of UART_TX_CS */
  UART_TX_CSHandle = osSemaphoreNew(16, 0, &UART_TX_CS_attributes);

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of KEY_CMD_Q */
  KEY_CMD_QHandle = osMessageQueueNew (16, sizeof(App_Key_EventPacket_t), &KEY_CMD_Q_attributes);

  /* creation of UART_RX_Q */
  UART_RX_QHandle = osMessageQueueNew (16, sizeof(uint8_t), &UART_RX_Q_attributes);

  /* creation of UART_TX_Q */
  UART_TX_QHandle = osMessageQueueNew (16, sizeof(APP_TxFrame_t), &UART_TX_Q_attributes);

  /* creation of CAN_RXF0_Q */
  CAN_RXF0_QHandle = osMessageQueueNew (16, sizeof(BSP_CAN_Packet_t), &CAN_RXF0_Q_attributes);

  /* creation of CAN_RXF1_Q */
  CAN_RXF1_QHandle = osMessageQueueNew (16, sizeof(BSP_CAN_Packet_t), &CAN_RXF1_Q_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of KEY_Debounce_T */
  KEY_Debounce_THandle = osThreadNew(Task_KEY_Debounce, NULL, &KEY_Debounce_T_attributes);

  /* creation of KEY_CMD_T */
  KEY_CMD_THandle = osThreadNew(TASK_KEY_CMD, NULL, &KEY_CMD_T_attributes);

  /* creation of UART_RX_T */
  UART_RX_THandle = osThreadNew(Task_UART_RX, NULL, &UART_RX_T_attributes);

  /* creation of UART_TX_T */
  UART_TX_THandle = osThreadNew(Task_UART_TX, NULL, &UART_TX_T_attributes);

  /* creation of UART_RX_CMD */
  UART_RX_CMDHandle = osThreadNew(Task_UART_RX_CMD, NULL, &UART_RX_CMD_attributes);

  /* creation of CAN_TX_T */
  CAN_TX_THandle = osThreadNew(Task_CAN_TX, NULL, &CAN_TX_T_attributes);

  /* creation of CAN_RXF0_T */
  CAN_RXF0_THandle = osThreadNew(Task_CAN_RXF0, NULL, &CAN_RXF0_T_attributes);

  /* creation of CAN_RXF1_T */
  CAN_RXF1_THandle = osThreadNew(Task_CAN_RXF1, NULL, &CAN_RXF1_T_attributes);

  /* creation of SERVO_T */
  SERVO_THandle = osThreadNew(Task_SERVO_T, NULL, &SERVO_T_attributes);

  /* creation of OLED_T */
  OLED_THandle = osThreadNew(Task_OLED, NULL, &OLED_T_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
__weak void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  for(;;)
  {
    //HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_3);
    osDelay(1);
  }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_Task_KEY_Debounce */
/**
* @brief Function implementing the KEY_Debounce_T thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Task_KEY_Debounce */
__weak void Task_KEY_Debounce(void *argument)
{
  /* USER CODE BEGIN Task_KEY_Debounce */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END Task_KEY_Debounce */
}

/* USER CODE BEGIN Header_TASK_KEY_CMD */
/**
* @brief Function implementing the KEY_CMD_T thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_TASK_KEY_CMD */
__weak void TASK_KEY_CMD(void *argument)
{
  /* USER CODE BEGIN TASK_KEY_CMD */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END TASK_KEY_CMD */
}

/* USER CODE BEGIN Header_Task_UART_RX */
/**
* @brief Function implementing the UART_RX_T thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Task_UART_RX */
__weak void Task_UART_RX(void *argument)
{
  /* USER CODE BEGIN Task_UART_RX */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END Task_UART_RX */
}

/* USER CODE BEGIN Header_Task_UART_TX */
/**
* @brief Function implementing the UART_TX_T thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Task_UART_TX */
__weak void Task_UART_TX(void *argument)
{
  /* USER CODE BEGIN Task_UART_TX */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END Task_UART_TX */
}

/* USER CODE BEGIN Header_Task_UART_RX_CMD */
/**
* @brief Function implementing the UART_RX_CMD thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Task_UART_RX_CMD */
__weak void Task_UART_RX_CMD(void *argument)
{
  /* USER CODE BEGIN Task_UART_RX_CMD */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END Task_UART_RX_CMD */
}

/* USER CODE BEGIN Header_Task_CAN_TX */
/**
* @brief Function implementing the CAN_TX_T thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Task_CAN_TX */
__weak void Task_CAN_TX(void *argument)
{
  /* USER CODE BEGIN Task_CAN_TX */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END Task_CAN_TX */
}

/* USER CODE BEGIN Header_Task_CAN_RXF0 */
/**
* @brief Function implementing the CAN_RXF0_T thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Task_CAN_RXF0 */
__weak void Task_CAN_RXF0(void *argument)
{
  /* USER CODE BEGIN Task_CAN_RXF0 */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END Task_CAN_RXF0 */
}

/* USER CODE BEGIN Header_Task_CAN_RXF1 */
/**
* @brief Function implementing the CAN_RXF1_T thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Task_CAN_RXF1 */
__weak void Task_CAN_RXF1(void *argument)
{
  /* USER CODE BEGIN Task_CAN_RXF1 */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END Task_CAN_RXF1 */
}

/* USER CODE BEGIN Header_Task_SERVO_T */
/**
* @brief Function implementing the SERVO_T thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Task_SERVO_T */
__weak void Task_SERVO_T(void *argument)
{
  /* USER CODE BEGIN Task_SERVO_T */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END Task_SERVO_T */
}

/* USER CODE BEGIN Header_Task_OLED */
/**
* @brief Function implementing the OLED_T thread.
* @param argument: Not used
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

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

