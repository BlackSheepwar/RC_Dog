/**
 * @file task_motion.c
 * @brief 步态线程：上电先趴伏，之后定周期推进相位
 * @author Ricardo
 * @date 2026-11-01
 *
 * @note 两层结构：
 *       APP_Motion_Sched_Scheduler() —— 判到位、推进相位（本线程调用）
 *         └─ APP_Motion_Scheduler()  —— 读舵机位置、更新腿状态
 *              └─ Svc_Servo_Motion_Tick() —— 真正输出脉宽（舵机线程调用）
 *
 *       舵机链与运动层都在 freertos.c 的 USER CODE 段里于调度前初始化
 *       （运动层依赖"舵机当前角"当基线，必须排在舵机设备之后），
 *       本线程只做"上电姿态 + 周期推进"。
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include <stdint.h>
#include "cmsis_os2.h"
// 功能包含
#include "app_motion.h"
#include "app_motion_scheduler.h"
#include "app_gait_seq.h"       /* GAIT_Grovel */

/*==============================================================================
 * 宏定义与常量
 *============================================================================*/
/** @brief 相位推进周期（ms），与舵机周期一致 */
#define TASK_MOTION_PERIOD_MS   10u

/*==============================================================================
 * 线程函数
 *============================================================================*/
/**
 * @brief 步态线程（入口名由 CubeMX .ioc 决定，不能改）
 * @param argument 线程参数（未使用）
 * @retval 无
 * @note 上电先执行"趴伏"：四腿收到最稳妥的姿态，再接受命令。
 *       这个初始姿态写在 TASK 而不是 APP，因为它是启动策略而非业务规则。
 */
void Task_Motion(void *argument)
{
    (void)argument;

    /* 给舵机线程一点时间完成首轮输出，再开始推进相位 */
    osDelay(TASK_MOTION_PERIOD_MS);

    APP_Motion_Sched_Start(&GAIT_Grovel);

    for (;;) {
        APP_Motion_Sched_Scheduler();
        osDelay(TASK_MOTION_PERIOD_MS);
    }
}
