/**
 * @file pid.c
 * @brief PID 闭环控制算法实现（纯算法，无硬件依赖）
 * @author 李嘉图
 * @date 2026-07-05
 *
 * @note 所有状态由调用方通过 PID_Handle_t 传入，
 *       本层不维护任何静态存储，纯数学运算。
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
#include "pid.h"
#include <string.h>

/*==============================================================================
 * 初始化函数
 *============================================================================*/
/**
 * @brief 初始化 PID 控制器句柄
 * @param pid PID 句柄指针（非 NULL）
 * @param Kp  比例系数
 * @param Ki  积分系数
 * @param Kd  微分系数
 * @param dt  计算周期（秒）
 */
void PID_Init(PID_Handle_t *pid, float Kp, float Ki, float Kd, float dt)
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
    pid->integral  = 0.0f;
    pid->dt        = dt;
}

/*==============================================================================
 * 参数设置
 *============================================================================*/
/**
 * @brief 设置 PID 目标值
 * @param pid    PID 句柄指针
 * @param target 目标值
 */
void PID_SetTarget(PID_Handle_t *pid, float target)
{
    if (pid == NULL) return;
    pid->target = target;
}

/**
 * @brief 设置 PID 参数（Kp, Ki, Kd）
 * @param pid PID 句柄指针
 * @param Kp  比例系数
 * @param Ki  积分系数
 * @param Kd  微分系数
 */
void PID_SetParam(PID_Handle_t *pid, float Kp, float Ki, float Kd)
{
    if (pid == NULL) return;
    pid->Kp = Kp;
    pid->Ki = Ki;
    pid->Kd = Kd;
}

/**
 * @brief 重置 PID 状态（清零误差/积分/输出）
 * @param pid PID 句柄指针
 */
void PID_Reset(PID_Handle_t *pid)
{
    if (pid == NULL) return;

    pid->out      = 0.0f;
    pid->error[0] = 0.0f;
    pid->error[1] = 0.0f;
    pid->error[2] = 0.0f;
    pid->integral = 0.0f;
}

/*==============================================================================
 * PID 控制核心
 *============================================================================*/
/**
 * @brief 位置式 PID 控制
 * @param pid    PID 句柄指针
 * @param actual 当前实际值（如编码器换算速度）
 * @return PID 全量输出
 *
 * @note 公式：u(t) = Kp·e(t) + Ki·∫e(t)dt + Kd·de(t)/dt
 *       integral 累加使用 dt 加权，微分使用 dt 归一化，
 *       使 Kp/Ki/Kd 取值与调度周期解耦。
 */
float PID_PositionControl(PID_Handle_t *pid, float actual)
{
    if (pid == NULL) return 0.0f;

    pid->actual = actual;

    /* 更新误差链 */
    pid->error[2] = pid->error[1];
    pid->error[1] = pid->error[0];
    pid->error[0] = pid->target - pid->actual;

    /* 积分累加（带 dt 加权） */
    pid->integral += pid->error[0] * pid->dt;

    /* PID 全量输出 */
    pid->out = pid->Kp * pid->error[0]
             + pid->Ki * pid->integral
             + pid->Kd * (pid->error[0] - pid->error[1]) / pid->dt;

    return pid->out;
}

/**
 * @brief 增量式 PID 控制
 * @param pid    PID 句柄指针
 * @param actual 当前实际值（如编码器换算速度）
 * @return PID 输出增量（需调用方累加到全量输出）
 *
 * @note 公式：Δu = Kp·Δe + Ki·e(t)·Δt + Kd·(Δe - Δe_prev)/Δt
 *       其中 Δe = e(t) - e(t-1), Δe_prev = e(t-1) - e(t-2)
 *       返回值是增量，调用方需自行：out += delta，再做限幅。
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
                + pid->Ki * pid->error[0] * pid->dt
                + pid->Kd * (pid->error[0] - 2.0f * pid->error[1] + pid->error[2]) / pid->dt;

    /* 累加到全量输出 */
    pid->out += delta;

    return delta;
}
