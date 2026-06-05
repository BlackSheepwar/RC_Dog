/**
 * @file task_key.c
 * @brief 实现按键扫描与消息发送任务
 * @author 李嘉图
 * @date 2026-05-08
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
#include "main.h"
#include "app_key.h"

/*==============================================================================
 * 任务函数
 *============================================================================*/
void Task_KEY(void *argument)
{
  App_Key_Init();
  App_Key_Register(1, 18, 1, GPIOB, GPIO_PIN_0);
  App_Key_Register(2, 18, 1, GPIOA, GPIO_PIN_7);
  App_Key_Register(3, 18, 1, GPIOA, GPIO_PIN_6);
  App_Key_Register(4, 18, 1, GPIOA, GPIO_PIN_5);
  App_Key_Register(5, 18, 1, GPIOA, GPIO_PIN_4);
  App_Key_Register(6, 18, 1, GPIOA, GPIO_PIN_3);
  for(;;)
  {
    App_Key_Update();
    osDelay(10);
  }
}