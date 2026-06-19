/**
 * @file task_can_tx.c
 * @brief CAN TX 服务任务
 * @author 李嘉图
 * @date 2026-06-19
 *
 * @note 等待 CAN_TX_BSHandle 二进制信号量（由 BSP 层 TX 完成中断释放），
 *       排空 APP 层软件发送缓冲中的待发帧。
 *       将耗时的 HAL 发送操作放在任务上下文中执行，避免中断处理过长。
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include <stdint.h>
#include "main.h"           /* CAN_TX_BSHandle, osSemaphoreId_t */
#include "common.h"
// 功能包含
#include "app_can.h"
#include "bsp_can.h"

/*==============================================================================
 * 任务函数
 *============================================================================*/
/**
 * @brief CAN TX 服务任务
 * @param argument 未使用
 *
 * @note 无限循环等待 CAN_TX_BSHandle，收到信号量后排空所有端口的发送缓冲。
 *       排空逻辑内部会处理硬件邮箱满的情况（未发出帧重新入队）。
 */
void Task_CAN_TX(void *argument)
{
    for (;;)
    {
        /* ---------- 等待 TX 完成信号量 ---------- */
        osSemaphoreAcquire(CAN_TX_BSHandle, osWaitForever);

        /* ---------- 排空所有端口的发送缓冲 ---------- */
        for (uint8_t i = 0; i < BSP_CAN_MAX_NUM; i++)
        {
            APP_CAN_FlushTxBuf(i);
        }
    }
}
