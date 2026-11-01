/**
 * @file app_uart_cmd.c
 * @brief 串口命令表：命令字 → 步态/轮速/姿态动作
 * @author Ricardo
 * @date 2026-11-01
 *
 * @note 行为模型只有两种，先立住再谈细化：
 *       - 动作类命令（走、转、后退、滚轮）：同一条命令按一次进入运动、
 *         再按一次回站立；用一个模块级状态 s_moving 记当前是"运动"还是"静止"，
 *         不再用函数内的 static 变量各记一份（那种写法改一个命令就会漏）。
 *       - 姿态类命令（站立/趴伏/竖直）：直接切步态，并把运动状态清回静止。
 *
 *       轮速一律用 APP_Chassis_Set_All() 给"整车前进速度"：
 *       镜面电机的极性在 Dev_Motor 里换算，这里不再出现 200/-200/200/-200。
 *
 *       这里只写产品动作，不写协议解析、不碰 GPIO 电平、不碰 OS。
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include <stddef.h>
#include "common.h"             /* ARRAY_SIZE */
// 功能包含
#include "app_uart.h"
#include "app_motion.h"
#include "app_motion_scheduler.h"
#include "app_gait_seq.h"       /* GAIT_xxx 步态表 */
#include "app_chassis.h"
#include "dev_led.h"

/*==============================================================================
 * 内部变量
 *============================================================================*/
/** @brief 运动状态：0 = 静止（站立），1 = 正在执行运动命令 */
static uint8_t s_moving = 0u;

/*==============================================================================
 * 内部函数
 *============================================================================*/
/**
 * @brief 运动命令的开关切换
 * @param gait     要执行的步态，可为 NULL（表示只动轮子、不变姿态）
 * @param velocity 整车前进速度（刻度值，正值向前）
 * @retval 无
 * @note 第一次调用进入运动，再次调用回站立并停轮。
 */
static void APP_UART_Move_Toggle(const motion_gait_t *gait, float velocity)
{
    if (s_moving == 0u) {
        if (gait != NULL) {
            APP_Motion_Sched_Start(gait);
        }
        APP_Chassis_Set_All(velocity);
        s_moving = 1u;
        return;
    }

    APP_Motion_Sched_Start(&GAIT_Stand);
    APP_Chassis_Stop();
    s_moving = 0u;
}

/**
 * @brief 直接切一个姿态并停轮
 * @param gait 目标步态，不可为 NULL
 * @retval 无
 */
static void APP_UART_Posture(const motion_gait_t *gait)
{
    APP_Motion_Sched_Start(gait);
    APP_Chassis_Stop();
    s_moving = 0u;
}

/*==============================================================================
 * 命令处理函数
 *============================================================================*/
/**
 * @brief 0xFF 测试：翻转指示灯
 * @param payload 载荷（未使用）
 * @param len     载荷长度（未使用）
 * @retval 无
 */
static void APP_UART_Cmd_Test(uint8_t *payload, uint8_t len)
{
    (void)payload;
    (void)len;

    Dev_LED_Toggle(&dev_led_0);
}

/**
 * @brief 0x01 前进（步态 + 轮子同向推进）
 * @param payload 载荷（未使用）
 * @param len     载荷长度（未使用）
 * @retval 无
 */
static void APP_UART_Cmd_Walk_Front(uint8_t *payload, uint8_t len)
{
    (void)payload;
    (void)len;

    APP_UART_Move_Toggle(&GAIT_Walk_2_Front, APP_CHASSIS_SPEED_WALK);
}

/**
 * @brief 0x02 后退
 * @param payload 载荷（未使用）
 * @param len     载荷长度（未使用）
 * @retval 无
 */
static void APP_UART_Cmd_Walk_Back(uint8_t *payload, uint8_t len)
{
    (void)payload;
    (void)len;

    APP_UART_Move_Toggle(&GAIT_Walk_2_Queen, -APP_CHASSIS_SPEED_WALK);
}

/**
 * @brief 0x11 左转（慢）
 * @param payload 载荷（未使用）
 * @param len     载荷长度（未使用）
 * @retval 无
 */
static void APP_UART_Cmd_Turn_Left_Slow(uint8_t *payload, uint8_t len)
{
    (void)payload;
    (void)len;

    APP_UART_Move_Toggle(&GAIT_Trot_1, 50.0f);
}

/**
 * @brief 0x12 右转（慢）
 * @param payload 载荷（未使用）
 * @param len     载荷长度（未使用）
 * @retval 无
 */
static void APP_UART_Cmd_Turn_Right_Slow(uint8_t *payload, uint8_t len)
{
    (void)payload;
    (void)len;

    APP_UART_Move_Toggle(&GAIT_Trot_1, -50.0f);
}

/**
 * @brief 0x13 左转（快）
 * @param payload 载荷（未使用）
 * @param len     载荷长度（未使用）
 * @retval 无
 */
static void APP_UART_Cmd_Turn_Left_Fast(uint8_t *payload, uint8_t len)
{
    (void)payload;
    (void)len;

    APP_UART_Move_Toggle(&GAIT_Trot_1, 100.0f);
}

/**
 * @brief 0x14 右转（快）
 * @param payload 载荷（未使用）
 * @param len     载荷长度（未使用）
 * @retval 无
 */
static void APP_UART_Cmd_Turn_Right_Fast(uint8_t *payload, uint8_t len)
{
    (void)payload;
    (void)len;

    APP_UART_Move_Toggle(&GAIT_Trot_1, -100.0f);
}

/**
 * @brief 0x21 站立
 * @param payload 载荷（未使用）
 * @param len     载荷长度（未使用）
 * @retval 无
 */
static void APP_UART_Cmd_Stand(uint8_t *payload, uint8_t len)
{
    (void)payload;
    (void)len;

    APP_UART_Posture(&GAIT_Stand);
}

/**
 * @brief 0x22 趴伏
 * @param payload 载荷（未使用）
 * @param len     载荷长度（未使用）
 * @retval 无
 */
static void APP_UART_Cmd_Grovel(uint8_t *payload, uint8_t len)
{
    (void)payload;
    (void)len;

    APP_UART_Posture(&GAIT_Grovel);
}

/**
 * @brief 0x23 竖直站立
 * @param payload 载荷（未使用）
 * @param len     载荷长度（未使用）
 * @retval 无
 */
static void APP_UART_Cmd_Vertical(uint8_t *payload, uint8_t len)
{
    (void)payload;
    (void)len;

    APP_UART_Posture(&GAIT_Vertical);
}

/**
 * @brief 0x31 踱步（原地，轮子不动）
 * @param payload 载荷（未使用）
 * @param len     载荷长度（未使用）
 * @retval 无
 */
static void APP_UART_Cmd_Trot(uint8_t *payload, uint8_t len)
{
    (void)payload;
    (void)len;

    APP_UART_Move_Toggle(&GAIT_Trot_1, 0.0f);
}

/**
 * @brief 0x41 轮子正转（不变姿态）
 * @param payload 载荷（未使用）
 * @param len     载荷长度（未使用）
 * @retval 无
 */
static void APP_UART_Cmd_Wheel_Front(uint8_t *payload, uint8_t len)
{
    (void)payload;
    (void)len;

    APP_UART_Move_Toggle(NULL, APP_CHASSIS_SPEED_ROLL);
}

/**
 * @brief 0x42 轮子反转（不变姿态）
 * @param payload 载荷（未使用）
 * @param len     载荷长度（未使用）
 * @retval 无
 */
static void APP_UART_Cmd_Wheel_Back(uint8_t *payload, uint8_t len)
{
    (void)payload;
    (void)len;

    APP_UART_Move_Toggle(NULL, -APP_CHASSIS_SPEED_ROLL);
}

/*==============================================================================
 * 命令表
 *============================================================================*/
/**
 * @brief 命令表项（值类型：表元素）
 */
typedef struct {
    uint8_t  cmd;                                       /* 命令字 */
    void   (*handler)(uint8_t *payload, uint8_t len);    /* 处理函数 */
} app_uart_cmd_entry_t;

/** @brief 命令表 */
static const app_uart_cmd_entry_t APP_UART_CMD_TABLE[] = {
    { .cmd = 0xFFu, .handler = APP_UART_Cmd_Test },
    { .cmd = 0x01u, .handler = APP_UART_Cmd_Walk_Front },
    { .cmd = 0x02u, .handler = APP_UART_Cmd_Walk_Back },
    { .cmd = 0x11u, .handler = APP_UART_Cmd_Turn_Left_Slow },
    { .cmd = 0x12u, .handler = APP_UART_Cmd_Turn_Right_Slow },
    { .cmd = 0x13u, .handler = APP_UART_Cmd_Turn_Left_Fast },
    { .cmd = 0x14u, .handler = APP_UART_Cmd_Turn_Right_Fast },
    { .cmd = 0x21u, .handler = APP_UART_Cmd_Stand },
    { .cmd = 0x22u, .handler = APP_UART_Cmd_Grovel },
    { .cmd = 0x23u, .handler = APP_UART_Cmd_Vertical },
    { .cmd = 0x31u, .handler = APP_UART_Cmd_Trot },
    { .cmd = 0x41u, .handler = APP_UART_Cmd_Wheel_Front },
    { .cmd = 0x42u, .handler = APP_UART_Cmd_Wheel_Back },
};

/*==============================================================================
 * 命令分发
 *============================================================================*/
/**
 * @brief 按命令字查表并执行
 * @param cmd     命令字
 * @param payload 载荷，可为 NULL
 * @param len     载荷长度
 * @retval 无
 */
void APP_UART_Cmd(uint8_t cmd, uint8_t *payload, uint8_t len)
{
    for (uint8_t i = 0u; i < ARRAY_SIZE(APP_UART_CMD_TABLE); i++) {
        if (APP_UART_CMD_TABLE[i].cmd == cmd) {
            APP_UART_CMD_TABLE[i].handler(payload, len);
            return;
        }
    }
}
