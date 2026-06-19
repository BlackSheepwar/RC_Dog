/**
 * @file task_servo.c
 * @brief 舵机控制处理任务（顶层调度入口）
 * @author 李嘉图
 * @date 2026-06-05
 *
 * @note 舵机注册参数已提取到 app_servo_cfg.h，
 *       定时器频率配置已下沉到 bsp_pwm.c 的 PWM_TIM_CFG 表，
 *       Task 层只做：初始化 → 周期调度，加舵机调参数只需改 cfg 头文件。
 *
 *       运动控制链路（10ms 周期）：
 *       Gait_IK_Scheduler() → 步态相位推进 + IK 解算
 *         → Limb_SetTarget() → 肢体目标 + 过渡时间
 *       Limb_Scheduler()    → 时间基线性插值
 *         → APP_Servo_SetTarget() → 设置舵机目标角度
 *       APP_Servo_Scheduler() → 浮点平滑 + PWM 输出
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
#include "app_gait.h"

/*==============================================================================
 * 任务函数
 *
 * 初始化顺序（固定）：
 *   1. APP_Servo_Init()        — 清空舵机池，初始化 PWM 底层
 *   2. BSP_PWM_SetAllFreq()    — 按 BSP 内置表配置所有定时器频率
 *   3. APP_Servo_Add(...)      — 注册所有舵机实例
 *   4. Gait_IK_Init()          — 初始化 Limb 层 + IK 步态状态
 *      （Gait_IK_Init 内部调用 Limb_Init，自动设定舵机高速跟随）
 *
 * 启动步态（在外部通过 CAN/UART/Key 命令调用）：
 *   Gait_IK_Start(&GAIT_IK_STAND) — 站立
 *   Gait_IK_Start(&GAIT_IK_WALK)  — 行走
 *   Gait_IK_Stop()                — 停止
 *============================================================================*/
void Task_SERVO_T(void *argument)
{
    /* ---- 初始化 ---- */
    APP_Servo_Init();

    /* ---- 注册舵机 ---- */
    for (uint8_t i = 0; i < ARRAY_SIZE(SERVO_CFG); i++)
    {
        APP_Servo_Add(SERVO_CFG[i].id,
                   &SERVO_CFG[i].hw,
                   SERVO_CFG[i].speed_dps);
    }

    /* ---- 初始化 IK 步态（需在舵机注册后，内部调用 Limb_Init 设定舵机高速） ---- */
    //Gait_IK_Init();

    /* ---- 周期调度：Gait → Limb → Servo ---- */
    for (;;)
    {
        Gait_IK_Scheduler();    /* 步态相位推进 + IK 解算 → Limb_SetTarget */
        Limb_Scheduler();       /* 时间基插值 → APP_Servo_SetTarget */
        APP_Servo_Scheduler();  /* 浮点平滑 → PWM 输出 */
        osDelay(SERVO_TICK_MS);
    }
}
