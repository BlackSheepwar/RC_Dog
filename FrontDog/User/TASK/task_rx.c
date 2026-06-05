/**
 * @file task_rx.c
 * @brief 实现USART接收处理任务
 * @author 李嘉图
 * @date 2026-06-05
 *
 * @note USART 端口注册参数已提取到 app_usart_cfg.h，
 *       Task 层遍历配置表完成注册，新增串口只需在表中加一项。
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
#include "main.h"
#include "app_usart.h"
#include "app_usart_cfg.h"
#include "common.h"

/*==============================================================================
 * 任务函数
 *============================================================================*/
void Task_RX(void *argument)
{
    APP_USART_Init();

    for (uint8_t i = 0; i < ARRAY_SIZE(USART_PORT_TABLE); i++)
    {
        APP_USART_RegisterPort(USART_PORT_TABLE[i].id,
                               USART_PORT_TABLE[i].huart);
    }

    uint8_t id;

    for (;;)
    {
        osMessageQueueGet(RX_QHandle, &id, NULL, osWaitForever);

        APP_USART_BuildRxPacket(id);

        uint8_t count = 0;

        while (1)
        {
            if (APP_USART_SendRxPacket() == 0)
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