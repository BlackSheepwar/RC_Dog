/**
 * @file task_par.c
 * @brief 实现按键控制任务，通过消息队列接收指令
 * @author 李嘉图
 * @date 2026-05-08
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
#include "main.h"
#include "app_key.h"
#include "app_key_cmd.h"
#include "stm32f103xb.h"
#include "stm32f1xx_hal_gpio.h"

/*==============================================================================
 * 任务函数
 *============================================================================*/
void StartPARTask(void *argument)
{
  App_Key_Init();

  App_Key_Register(1, 18, 1, GPIOB, GPIO_PIN_0);
  App_Key_Register(2, 18, 1, GPIOA, GPIO_PIN_7);
  App_Key_Register(3, 18, 1, GPIOA, GPIO_PIN_6);
  App_Key_Register(4, 18, 1, GPIOA, GPIO_PIN_5);
  App_Key_Register(5, 18, 1, GPIOA, GPIO_PIN_4);
  App_Key_Register(6, 18, 1, GPIOA, GPIO_PIN_3);
  
  Debounce_Event_packet_t msg;
  for(;;)
  {
    osMessageQueueGet(KEYQueueHandle, &msg, NULL, osWaitForever);
    uint8_t id = msg.id;
    Debounce_Event_t event = msg.event;
    App_Key_CMD_Packet(id, event);
  }
}