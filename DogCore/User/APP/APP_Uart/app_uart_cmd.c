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
#include "app_gait_seq.h"
#include "app_motion_scheduler.h"
#include <stdint.h>

/*==============================================================================
 * 命令处理函数
 *============================================================================*/
static void APP_UART_AA(uint8_t *payload, uint8_t len)
{
    BSP_GPIO_Toggle(2);
}
 
static void APP_UART_F0(uint8_t *payload, uint8_t len)
{
    static uint8_t i = 0;
    if (i == 0) {
        MotionSched_Start(&GAIT_KICK0);  
        i = i+1;
    }
    else{
        MotionSched_Start(&GAIT_KICK2);
        i = i-1;
    }
} 

static void APP_UART_F1(uint8_t *payload, uint8_t len)
{
    MotionSched_Start(&GAIT_KICK2);
}  

static void APP_UART_F2(uint8_t *payload, uint8_t len)
{
    MotionSched_Start(&GAIT_KICK1);
}

static void APP_UART_F3(uint8_t *payload, uint8_t len)
{

}

static void APP_UART_F4(uint8_t *payload, uint8_t len)
{

}

static void APP_UART_F5(uint8_t *payload, uint8_t len)
{

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
    { .cmd = 0xF3, .handler = APP_UART_F3 },
    { .cmd = 0xF4, .handler = APP_UART_F4 },
    { .cmd = 0xF5, .handler = APP_UART_F5 },
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
