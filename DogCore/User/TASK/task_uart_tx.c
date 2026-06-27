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
    static uint8_t buf[APP_TX_BUF_SIZE] __attribute__((section(".dma_buffer")));
    uint8_t id, len;

    for (;;)
    {
        /* 1. 阻塞等待帧数据（从 tx_pool 读出） */
        APP_UART_GetTxFrame(buf, &id, &len);

        /* 2. 等待前一次 DMA 发送完成 */
        while (BSP_UART_IsTxBusy(id))
            osDelay(1);

        /* 3. 启动 DMA 发送 */
        BSP_UART_SendDMA(id, buf, len);
    }
}
