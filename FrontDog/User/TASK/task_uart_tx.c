/**
 * @file task_uart_tx.c
 * @brief UART 发送处理任务（双缓冲 DMA）
 * @author 李嘉图
 * @date 2026-06-20
 *
 * @note 等待队列 → 区分数据帧与标记帧，经 APP 双缓冲发送。
 *
 *       数据帧（len > 0）：通过双缓冲发送或预装。
 *       标记帧（len == 0）：ISR 发回的 TX 完成通知，
 *       任务趁机从队列取帧装填空闲缓冲。
 *
 *       双缓冲（ping-pong）：一发一装，消除帧间间隙。
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include "main.h"
// 功能包含
#include "app_uart.h"

/*==============================================================================
 * 任务函数
 *============================================================================*/
void Task_UART_TX(void *argument)
{
    APP_TxFrame_t frame;

    for (;;)
    {
        /* 1. 等待队列（数据帧或标记帧） */
        osMessageQueueGet(UART_TX_QHandle, &frame, NULL, osWaitForever);

        /* 2. 标记帧 → TX 完成通知，尝试预装下一帧 */
        if (frame.len == 0)
        {
            APP_UART_OnTxComplete(frame.id);
            continue;
        }

        /* 3. 数据帧 → 双缓冲发送 */
        APP_UART_TrySendDual(&frame);
    }
}
