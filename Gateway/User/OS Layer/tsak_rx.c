/**
 * @file tsak_rx.c
 * @brief 实现USART接收处理任务
 * @author 李嘉图
 * @date 2026-5-4
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
#include "main.h"
#include "app_usart.h"
#include "usart.h"

/*==============================================================================
 * 外部变量声明
 *============================================================================*/
extern UART_HandleTypeDef huart1;
extern DMA_HandleTypeDef hdma_usart1_rx;

/*==============================================================================
 * 任务函数
 *============================================================================*/
/**
 * @brief RXTask 任务入口函数
 * @param argument 任务参数（未使用）
 */
void StartRXTask(void *argument)
{
  APP_USART_Init();
  APP_USART_RegisterPort(1, &huart1, &hdma_usart1_rx);
  uint8_t id;

 for (;;)
 {
   osMessageQueueGet(RXQueueHandle, &id, NULL, osWaitForever);

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