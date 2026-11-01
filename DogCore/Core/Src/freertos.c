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
#include "FreeRTOS.h"
#include "cmsis_os2.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "svc_key_logic.h"   /* App_Key_EventPacket_t 别名：生成代码要用它建队列 */
#include "app_uart.h"        /* APP_TxFrame_t 别名：生成代码要用它建队列 */
#include "bsp_can.h"         /* BSP_CAN_Packet_t 别名：生成代码要用它建队列 */
#include "svc_uart.h"
#include "bsp_iic.h"
/* 初始化链用到的各层头文件 */
#include "bsp_pwm.h"
#include "dev_key.h"
#include "dev_motor.h"
#include "dev_servo.h"
#include "svc_motor_speed.h"
#include "svc_servo_motion.h"
#include "app_chassis.h"
#include "app_motion.h"
#include "app_motion_scheduler.h"
#include "app_can.h"
#include "oled.h"
#include "oled_ui.h"
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
  .priority = (osPriority_t) osPriorityHigh4,
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
/* Definitions for Servo_T */
osThreadId_t Servo_THandle;
uint32_t Servo_TBuffer[ 128 ];
osStaticThreadDef_t Servo_TControlBlock;
const osThreadAttr_t Servo_T_attributes = {
  .name = "Servo_T",
  .cb_mem = &Servo_TControlBlock,
  .cb_size = sizeof(Servo_TControlBlock),
  .stack_mem = &Servo_TBuffer[0],
  .stack_size = sizeof(Servo_TBuffer),
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
/* Definitions for Motion_T */
osThreadId_t Motion_THandle;
uint32_t Motion_TBuffer[ 256 ];
osStaticThreadDef_t Motion_TControlBlock;
const osThreadAttr_t Motion_T_attributes = {
  .name = "Motion_T",
  .cb_mem = &Motion_TControlBlock,
  .cb_size = sizeof(Motion_TControlBlock),
  .stack_mem = &Motion_TBuffer[0],
  .stack_size = sizeof(Motion_TBuffer),
  .priority = (osPriority_t) osPriorityHigh5,
};
/* Definitions for Motor_T */
osThreadId_t Motor_THandle;
uint32_t Motor_TBuffer[ 256 ];
osStaticThreadDef_t Motor_TControlBlock;
const osThreadAttr_t Motor_T_attributes = {
  .name = "Motor_T",
  .cb_mem = &Motor_TControlBlock,
  .cb_size = sizeof(Motor_TControlBlock),
  .stack_mem = &Motor_TBuffer[0],
  .stack_size = sizeof(Motor_TBuffer),
  .priority = (osPriority_t) osPriorityHigh5,
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
/* Definitions for IIC_LOCK */
osMutexId_t IIC_LOCKHandle;
const osMutexAttr_t IIC_LOCK_attributes = {
  .name = "IIC_LOCK"
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
void Task_Servo(void *argument);
void Task_OLED(void *argument);
void Task_Motion(void *argument);
void Task_Motor(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */
  /* 全系统初始化：在 osKernelStart() 之前、任何线程存在之前跑完。
     顺序自底向上：定时器频率 → 设备 → 服务 → APP。
     这条链不碰任何 OS 对象，所以放最早；要互斥量的在 RTOS_MUTEX 段，
     要队列的在 RTOS_QUEUES 段，见下面两段。
     任一步失败立即 Error_Handler()：此刻没有线程在跑，停机是干净的。 */
  BSP_PWM_Init();
  if (Dev_Key_Init() != 0) {
    Error_Handler();
  }
  if (Dev_Motor_Init() != 0) {
    Error_Handler();
  }
  if (Dev_Servo_Init() != 0) {
    Error_Handler();
  }
  if (Svc_Motor_Speed_Init() != 0) {
    Error_Handler();
  }
  if (Svc_Servo_Motion_Init() != 0) {
    Error_Handler();
  }
  if (APP_Chassis_Init() != 0) {
    Error_Handler();
  }
  APP_Motion_Init();
  APP_Motion_Sched_Init();
  /* USER CODE END Init */
  /* Create the mutex(es) */
  /* creation of IIC_LOCK */
  IIC_LOCKHandle = osMutexNew(&IIC_LOCK_attributes);

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* IIC 总线登记：IIC_LOCK 已在上面的 "Create the mutex(es)" 里创建，
     MX_I2C2_Init() 也已在 main 里跑过，这里只做句柄校验。
     必须在建线程之前完成，否则 OLED 任务第一次刷屏会拿到未初始化的总线。 */
  (void)BSP_IIC_Init();
  /* OLED 面板：整屏刷新走 I2C，要等 IIC_LOCK 就位；上电初始化约 23 ms，
     放在线程之前，线程里就只剩刷屏。 */
  OLED_Init();
  OLED_UI_FpsInit();
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
  /* 串口链路：队列/信号量都建好之后、线程启动之前初始化。
     这里会建立 GFIFO 字节队列并启动 DMA 循环接收，
     早于本段则 OS 对象还不存在，晚于线程启动则可能丢开头几个字节。 */
  (void)Svc_Uart_Init();
  /* CAN 链路：FDCAN 一启动接收，中断就会直接往 CAN_RXF0_Q 投帧，
     所以 Init/Open 必须排在 CAN_RXF0_Q 建好之后。 */
  if (APP_CAN_Init() != 0) {
    Error_Handler();
  }
  if (APP_CAN_Open(APP_CAN_PORT_1) != 0) {
    Error_Handler();
  }
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

  /* creation of Servo_T */
  Servo_THandle = osThreadNew(Task_Servo, NULL, &Servo_T_attributes);

  /* creation of OLED_T */
  OLED_THandle = osThreadNew(Task_OLED, NULL, &OLED_T_attributes);

  /* creation of Motion_T */
  Motion_THandle = osThreadNew(Task_Motion, NULL, &Motion_T_attributes);

  /* creation of Motor_T */
  Motor_THandle = osThreadNew(Task_Motor, NULL, &Motor_T_attributes);

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
    /* 标定完成后取消注释，恢复 PID 调参数据发送 */
    //APP_Debug_Send_Motor_PID(2);
    osDelay(50);
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

/* USER CODE BEGIN Header_Task_Servo */
/**
* @brief Function implementing the Servo_T thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Task_Servo */
__weak void Task_Servo(void *argument)
{
  /* USER CODE BEGIN Task_Servo */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END Task_Servo */
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

/* USER CODE BEGIN Header_Task_Motion */
/**
* @brief Function implementing the Motion_T thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Task_Motion */
__weak void Task_Motion(void *argument)
{
  /* USER CODE BEGIN Task_Motion */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END Task_Motion */
}

/* USER CODE BEGIN Header_Task_Motor */
/**
* @brief Function implementing the Motor_T thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Task_Motor */
__weak void Task_Motor(void *argument)
{
  /* USER CODE BEGIN Task_Motor */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END Task_Motor */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

