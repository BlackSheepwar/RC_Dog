/**
 * @file common_pid.h
 * @brief 通用 PID 算法（纯软件层）：位置式 + 增量式，双精度限幅抗饱和
 * @author Ricardo
 * @date 2026-11-01
 *
 * @note 本模块是全工程唯一的 PID 实现，属于 Common 横向层：
 *       不碰 HAL、不碰 OS、不看时钟、不维护任何全局状态——
 *       一份控制回路一份上下文，谁用谁自己持有，可在 PC 上直接单测。
 *
 *       为什么不透明化：PID 上下文是典型的"值类型"——每路控制回路各一份，
 *       上层要按值放进结构体、要算 sizeof 才能开数组（4 个电机就是
 *       4 份），这正是本规范给值类型留的例外。因此布局写在头文件，
 *       但**状态一律通过 Get 系列只读函数访问**，上层不要直接读成员：
 *       以后加"积分分离""前馈"这类字段时，只在 .c 里改，上层不动。
 *
 *       两种算法语义不同，不要混用：
 *       - 位置式 Common_Pid_Update()：带 dt，输出是控制量绝对值（舵机角度、
 *         阀门开度这类"直接给多少"的回路）；
 *       - 增量式 Common_Pid_Update_Increment()：不带 dt，输出是相对上次的
 *         增量并内部累加（速度环这类"在上次基础上加一点"的回路）。
 */

#ifndef __Common_Pid_H__
#define __Common_Pid_H__

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include <stdint.h>

/*==============================================================================
 * 类型定义
 *============================================================================*/
/**
 * @brief PID 配置（一次性写全，避免 Init 参数列表过长）
 */
typedef struct {
    float kp;            /* 比例系数 */
    float ki;            /* 积分系数 */
    float kd;            /* 微分系数，置 0 即关闭 */
    float out_min;       /* 输出下限，抗饱和 */
    float out_max;       /* 输出上限，抗饱和 */
    float integral_min;  /* 积分项下限（仅位置式使用） */
    float integral_max;  /* 积分项上限（仅位置式使用） */
} common_pid_cfg_t;

/**
 * @brief PID 上下文（布局在此，但不许上层直接访问成员）
 * @note 状态字段对外只读，读写一律走 Common_Pid_Get_xxx /
 *       Common_Pid_Set_xxx 与 Common_Pid_Update。
 */
typedef struct {
    float    kp;            /* 比例系数 */
    float    ki;            /* 积分系数 */
    float    kd;            /* 微分系数 */
    float    out_min;       /* 输出下限 */
    float    out_max;       /* 输出上限 */
    float    integral_min;  /* 积分项下限 */
    float    integral_max;  /* 积分项上限 */
    float    target;        /* 目标值 */
    float    actual;        /* 最近一次的实际值 */
    float    out;           /* 最近一次的输出值 */
    float    integral;      /* 位置式积分累加值 */
    float    error[3];      /* 误差历史：[0] 本次，[1] 上次，[2] 上上次 */
} common_pid_t;

/*==============================================================================
 * 初始化与参数
 *============================================================================*/
/**
 * @brief 初始化 PID 上下文
 * @param pid PID 上下文，不可为 NULL
 * @param cfg 配置参数，不可为 NULL；上下限倒置时自动交换
 * @retval 0: 初始化成功
 * @retval -1: pid 或 cfg 为 NULL
 * @note 会清零全部状态（目标、误差历史、积分、输出），可当作复位使用。
 */
int Common_Pid_Init(common_pid_t *pid, const common_pid_cfg_t *cfg);

/**
 * @brief 清零运行时状态，保留整定参数
 * @param pid PID 上下文，可为 NULL（安全返回）
 * @retval 无
 * @note 目标值一并清零；重新起步前若需要目标，调用方要重新 Set_Target。
 */
void Common_Pid_Reset(common_pid_t *pid);

/**
 * @brief 设置目标值
 * @param pid    PID 上下文，可为 NULL（安全返回）
 * @param target 目标值，单位由调用方约定
 * @retval 无
 */
void Common_Pid_Set_Target(common_pid_t *pid, float target);

/**
 * @brief 只改三个增益，不动输出限幅
 * @param pid PID 上下文，可为 NULL（安全返回）
 * @param kp  比例系数
 * @param ki  积分系数
 * @param kd  微分系数
 * @retval 无
 * @note 在线调参用；改增益不清积分，避免调参瞬间输出跳变。
 */
void Common_Pid_Set_Gain(common_pid_t *pid, float kp, float ki, float kd);

/*==============================================================================
 * 控制核心
 *============================================================================*/
/**
 * @brief 位置式 PID：输出控制量绝对值
 * @param pid    PID 上下文，不可为 NULL
 * @param actual 本次实际值
 * @param dt_s   距上次调用的时间（秒），必须 > 0；传 0 或负值按 1 ms 处理
 * @retval 本次输出值，已限幅到 [out_min, out_max]
 * @note u = Kp·e + ∫Ki·e·dt + Kd·de/dt
 *       积分项单独限幅（integral_min/max），防止长时间偏差把积分灌满；
 *       输出限幅后再回写积分，构成条件积分抗饱和。
 */
float Common_Pid_Update(common_pid_t *pid, float actual, float dt_s);

/**
 * @brief 增量式 PID：输出本次增量，内部累加全量输出
 * @param pid    PID 上下文，不可为 NULL
 * @param actual 本次实际值
 * @retval 本次输出增量 Δu
 * @note Δu = Kp·Δe + Ki·e + Kd·(Δe − Δe_prev)
 *       Ki/Kd 项不带 dt：这是速度环的现场整定习惯（Ki 与 Kp 同量级即可），
 *       换成位置式会因 dt=0.01 让积分项缩小 100 倍，原有参数全部失效。
 *       全量输出累加后限幅，调用方用 Common_Pid_Get_Out() 取。
 */
float Common_Pid_Update_Increment(common_pid_t *pid, float actual);

/*==============================================================================
 * 状态查询（只读）
 *============================================================================*/
/**
 * @brief 取最近一次输出值
 * @param pid PID 上下文，可为 NULL
 * @retval 输出值；pid 为 NULL 时返回 0
 */
float Common_Pid_Get_Out(const common_pid_t *pid);

/**
 * @brief 取当前目标值
 * @param pid PID 上下文，可为 NULL
 * @retval 目标值；pid 为 NULL 时返回 0
 */
float Common_Pid_Get_Target(const common_pid_t *pid);

/**
 * @brief 取最近一次实际值
 * @param pid PID 上下文，可为 NULL
 * @retval 实际值；pid 为 NULL 时返回 0
 */
float Common_Pid_Get_Actual(const common_pid_t *pid);

/**
 * @brief 取最近一次偏差 e = target − actual
 * @param pid PID 上下文，可为 NULL
 * @retval 偏差；pid 为 NULL 时返回 0
 */
float Common_Pid_Get_Error(const common_pid_t *pid);

#endif /* __Common_Pid_H__ */
