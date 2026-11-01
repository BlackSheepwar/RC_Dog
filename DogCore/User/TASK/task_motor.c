/**
 * @file task_motor.c
 * @brief 电机线程：定周期跑一次速度环
 * @author Ricardo
 * @date 2026-11-01
 *
 * @note 闭环算法在 Service/Svc_Motor_Speed，PID 在 Common/Common_Pid，
 *       占空比输出在 Device/Dev_Motor；本线程只提供"节拍 + 时间差"。
 *
 *       周期必须与整定时使用的采样周期一致：速度换算把 elapsed_ms 当分母，
 *       周期变了而整定没重做，实测速度会整体缩放（表现为"同样的目标值跑得不一样快"）。
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include <stdint.h>
#include "cmsis_os2.h"
// 功能包含
#include "svc_motor_speed.h"

/*==============================================================================
 * 宏定义与常量
 *============================================================================*/
/** @brief 闭环周期（ms） */
#define TASK_MOTOR_PERIOD_MS    10u

/*==============================================================================
 * 线程函数
 *============================================================================*/
/**
 * @brief 电机速度环线程（入口名由 CubeMX .ioc 决定，不能改）
 * @param argument 线程参数（未使用）
 * @retval 无
 * @note 定时器频率、电机设备、速度环服务都在 freertos.c 的 USER CODE 段里
 *       于调度前统一初始化，本线程只负责"定周期推一次"，不做初始化。
 */
void Task_Motor(void *argument)
{
    uint32_t last_tick;

    (void)argument;

    last_tick = osKernelGetTickCount();

    for (;;) {
        uint32_t now     = osKernelGetTickCount();
        uint32_t elapsed = now - last_tick;
        last_tick = now;

        Svc_Motor_Speed_Tick_All(elapsed);
        osDelay(TASK_MOTOR_PERIOD_MS);
    }
}
