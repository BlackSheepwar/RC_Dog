/**
 * @file task_se.c
 * @brief 实现舵机控制处理任务
 * @author 李嘉图
 * @date 2026-6-5
 *
 * @note 舵机注册参数和定时器频率配置已提取到 se_cfg.h，
 *       Task 层只做：初始化 → 循环遍历配置表 → 周期调度。
 *       加舵机或调参数只需改 se_cfg.h，不动 Task 代码。
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
#include "main.h"
#include "app_se.h"
#include "se_cfg.h"
#include "common.h"
#include <stdint.h>

/*==============================================================================
 * 任务函数
 *============================================================================*/
void Task_SE(void *argument)
{
    /* ---- 初始化 ---- */
    APP_SE_Init();

    /* ---- 配置定时器频率 ---- */
    for (uint8_t i = 0; i < SE_TIM_CFG_COUNT; i++)
    {
        BSP_SetFreq(SE_TIM_CFG[i].htim, SE_TIM_CFG[i].clock_hz, SE_TIM_CFG[i].freq);
    }

    /* ---- 注册舵机 ---- */
    for (uint8_t i = 0; i < SE_SERVO_COUNT; i++)
    {
        APP_SE_Add(SE_SERVO_CFG[i].id,
                   SE_SERVO_CFG[i].htim,
                   SE_SERVO_CFG[i].channel,
                   SE_SERVO_CFG[i].offset,
                   SE_SERVO_CFG[i].init_angle,
                   SE_SERVO_CFG[i].speed,
                   SE_SERVO_CFG[i].reverse);
    }

    /* ---- 周期调度 ---- */
    for (;;)
    {
        APP_SE_Scheduler();
        osDelay(10);
    }
}
