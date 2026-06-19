/**
 * @file task_uart_rx.c
 * @brief 实现UART接收处理任务
 * @author 李嘉图
 * @date 2026-06-05
 *
 * @note UART 端口注册参数已提取到 app_uart_cfg.h，
 *       Task 层遍历配置表完成注册，新增串口只需在表中加一项。
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
#include "app_uart_cfg.h"

/*==============================================================================
 * 任务函数
 *============================================================================*/
void Task_UART_RX(void *argument)
{
    APP_UART_Init();

    for (uint8_t i = 0; i < ARRAY_SIZE(UART_PORT_TABLE); i++)
    {
        APP_UART_RegisterPort(UART_PORT_TABLE[i].id,
                               UART_PORT_TABLE[i].huart);
    }

    uint8_t id;

    for (;;)
    {
        osMessageQueueGet(UART_RX_QHandle, &id, NULL, osWaitForever);

        APP_UART_BuildRxPacket(id);

        uint8_t count = 0;

        while (1)
        {
            if (APP_UART_SendRxPacket() == 0)
                break;

            count++;

            // 每连续处理5个包，让出CPU
            if (count >= 5)
            {
                count = 0;
                osDelay(2);
            }
        }
    }
}