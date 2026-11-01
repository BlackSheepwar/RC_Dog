/**
 * @file task_servo.c
 * @brief 舵机线程：定周期推一次运动服务
 * @author Ricardo
 * @date 2026-11-01
 *
 * @note 线程只回答三件事：多久跑一次、跑哪个函数、时间差从哪来。
 *       平滑算法在 Service/Svc_Servo_Motion，脉宽换在 Device/Dev_Servo，
 *       角度目标由 APP_Motion 给——本文件一行业务都没有。
 *
 *       elapsed_ms 用 OS 节拍差分：服务层只收参数、不看时钟，
 *       换 OS 只改这里。
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include <stdint.h>
#include "cmsis_os2.h"
// 功能包含
#include "svc_servo_motion.h"

/*==============================================================================
 * 宏定义与常量
 *============================================================================*/
/** @brief 调度周期（ms）；改了它要一并检查角速度的整定值 */
#define TASK_SERVO_PERIOD_MS    10u

/*==============================================================================
 * 线程函数
 *============================================================================*/
/**
 * @brief 舵机平滑线程（入口名由 CubeMX .ioc 决定，不能改）
 * @param argument 线程参数（未使用）
 * @retval 无
 * @note 定时器频率、舵机设备、运动服务都在 freertos.c 的 USER CODE 段里
 *       于调度前统一初始化，本线程只负责"定周期推一次"，不做初始化。
 */
void Task_Servo(void *argument)
{
    uint32_t last_tick;

    (void)argument;

    last_tick = osKernelGetTickCount();

    for (;;) {
        uint32_t now     = osKernelGetTickCount();
        uint32_t elapsed = now - last_tick;
        last_tick = now;

        (void)Svc_Servo_Motion_Tick_All(elapsed);
        osDelay(TASK_SERVO_PERIOD_MS);
    }
}
