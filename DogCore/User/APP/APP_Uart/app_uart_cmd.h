/**
 * @file app_uart_cmd.h
 * @brief 命令分发
 * @author 李嘉图
 * @date 2026-5-4
 */

#ifndef __APP_UART_CMD_H__
#define __APP_UART_CMD_H__

#include <stdint.h>

/**
 * @brief 命令分发函数
 * @param cmd 命令字
 * @param payload 命令数据
 * @param len 数据长度
 */
void APP_UART_Cmd(uint8_t cmd, uint8_t *payload, uint8_t len);

#endif
