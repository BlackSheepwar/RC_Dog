/**
 * @file app_uart_cfg.h
 * @brief UART 端口注册参数配置表
 * @author 李嘉图
 * @date 2026-06-05
 *
 * @note 将 task_uart_rx.c 中的 UART 端口注册参数提取到此处，
 *       Task 层通过循环遍历配置表完成注册，不再硬编码具体句柄。
 */

#ifndef __APP_UART_CFG_H__
#define __APP_UART_CFG_H__

/*==============================================================================
 * 头文件包含
 *============================================================================*/
#include "main.h"
#include "usart.h"

/*==============================================================================
 * UART 端口注册参数配置
 *============================================================================*/
typedef struct {
    uint8_t           id;
    UART_HandleTypeDef *huart;
} uart_port_cfg_t;

/* UART 端口注册表：{id, huart}（DMA句柄从 huart->hdmarx 获取） */
static const uart_port_cfg_t UART_PORT_TABLE[] = {
    { .id = 1, .huart = &huart4 },
    { .id = 2, .huart = &huart5 },
};

#endif /* __APP_UART_CFG_H__ */
