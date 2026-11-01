/**
 * @file app_uart.c
 * @brief 串口业务层实现：向链路服务提交发送帧
 * @author 李嘉图
 * @date 2026-10-31
 *
 * @note 这里原来有 600 行 DMA 环形缓冲、共享池、描述符 FIFO 的代码，
 *       现在全部由 Service 层（Svc_Uart + Svc_Proto）与 BSP 层的 GFIFO
 *       承担，本文件只剩"业务出口"这一个职责。
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include <stdint.h>
#include "app_uart.h"

/*==============================================================================
 * 发送
 *============================================================================*/
/**
 * @brief 向指定链路发送一帧命令
 * @param id      目标链路，取 APP_UART_PORT_x
 * @param cmd     命令字
 * @param payload 载荷，len 为 0 时可为 NULL
 * @param len     载荷长度
 * @retval 0: 已提交发送队列
 * @retval -1: 参数非法或队列满
 */
int APP_UART_Send(uint8_t id, uint8_t cmd, const uint8_t *payload, uint8_t len)
{
    return Svc_Uart_Send(id, cmd, payload, len);
}
