/**
 * @file app_uart_cmd.c
 * @brief 命令分发（配置表驱动）
 * @author 李嘉图
 * @date 2026-6-5
 *
 * @note 原先的 switch(cmd) 已替换为命令配置表，
 *       新增命令只需在 CMD_TABLE 中添加一行。
 *       命令处理函数保持原有的 static 风格不变。
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include "main.h"
#include "common.h"
#include "app_uart_cmd.h"
// 功能包含
#include "app_uart.h"
#include "bsp_gpio.h"

/*==============================================================================
 * 命令处理函数
 *============================================================================*/
static void APP_UART_AA(uint8_t *payload, uint8_t len)
{
    BSP_GPIO_Toggle(9);
}

/*==============================================================================
 * 命令配置表
 *============================================================================*/
typedef struct {
    uint8_t  cmd;
    void     (*handler)(uint8_t *payload, uint8_t len);
} uart_cmd_entry_t;

static const uart_cmd_entry_t CMD_TABLE[] = {
    { .cmd = 0xAA, .handler = APP_UART_AA },
};

/*==============================================================================
 * 命令分发
 *============================================================================*/
/**
 * @brief 命令分发函数
 * @param cmd 命令字
 * @param payload 命令数据
 * @param len 数据长度
 */
void APP_UART_Cmd(uint8_t cmd, uint8_t *payload, uint8_t len)
{
    for (uint8_t i = 0; i < ARRAY_SIZE(CMD_TABLE); i++)
    {
        if (CMD_TABLE[i].cmd == cmd)
        {
            CMD_TABLE[i].handler(payload, len);
            return;
        }
    }
}
