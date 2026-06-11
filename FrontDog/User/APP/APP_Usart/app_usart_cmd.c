/**
 * @file app_usart_cmd.c
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
#include "main.h"
#include <stdint.h>
#include "app_usart_cmd.h"
#include "app_usart.h"
#include "app_servo.h"
#include "common.h"

/*==============================================================================
 * 命令处理函数
 *============================================================================*/
static void APP_USART_AA(uint8_t *payload, uint8_t len)
{
    (void)payload;
    (void)len;
    APP_Servo_SetTarget(1, -125);
}

static void APP_USART_A9(uint8_t *payload, uint8_t len)
{
    (void)payload;
    (void)len;
    APP_Servo_SetTarget(1, 125);
}

static void APP_USART_A8(uint8_t *payload, uint8_t len)
{
    (void)payload;
    (void)len;
    for (uint8_t i = 0; i < 10; i++)
    {
        APP_USART_BuildTxPacket(1, 0xA8, &i, 1);
    }
}

/*==============================================================================
 * 命令配置表
 *============================================================================*/
typedef struct {
    uint8_t  cmd;
    void     (*handler)(uint8_t *payload, uint8_t len);
} usart_cmd_entry_t;

static const usart_cmd_entry_t CMD_TABLE[] = {
    { .cmd = 0xAA, .handler = APP_USART_AA },
    { .cmd = 0xA9, .handler = APP_USART_A9 },
    { .cmd = 0xA8, .handler = APP_USART_A8 },
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
void APP_USART_Cmd(uint8_t cmd, uint8_t *payload, uint8_t len)
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
