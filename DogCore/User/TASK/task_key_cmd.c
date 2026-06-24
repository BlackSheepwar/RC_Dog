/**
 * @file task_key_cmd.c
 * @brief 按键命令分发任务（通过消息队列接收指令）
 * @author 李嘉图
 * @date 2026-06-18
 *
 * @note 接收 APP_Key 发来的按键事件消息，调用 App_Key_CMD_Packet 分发。
 *       与 task_key.c（按键扫描任务）构成生产-消费对。
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include <stdint.h>
#include "main.h"
// 功能包含
#include "app_key.h"
#include "app_key_cmd.h"

/*==============================================================================
 * 任务函数
 *============================================================================*/
void TASK_KEY_CMD(void *argument)
{
  App_Key_EventPacket_t msg;
  for(;;)
  {
    osMessageQueueGet(KEY_CMD_QHandle, &msg, NULL, osWaitForever);
    uint8_t key_id = msg.key_id;
    App_Key_Event_t event = msg.event;
    App_Key_CMD_Packet(key_id, event);
  }
}