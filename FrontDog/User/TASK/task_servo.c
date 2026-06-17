/**
 * @file task_servo.c
 * @brief 实现舵机控制处理任务
 * @author 李嘉图
 * @date 2026-06-05
 *
 * @note 舵机注册参数和定时器频率配置已提取到 app_servo_cfg.h，
 *       Task 层只做：初始化 → 循环遍历配置表 → 周期调度。
 *       加舵机或调参数只需改 app_servo_cfg.h，不动 Task 代码。
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
#include "main.h"
#include "app_servo.h"
#include "app_servo_cfg.h"
#include "app_gait.h"
#include "common.h"
#include <stdint.h>

/*==============================================================================
 * 任务函数
 *============================================================================*/
void Task_SERVO_T(void *argument)
{
    /* ---- 初始化 ---- */
    APP_Servo_Init();

    /* ---- 配置定时器频率 ---- */
    for (uint8_t i = 0; i < SERVO_TIM_CFG_COUNT; i++)
    {
        BSP_PWM_SetFreq(SERVO_TIM_CFG[i].htim, SERVO_TIM_CFG[i].clock_hz, SERVO_TIM_CFG[i].freq);
    }

    /* ---- 注册舵机 ---- */
    for (uint8_t i = 0; i < SERVO_COUNT; i++)
    {
        APP_Servo_Add(SERVO_CFG[i].id,
                   &SERVO_CFG[i].hw,
                   SERVO_CFG[i].speed_dps);
    }

    /* ---- 初始化 IK 步态（需在舵机注册后，会在 Limb 层设置舵机高速）---- */
    //Gait_IK_Init();

    /* ---- 周期调度（架构链路见 gait doc）---- */
    for (;;)
    {
        //Gait_IK_Scheduler();    /* 步态相位推进 + IK 解算 → Limb_SetTarget */
        //Limb_Scheduler();       /* 时间基插值 → APP_Servo_SetTarget */
        APP_Servo_Scheduler();  /* PWM 输出 */
        osDelay(SERVO_TICK_MS);
    }
}
