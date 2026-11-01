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

/*==============================================================================
 * 串口压力测试计数接口
 *============================================================================*/
/**
 * @brief 清零压力测试计数器
 */
void APP_UART_ResetStressCounts(void);

/**
 * @brief 读取压力测试计数器
 * @param[out] aa  0xAA 命令收到次数
 * @param[out] a9  0xA9 命令收到次数
 * @param[out] a8  0xA8 命令收到次数
 */
void APP_UART_GetStressCounts(uint32_t *aa, uint32_t *a9, uint32_t *a8);

#endif
