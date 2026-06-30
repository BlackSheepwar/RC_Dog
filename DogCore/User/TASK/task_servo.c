/**
 * @file task_servo.c
 * @brief 舵机控制任务 — 注册 + 平滑输出
 * @author 李嘉图
 * @date 2026-06-05
 *
 * @note 舵机注册参数已提取到 app_servo_cfg.h，
 *       定时器频率配置已下沉到 bsp_pwm.c 的 PWM_TIM_CFG 表。
 *
 *       Task 层只做：初始化 → 周期调度（舵机平滑 + PWM 输出）。
 *       步态调度（Gait_IK + Limb）已拆到 Motion_T（task_motion.c）。
 *
 *       本任务只输出舵机目标角度，不参与步态/IK 计算。
 *       Motion_T 10ms 算好目标 → APP_Servo_SetTarget 写入，
 *       本任务 10ms 做浮点平滑 → PWM 输出。
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include <stdint.h>
#include "main.h"
#include "common.h"
// 功能包含
#include "app_servo.h"
#include "app_servo_cfg.h"

/*==============================================================================
 * 任务函数
 *
 * 初始化顺序（固定）：
 *   1. APP_Servo_Init()        — 清空舵机池，初始化 PWM 底层
 *   2. BSP_PWM_SetAllFreq()    — 按 BSP 内置表配置所有定时器频率
 *   3. APP_Servo_Add(...)      — 注册所有舵机实例
 *
 * 运行时（10ms 周期）：
 *   APP_Servo_Scheduler()      — 浮点平滑 → PWM 输出
 *
 * 步态调度（Gait_IK_Init / Gait_IK_Scheduler / Limb_Scheduler）
 * 已在 Motion_T 中独立运行。
 *============================================================================*/
void Task_Servo(void *argument)
{
    /* ---- 初始化 ---- */
    APP_Servo_Init();

    /* ---- 注册舵机 ---- */
    for (uint8_t i = 0; i < ARRAY_SIZE(SERVO_CFG); i++)
    {
        APP_Servo_Add(SERVO_CFG[i].servo_id,
                   SERVO_CFG[i].pwm_id,
                   &SERVO_CFG[i].hw,
                   SERVO_CFG[i].speed_dps);
    }

    /* ---- 周期调度：舵机平滑输出（步态调度已在 Motion_T 中独立运行） ---- */
    for (;;)
    {
        APP_Servo_Scheduler();  /* 浮点平滑 → PWM 输出 */
        osDelay(SERVO_TICK_MS);
    }
}
