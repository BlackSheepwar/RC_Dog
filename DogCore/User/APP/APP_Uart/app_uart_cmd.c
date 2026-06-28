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
#include "app_servo.h"
#include "bsp_gpio.h"

/*==============================================================================
 * 命令处理函数
 *============================================================================*/
static void APP_UART_AA(uint8_t *payload, uint8_t len)
{
    BSP_GPIO_Toggle(2);
}

static void APP_UART_F0(uint8_t *payload, uint8_t len)
{
    APP_Servo_SetTarget(1, -38);
    APP_Servo_SetTarget(2, 27);
} 

static void APP_UART_F1(uint8_t *payload, uint8_t len)
{
    APP_Servo_SetTarget(1, -77);
    APP_Servo_SetTarget(2, -63);
}  

static void APP_UART_F2(uint8_t *payload, uint8_t len)
{
    APP_Servo_SetTarget(1, 0);
    APP_Servo_SetTarget(2, 90);
}

/**
 * @brief 挡位调速命令（0xE0）
 * @param payload[0] 挡位 0~10，0=停止，1=20°/s … 10=200°/s
 * @param len 数据长度
 * @note 速算：speed_dps = payload[0] * 20，超范围截断到 10
 */
static void APP_UART_E0(uint8_t *payload, uint8_t len)
{
    if (payload == NULL || len == 0) return;

    uint8_t gear = payload[0];
    if (gear > 10) gear = 10;

    float speed = (float)gear * 20.0f;
    APP_Servo_SetSpeed(1, speed);
    APP_Servo_SetSpeed(2, speed);
}

/*==============================================================================
 * 命令配置表
 *============================================================================*/
typedef struct {
    uint8_t  cmd;
    void     (*handler)(uint8_t *payload, uint8_t len);
} uart_cmd_entry_t;

static const uart_cmd_entry_t CMD_TABLE[] = {
    // 测试命令
    { .cmd = 0xAA, .handler = APP_UART_AA },
    // 伺服命令
    { .cmd = 0xF0, .handler = APP_UART_F0 },
    { .cmd = 0xF1, .handler = APP_UART_F1 },
    { .cmd = 0xF2, .handler = APP_UART_F2 },
    // 挡位调速命令
    { .cmd = 0xE0, .handler = APP_UART_E0 },
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
