/**
 * @file task_motion.c
 * @brief 步态调度任务 — 新架构（两层分离）
 * @author 李嘉图
 * @date 2026-06-29
 *
 * @note 新架构（简化版，无IK）：
 *       Layer 1: Motion (app_motion.c)        - 角度插值 + 同步
 *       Layer 2: Scheduler (app_motion_scheduler.c) - 周期调度
 *
 *       角度输入格式：符合二连杆模型的关节角度
 *       后续扩展时可加入IK层做坐标→角度转换
 *
 *       初始化顺序：Motion → Scheduler
 *       周期调用：Scheduler(10ms) → 内部调用Motion
 */

#include <stdint.h>
#include "main.h"
#include "common.h"
#include "app_motion.h"
#include "app_motion_scheduler.h"

void Task_Motion(void *argument)
{
    /* ── 初始化两层架构 ── */
    Motion_Init();           /* 第一层：角度控制 */
    MotionSched_Init();      /* 第二层：周期调度 */

    /* ── 周期调度（10ms） ── */
    for (;;)
    {
        MotionSched_Scheduler();  /* 推进步态相位 + 调用Motion_Scheduler */
        osDelay(10);
    }
}
