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
#include "tim.h"
#include "cmsis_os.h"
// 功能包含
#include "app_uart.h"
#include "app_servo.h"
#include "bsp_gpio.h"
#include "app_gait_seq.h"
#include "app_motion_scheduler.h"
#include "app_motor.h"
#include "app_motor_debug.h"

/*==============================================================================
 * 命令处理函数
 *============================================================================*/
static void APP_UART_FF(uint8_t *payload, uint8_t len)
{
    BSP_GPIO_Toggle(2);
}

static void APP_UART_01(uint8_t *payload, uint8_t len)
{
    static uint8_t i = 0;
    if (i == 0) {
        MotionSched_Start(&GAIT_Walk_2_Front);
        APP_Motor_SetSpeed(1, 150);
        APP_Motor_SetSpeed(2, -150);
        APP_Motor_SetSpeed(3, 150);
        APP_Motor_SetSpeed(4, -150);
        
        i = i+1;
    }
    else{
        MotionSched_Start(&GAIT_Init);
        APP_Motor_SetSpeed(1, 0);
        APP_Motor_SetSpeed(2, 0);
        APP_Motor_SetSpeed(3, 0);
        APP_Motor_SetSpeed(4, 0);
        i = i-1;
    }
}

static void APP_UART_02(uint8_t *payload, uint8_t len)
{
    static uint8_t i = 0;
    if (i == 0) {
        MotionSched_Start(&GAIT_Walk_2_Queen);
        APP_Motor_SetSpeed(1, -150);
        APP_Motor_SetSpeed(2, 150);
        APP_Motor_SetSpeed(3, -150);
        APP_Motor_SetSpeed(4, 150);
        i = i+1;
    }
    else{
        MotionSched_Start(&GAIT_Init);
        APP_Motor_SetSpeed(1, 0);
        APP_Motor_SetSpeed(2, 0);
        APP_Motor_SetSpeed(3, 0);
        APP_Motor_SetSpeed(4, 0);
        i = i-1;
    }
}

static void APP_UART_11(uint8_t *payload, uint8_t len)
{
    BSP_GPIO_Toggle(2);
    static uint8_t i = 0;
    if (i == 0) {
    MotionSched_Start(&GAIT_KICK1);  
    APP_Motor_SetSpeed(1, 150);
    APP_Motor_SetSpeed(2, 150);
    APP_Motor_SetSpeed(3, 150);
    APP_Motor_SetSpeed(4, 150);
        i = i+1;
    }
    else{
    MotionSched_Start(&GAIT_Init);
    APP_Motor_SetSpeed(1, 0);
    APP_Motor_SetSpeed(2, 0);
    APP_Motor_SetSpeed(3, 0);
    APP_Motor_SetSpeed(4, 0);
        i = i-1;
    }
}

static void APP_UART_12(uint8_t *payload, uint8_t len)
{
    BSP_GPIO_Toggle(2);
    static uint8_t i = 0;
    if (i == 0) {
    MotionSched_Start(&GAIT_KICK1);  
    APP_Motor_SetSpeed(1, -150);
    APP_Motor_SetSpeed(2, -150);
    APP_Motor_SetSpeed(3, -150);
    APP_Motor_SetSpeed(4, -150);
        i = i+1;
    }
    else{
    MotionSched_Start(&GAIT_Init);
    APP_Motor_SetSpeed(1, 0);
    APP_Motor_SetSpeed(2, 0);
    APP_Motor_SetSpeed(3, 0);
    APP_Motor_SetSpeed(4, 0);
        i = i-1;
    }
}

static void APP_UART_21(uint8_t *payload, uint8_t len)
{
    static uint8_t i = 0;
    if (i == 0) {
        MotionSched_Start(&GAIT_Init);  
        i = i+1;
    }
    else{
        MotionSched_Start(&GAIT_Init_2);
        i = i-1;
    }
}

static void APP_UART_22(uint8_t *payload, uint8_t len)
{
    static uint8_t i = 0;
    if (i == 0) {
        MotionSched_Start(&GAIT_KICK1);  
        i = i+1;
    }
    else{
        MotionSched_Start(&GAIT_Init);
        i = i-1;
    }
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
    { .cmd = 0xFF, .handler = APP_UART_FF },
    // 移动
    { .cmd = 0x01, .handler = APP_UART_01 },
    { .cmd = 0x02, .handler = APP_UART_02 },
    // 转向
    { .cmd = 0x11, .handler = APP_UART_11 },
    { .cmd = 0x12, .handler = APP_UART_12 },
    // 姿态调整
    { .cmd = 0x21, .handler = APP_UART_21 },
    { .cmd = 0x22, .handler = APP_UART_22 },
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
