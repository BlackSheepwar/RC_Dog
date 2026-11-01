/**
 * @file task_key_cmd.c
 * @brief 按键命令线程（入口名由 CubeMX .ioc 决定，不能改）：取一条事件，交给 APP 命令表
 * @author Ricardo
 * @date 2026-11-01
 *
 * @note 与扫描线程分开的理由：扫描必须准时（10 ms 一拍），
 *       而命令处理可能很慢（切步态、动电机）。合成一个线程时，
 *       一次慢命令就会让消抖计时失真，长按判定跟着飘。
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include <stdint.h>
#include "cmsis_os2.h"
#include "main.h"               /* KEY_CMD_QHandle */
// 功能包含
#include "app_key_cmd.h"

/*==============================================================================
 * 线程函数
 *============================================================================*/
/**
 * @brief 按键命令线程（入口名由 CubeMX .ioc 决定，不能改）
 * @param argument 线程参数（未使用）
 * @retval 无
 * @note 常驻阻塞在队列上，取到一条事件就分发一次。
 */
void TASK_KEY_CMD(void *argument)
{
    key_event_packet_t msg;

    (void)argument;

    for (;;) {
        if (osMessageQueueGet(KEY_CMD_QHandle, &msg, NULL, osWaitForever) != osOK) {
            continue;
        }

        APP_Key_Cmd_Dispatch(msg.key_id, msg.event);
    }
}
