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
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
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
/* Definitions for KEYTask */
osThreadId_t KEYTaskHandle;
const osThreadAttr_t KEYTask_attributes = {
  .name = "KEYTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityRealtime5,
};
/* Definitions for PARTask */
osThreadId_t PARTaskHandle;
const osThreadAttr_t PARTask_attributes = {
  .name = "PARTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityHigh6,
};
/* Definitions for RXTask */
osThreadId_t RXTaskHandle;
const osThreadAttr_t RXTask_attributes = {
  .name = "RXTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityRealtime6,
};
/* Definitions for TXTask */
osThreadId_t TXTaskHandle;
const osThreadAttr_t TXTask_attributes = {
  .name = "TXTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityAboveNormal7,
};
/* Definitions for SETask */
osThreadId_t SETaskHandle;
const osThreadAttr_t SETask_attributes = {
  .name = "SETask",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityHigh7,
};
/* Definitions for CANTask */
osThreadId_t CANTaskHandle;
const osThreadAttr_t CANTask_attributes = {
  .name = "CANTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityRealtime7,
};
/* Definitions for OLEDTask */
osThreadId_t OLEDTaskHandle;
const osThreadAttr_t OLEDTask_attributes = {
  .name = "OLEDTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityLow7,
};
/* Definitions for RXQueue */
osMessageQueueId_t RXQueueHandle;
const osMessageQueueAttr_t RXQueue_attributes = {
  .name = "RXQueue"
};
/* Definitions for KEYQueue */
osMessageQueueId_t KEYQueueHandle;
const osMessageQueueAttr_t KEYQueue_attributes = {
  .name = "KEYQueue"
};
/* Definitions for TXBinarySem */
osSemaphoreId_t TXBinarySemHandle;
const osSemaphoreAttr_t TXBinarySem_attributes = {
  .name = "TXBinarySem"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartKEYTask(void *argument);
void StartPARTask(void *argument);
void StartRXTask(void *argument);
void StartTXTask(void *argument);
void StartSETask(void *argument);
void StartCANTask(void *argument);
void StartOLEDTask(void *argument);

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
  /* creation of TXBinarySem */
  TXBinarySemHandle = osSemaphoreNew(1, 1, &TXBinarySem_attributes);

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of RXQueue */
  RXQueueHandle = osMessageQueueNew (16, sizeof(uint8_t), &RXQueue_attributes);

  /* creation of KEYQueue */
  KEYQueueHandle = osMessageQueueNew (16, sizeof(Debounce_Event_packet_t), &KEYQueue_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of KEYTask */
  KEYTaskHandle = osThreadNew(StartKEYTask, NULL, &KEYTask_attributes);

  /* creation of PARTask */
  PARTaskHandle = osThreadNew(StartPARTask, NULL, &PARTask_attributes);

  /* creation of RXTask */
  RXTaskHandle = osThreadNew(StartRXTask, NULL, &RXTask_attributes);

  /* creation of TXTask */
  TXTaskHandle = osThreadNew(StartTXTask, NULL, &TXTask_attributes);

  /* creation of SETask */
  SETaskHandle = osThreadNew(StartSETask, NULL, &SETask_attributes);

  /* creation of CANTask */
  CANTaskHandle = osThreadNew(StartCANTask, NULL, &CANTask_attributes);

  /* creation of OLEDTask */
  OLEDTaskHandle = osThreadNew(StartOLEDTask, NULL, &OLEDTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartKEYTask */
/**
  * @brief  Function implementing the KEYTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartKEYTask */
__weak void StartKEYTask(void *argument)
{
  /* USER CODE BEGIN StartKEYTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartKEYTask */
}

/* USER CODE BEGIN Header_StartPARTask */
/**
* @brief Function implementing the PARTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartPARTask */
__weak void StartPARTask(void *argument)
{
  /* USER CODE BEGIN StartPARTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartPARTask */
}

/* USER CODE BEGIN Header_StartRXTask */
/**
* @brief Function implementing the RXTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartRXTask */
__weak void StartRXTask(void *argument)
{
  /* USER CODE BEGIN StartRXTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartRXTask */
}

/* USER CODE BEGIN Header_StartTXTask */
/**
* @brief Function implementing the TXTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTXTask */
__weak void StartTXTask(void *argument)
{
  /* USER CODE BEGIN StartTXTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartTXTask */
}

/* USER CODE BEGIN Header_StartSETask */
/**
* @brief Function implementing the SETask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartSETask */
__weak void StartSETask(void *argument)
{
  /* USER CODE BEGIN StartSETask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartSETask */
}

/* USER CODE BEGIN Header_StartCANTask */
/**
* @brief Function implementing the CANTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartCANTask */
__weak void StartCANTask(void *argument)
{
  /* USER CODE BEGIN StartCANTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartCANTask */
}

/* USER CODE BEGIN Header_StartOLEDTask */
/**
* @brief Function implementing the OLEDTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartOLEDTask */
__weak void StartOLEDTask(void *argument)
{
  /* USER CODE BEGIN StartOLEDTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartOLEDTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

