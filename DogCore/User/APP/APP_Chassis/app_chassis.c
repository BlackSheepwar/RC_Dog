/**
 * @file app_chassis.c
 * @brief 底盘业务层实现：轮号 → 速度环句柄
 * @author Ricardo
 * @date 2026-11-01
 *
 * @note 轮号的约定只有一处：数组下标 = 轮号 - 1。
 *       1/3 是左侧（左前、左后），2/4 是右侧（右前、右后），
 *       与 Dev_Motor 的实例顺序一致；要改成 1/2 左、3/4 右只改 Init 里的一行映射。
 *
 *       本层没有 PID、没有占空比、没有时间：目标值写进速度环就结束，
 *       剩下的由 Svc_Motor_Speed 在 10 ms 节拍里完成。
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include <stddef.h>
#include "common.h"             /* ARRAY_SIZE */
#include "app_chassis.h"
// 功能包含
#include "dev_motor.h"
#include "svc_motor_speed.h"

/*==============================================================================
 * 内部变量
 *============================================================================*/
/** @brief 轮号 → 速度环句柄（下标 = 轮号 - 1） */
static svc_motor_speed_t *s_wheel[APP_CHASSIS_WHEELS];
/** @brief 已接管的轮子数 */
static uint8_t s_count = 0u;

/*==============================================================================
 * 内部函数
 *============================================================================*/
/**
 * @brief 轮号 → 速度环句柄
 * @param wheel 轮号，1 起
 * @retval 非 NULL: 句柄
 * @retval NULL: 轮号非法
 */
static svc_motor_speed_t *APP_Chassis_Wheel(uint8_t wheel)
{
    if ((wheel < 1u) || (wheel > s_count)) {
        return NULL;
    }

    return s_wheel[wheel - 1u];
}

/*==============================================================================
 * 接口实现
 *============================================================================*/
/**
 * @brief 把设备层的电机句柄换成速度环句柄
 * @retval 0: 成功
 * @retval -1: 设备数量不足，或某个电机没被速度环接管
 */
int APP_Chassis_Init(void)
{
    uint8_t dev_count = Dev_Motor_Count();

    if (dev_count < APP_CHASSIS_WHEELS) {
        return -1;
    }

    s_count = APP_CHASSIS_WHEELS;

    for (uint8_t i = 0u; i < s_count; i++) {
        /* 设备句柄 → 速度环句柄：编号到这里为止 */
        s_wheel[i] = Svc_Motor_Speed_Of(Dev_Motor_Get(i));

        if (s_wheel[i] == NULL) {
            return -1;
        }
    }

    return 0;
}

/**
 * @brief 设置单个轮子的速度
 * @param wheel    轮号，1 ~ APP_CHASSIS_WHEELS
 * @param velocity 速度刻度值，正值 = 轮子往前
 * @retval 无
 */
void APP_Chassis_Set_Wheel(uint8_t wheel, float velocity)
{
    Svc_Motor_Speed_Set_Target(APP_Chassis_Wheel(wheel), velocity);
}

/**
 * @brief 四个轮子同时给同一速度
 * @param velocity 速度刻度值，正值 = 整车前进
 * @retval 无
 */
void APP_Chassis_Set_All(float velocity)
{
    for (uint8_t i = 0u; i < s_count; i++) {
        Svc_Motor_Speed_Set_Target(s_wheel[i], velocity);
    }
}

/**
 * @brief 四个轮子全部停转
 * @retval 无
 */
void APP_Chassis_Stop(void)
{
    APP_Chassis_Set_All(0.0f);
}

/**
 * @brief 查询某个轮子的目标速度
 * @param wheel 轮号，1 ~ APP_CHASSIS_WHEELS
 * @retval 目标速度；轮号非法时返回 0
 */
float APP_Chassis_Get_Wheel_Velocity(uint8_t wheel)
{
    svc_motor_speed_t *speed = APP_Chassis_Wheel(wheel);
    float              target = 0.0f;

    if (speed == NULL) {
        return 0.0f;
    }

    (void)Svc_Motor_Speed_Get_State(speed, &target, NULL, NULL);
    return target;
}
