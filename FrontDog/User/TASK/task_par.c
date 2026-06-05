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

/*==============================================================================
 * 任务函数
 *============================================================================*/
void Task_PAR(void *argument)
{
  Debounce_Event_packet_t msg;
  for(;;)
  {
    osMessageQueueGet(KEY_QHandle, &msg, NULL, osWaitForever);
    uint8_t id = msg.id;
    Debounce_Event_t event = msg.event;
    App_Key_CMD_Packet(id, event);
  }
}