/**
 * @file task_motor.c
 * @brief 电机 PID 控制任务（FreeRTOS）
 * @author 李嘉图
 * @date 2026-07-05
 *
 * @note 此文件提供供 RTOS 任务调用的函数入口。
 *       实际的 FreeRTOS 任务创建在 CubeMX 中完成（见配置说明），
 *       本文件只包含被任务调用的执行函数。
 *
 *       调度流程：
 *         CubeMX 创建的 RTOS 任务
 *           → 每 MOTOR_PID_TICK_MS(10ms) 调用一次
 *             → APP_Motor_RunPID() 完成全部 4 个电机的闭环控制
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
 // 固定包含
#include "main.h"
// 功能包含
#include "app_motor.h"
#include "app_motor_debug.h"

/*==============================================================================
 * 电机 PID 任务执行函数
 *============================================================================*/
/**
 * @brief 电机 PID 控制任务（由 CubeMX 创建的 RTOS 任务调用）
 * @param param 任务参数（本函数不使用，传 NULL 即可）
 */
void Task_Motor(void *param)
{
    APP_Motor_Init();

    for (;;)
    {
        APP_Motor_RunPID();
        osDelay(MOTOR_PID_TICK_MS);
    }
}
