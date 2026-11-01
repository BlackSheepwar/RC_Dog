/**
 * @file pid.c
 * @brief 增量式 PID 闭环控制算法实现（纯算法，无硬件依赖）
 * @author 李嘉图
 * @date 2026-07-05
 *
 * @note 所有状态由调用方通过 PID_Handle_t 传入，
 *       本层不维护任何静态存储，纯数学运算。
 *       仅保留增量式 PID（参考 Mini 4WD RC 实现）。
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include "pid.h"
#include <string.h>

/*==============================================================================
 * 初始化函数
 *============================================================================*/
void PID_Init(PID_Handle_t *pid, float Kp, float Ki, float Kd,
              float out_max, float out_min)
{
    if (pid == NULL) return;

    pid->target   = 0.0f;
    pid->actual   = 0.0f;
    pid->out      = 0.0f;

    pid->Kp = Kp;
    pid->Ki = Ki;
    pid->Kd = Kd;

    pid->error[0]  = 0.0f;
    pid->error[1]  = 0.0f;
    pid->error[2]  = 0.0f;
    pid->out_max   = out_max;
    pid->out_min   = out_min;
}

/*==============================================================================
 * 参数设置
 *============================================================================*/
void PID_SetTarget(PID_Handle_t *pid, float target)
{
    if (pid == NULL) return;
    pid->target = target;
}

void PID_SetParam(PID_Handle_t *pid, float Kp, float Ki, float Kd)
{
    if (pid == NULL) return;
    pid->Kp = Kp;
    pid->Ki = Ki;
    pid->Kd = Kd;
}

void PID_Reset(PID_Handle_t *pid)
{
    if (pid == NULL) return;

    pid->out      = 0.0f;
    pid->error[0] = 0.0f;
    pid->error[1] = 0.0f;
    pid->error[2] = 0.0f;
}

/*==============================================================================
 * PID 控制核心
 *============================================================================*/
/**
 * @brief 增量式 PID 控制
 * @param pid    PID 句柄指针
 * @param actual 当前实际值（如编码器换算速度）
 * @return PID 输出增量
 *
 * @note Δu = Kp·Δe + Ki·e + Kd·(Δe − Δe_prev)
 *       其中 Δe = e(t) − e(t−1), Δe_prev = e(t−1) − e(t−2)
 *       Ki/Kd 项不带 dt（与 Mini 4WD RC 一致）
 *       返回值是增量，内部自动累加到 pid->out 并限幅
 */
float PID_IncrementControl(PID_Handle_t *pid, float actual)
{
    if (pid == NULL) return 0.0f;

    pid->actual = actual;

    /* 更新误差链 */
    pid->error[2] = pid->error[1];
    pid->error[1] = pid->error[0];
    pid->error[0] = pid->target - pid->actual;

    /* 增量计算 */
    float delta = pid->Kp * (pid->error[0] - pid->error[1])
                + pid->Ki * pid->error[0]
                + pid->Kd * (pid->error[0] - 2.0f * pid->error[1] + pid->error[2]);

    /* 累加到全量输出（带输出限幅抗饱和） */
    pid->out += delta;
    if (pid->out > pid->out_max)  pid->out = pid->out_max;
    if (pid->out < pid->out_min)  pid->out = pid->out_min;

    return delta;
}
