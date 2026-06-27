/**
 * @file task_uart_tx.c
 * @brief UART 发送处理任务（TX 池+描述符 FIFO 方式）
 * @author 李嘉图
 * @date 2026-06-27
 *
 * @note APP_UART_GetTxFrame 阻塞取帧 → 等待 DMA 空闲 → 启动 DMA 发送。
 *
 *       帧数据写入 TX 池，描述符入 FIFO，信号量通知此任务。
 *       无 ISR 预装，无双缓冲，无 RTOS 消息队列大帧拷贝。
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
    uint8_t buf[APP_TX_BUF_SIZE];   // 需要 DMA 访问
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
