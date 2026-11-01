/**
 * @file main.h
 * @brief 主机端测试替身：只声明 Service 层用到的 OS 对象句柄
 * @author 李嘉图
 * @date 2026-10-31
 *
 * @note 固件里这些句柄由 CubeMX 生成在 freertos.c；
 *       测试时只需要声明，链接阶段由测试自己的桩实现提供。
 */

#ifndef __MAIN_H__
#define __MAIN_H__

#include "cmsis_os2.h"

extern osMessageQueueId_t UART_RX_QHandle;
extern osMessageQueueId_t UART_TX_QHandle;
extern osSemaphoreId_t    UART_RX_BSHandle;
extern osSemaphoreId_t    UART_TX_CSHandle;

#endif /* __MAIN_H__ */
