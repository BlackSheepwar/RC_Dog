/**
 * @file common_pid.c
 * @brief 通用 PID 算法实现：位置式 + 增量式
 * @author Ricardo
 * @date 2026-11-01
 *
 * @note 全部状态都存在调用方给的 common_pid_t 里，本文件没有 static 变量，
 *       同一个函数可以被任意多个控制回路并发调用（互不干扰）。
 *       只用到加减乘除与浮点比较，不依赖 libm，也不看时间。
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include <stddef.h>
#include "common_pid.h"

/*==============================================================================
 * 内部函数
 *============================================================================*/
/**
 * @brief 把 value 限幅到 [lo, hi]
 * @param value 待限幅的值
 * @param lo    下限
 * @param hi    上限（小于 lo 时按 lo 处理）
 * @retval 限幅后的值
 */
static float Common_Pid_Clamp(float value, float lo, float hi)
{
    if (hi < lo) {
        hi = lo;
    }
    if (value < lo) {
        return lo;
    }
    if (value > hi) {
        return hi;
    }
    return value;
}

/*==============================================================================
 * 初始化与参数
 *============================================================================*/
/**
 * @brief 初始化 PID 上下文
 * @param pid PID 上下文，不可为 NULL
 * @param cfg 配置参数，不可为 NULL；上下限倒置时自动交换
 * @retval 0: 初始化成功
 * @retval -1: pid 或 cfg 为 NULL
 */
int Common_Pid_Init(common_pid_t *pid, const common_pid_cfg_t *cfg)
{
    if ((pid == NULL) || (cfg == NULL)) {
        return -1;
    }

    pid->kp = cfg->kp;
    pid->ki = cfg->ki;
    pid->kd = cfg->kd;

    /* 上下限倒置是配置错误，这里直接交换而不是报错：
     * 现场改参数时最常见的笔误是 min/max 写反，交换后行为可预期 */
    pid->out_min      = (cfg->out_min < cfg->out_max) ? cfg->out_min : cfg->out_max;
    pid->out_max      = (cfg->out_min < cfg->out_max) ? cfg->out_max : cfg->out_min;
    pid->integral_min = (cfg->integral_min < cfg->integral_max)
                        ? cfg->integral_min : cfg->integral_max;
    pid->integral_max = (cfg->integral_min < cfg->integral_max)
                        ? cfg->integral_max : cfg->integral_min;

    Common_Pid_Reset(pid);
    return 0;
}

/**
 * @brief 清零运行时状态，保留整定参数
 * @param pid PID 上下文，可为 NULL（安全返回）
 * @retval 无
 */
void Common_Pid_Reset(common_pid_t *pid)
{
    if (pid == NULL) {
        return;
    }

    pid->target   = 0.0f;
    pid->actual   = 0.0f;
    pid->out      = 0.0f;
    pid->integral = 0.0f;
    pid->error[0] = 0.0f;
    pid->error[1] = 0.0f;
    pid->error[2] = 0.0f;
}

/**
 * @brief 设置目标值
 * @param pid    PID 上下文，可为 NULL（安全返回）
 * @param target 目标值
 * @retval 无
 */
void Common_Pid_Set_Target(common_pid_t *pid, float target)
{
    if (pid == NULL) {
        return;
    }
    pid->target = target;
}

/**
 * @brief 只改三个增益，不动输出限幅
 * @param pid PID 上下文，可为 NULL（安全返回）
 * @param kp  比例系数
 * @param ki  积分系数
 * @param kd  微分系数
 * @retval 无
 */
void Common_Pid_Set_Gain(common_pid_t *pid, float kp, float ki, float kd)
{
    if (pid == NULL) {
        return;
    }
    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;
}

/*==============================================================================
 * 控制核心
 *============================================================================*/
/**
 * @brief 位置式 PID：输出控制量绝对值
 * @param pid    PID 上下文，不可为 NULL
 * @param actual 本次实际值
 * @param dt_s   距上次调用的时间（秒），必须 > 0；传 0 或负值按 1 ms 处理
 * @retval 本次输出值，已限幅到 [out_min, out_max]
 * @note 分四步：更新误差 → 积分累加并限幅 → 微分 → 求和与输出限幅。
 *       dt 非法时用 1 ms 兜底，避免除零把输出打成 inf。
 */
float Common_Pid_Update(common_pid_t *pid, float actual, float dt_s)
{
    float p_term;
    float d_term;

    if (pid == NULL) {
        return 0.0f;
    }

    if (dt_s <= 0.0f) {
        dt_s = 0.001f;
    }

    /* ---------- 1. 误差 ---------- */
    pid->actual   = actual;
    pid->error[2] = pid->error[1];
    pid->error[1] = pid->error[0];
    pid->error[0] = pid->target - actual;

    /* ---------- 2. 积分累加与积分限幅 ---------- */
    pid->integral += pid->ki * pid->error[0] * dt_s;
    pid->integral = Common_Pid_Clamp(pid->integral,
                                     pid->integral_min, pid->integral_max);

    /* ---------- 3. 比例与微分 ---------- */
    p_term = pid->kp * pid->error[0];
    d_term = pid->kd * (pid->error[0] - pid->error[1]) / dt_s;

    /* ---------- 4. 求和与输出限幅 ---------- */
    pid->out = Common_Pid_Clamp(p_term + pid->integral + d_term,
                                pid->out_min, pid->out_max);
    return pid->out;
}

/**
 * @brief 增量式 PID：输出本次增量，内部累加全量输出
 * @param pid    PID 上下文，不可为 NULL
 * @param actual 本次实际值
 * @retval 本次输出增量 Δu
 * @note Δu = Kp·Δe + Ki·e + Kd·(Δe − Δe_prev)，三项都不带 dt，
 *       与位置式在 dt=1 时等价；速度环沿用这套整定习惯。
 */
float Common_Pid_Update_Increment(common_pid_t *pid, float actual)
{
    float delta;

    if (pid == NULL) {
        return 0.0f;
    }

    /* ---------- 1. 误差链 ---------- */
    pid->actual   = actual;
    pid->error[2] = pid->error[1];
    pid->error[1] = pid->error[0];
    pid->error[0] = pid->target - actual;

    /* ---------- 2. 增量计算 ---------- */
    delta = pid->kp * (pid->error[0] - pid->error[1])
          + pid->ki * pid->error[0]
          + pid->kd * (pid->error[0] - 2.0f * pid->error[1] + pid->error[2]);

    /* ---------- 3. 累加全量输出并限幅（抗饱和） ---------- */
    pid->integral = Common_Pid_Clamp(pid->integral + delta,
                                     pid->out_min, pid->out_max);
    pid->out = pid->integral;

    return delta;
}

/*==============================================================================
 * 状态查询（只读）
 *============================================================================*/
/**
 * @brief 取最近一次输出值
 * @param pid PID 上下文，可为 NULL
 * @retval 输出值；pid 为 NULL 时返回 0
 */
float Common_Pid_Get_Out(const common_pid_t *pid)
{
    return (pid != NULL) ? pid->out : 0.0f;
}

/**
 * @brief 取当前目标值
 * @param pid PID 上下文，可为 NULL
 * @retval 目标值；pid 为 NULL 时返回 0
 */
float Common_Pid_Get_Target(const common_pid_t *pid)
{
    return (pid != NULL) ? pid->target : 0.0f;
}

/**
 * @brief 取最近一次实际值
 * @param pid PID 上下文，可为 NULL
 * @retval 实际值；pid 为 NULL 时返回 0
 */
float Common_Pid_Get_Actual(const common_pid_t *pid)
{
    return (pid != NULL) ? pid->actual : 0.0f;
}

/**
 * @brief 取最近一次偏差 e = target − actual
 * @param pid PID 上下文，可为 NULL
 * @retval 偏差；pid 为 NULL 时返回 0
 */
float Common_Pid_Get_Error(const common_pid_t *pid)
{
    return (pid != NULL) ? pid->error[0] : 0.0f;
}
