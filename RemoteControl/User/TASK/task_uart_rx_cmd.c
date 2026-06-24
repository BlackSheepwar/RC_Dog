/**
 * @file task_uart_rx_cmd.c
 * @brief UART 命令分发任务
 * @author 李嘉图
 * @date 2026-06-20
 *
 * @note 等待 UART_RX_BS 信号量 → 从描述符 FIFO 弹出包 → 调用命令处理。
 *       通过独立任务，将命令处理与 DMA 数据读取（Task_UART_RX）分离。
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
 *
 * CubeMX 在 freertos.c 中生成了该任务的弱定义，
 * 此处强定义覆盖之。
 *============================================================================*/
void Task_UART_RX_CMD(void *argument)
{
    for (;;)
    {
        /* 等待 RX 任务通知（有已解析的数据包待处理） */
        osSemaphoreAcquire(UART_RX_BSHandle, osWaitForever);

        /* 消费所有已解析的包 */
        uint8_t count = 0;
        while (APP_UART_SendRxPacket())
        {
            count++;

            /* 每连续处理 5 个包让出 CPU */
            if (count >= 5)
            {
                count = 0;
                osDelay(2);
            }
        }
    }
}
