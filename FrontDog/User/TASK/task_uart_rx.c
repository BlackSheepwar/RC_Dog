/**
 * @file task_uart_rx.c
 * @brief UART 接收处理任务
 * @author 李嘉图
 * @date 2026-06-20
 *
 * @note 收到 BSP 中断通知后，从 DMA 循环缓冲读取数据、
 *       解析数据包，入描述符 FIFO，然后唤醒 CMD 任务处理。
 *
 *       命令分发已拆分到 UART_RX_CMD 任务。
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include <stdint.h>
#include "main.h"
#include "common.h"
// 功能包含
#include "app_uart.h"

/*==============================================================================
 * 任务函数
 *============================================================================*/
void Task_UART_RX(void *argument)
{
    /* 初始化 BSP + 所有串口端口 */
    APP_UART_Init();

    uint8_t id;

    /* 主循环：等待中断通知 → 处理数据 → 通知 CMD 任务 */
    for (;;)
    {
        osMessageQueueGet(UART_RX_QHandle, &id, NULL, osWaitForever);

        /* 从 DMA 循环缓冲读取新数据并解析（入描述符 FIFO） */
        APP_UART_ProcessRxData(id);

        /* 唤醒 CMD 任务消费描述符 */
        osSemaphoreRelease(UART_RX_BSHandle);
    }
}
