/**
 * @file pid.h
 * @brief PID 闭环控制算法（纯算法，无硬件依赖）
 * @author 李嘉图
 * @date 2026-07-05
 *
 * @note 本层只做数学运算，不涉及任何硬件或平台相关内容。
 *       状态由调用方维护（传入 PID_Handle_t 指针），
 *       可在任意平台复用（STM32、ESP32、PC仿真等）。
 *       支持位置式 PID 和增量式 PID 两种模式。
 */

#ifndef __PID_H__
#define __PID_H__

/*==============================================================================
 * 头文件包含
 *============================================================================*/
#include <stdint.h>

/*==============================================================================
 * PID 控制器句柄
 *
 * 调用方分配此结构体实例并传入各函数指针。
 * 所有状态（误差历史、积分累加、输出值）均存放在此结构体中，
 * 本层不维护任何静态/全局变量。
 *============================================================================*/
typedef struct {
    float target;           /* 目标值                                */
    float actual;           /* 实际值（由调用方每次调度前更新）       */
    float out;              /* PID 输出值                             */

    float Kp;               /* 比例系数                              */
    float Ki;               /* 积分系数                              */
    float Kd;               /* 微分系数                              */

    float error[3];         /* 误差历史 [0]=本次, [1]=上次, [2]=上上次 */
    float integral;         /* 误差积分累加                          */
    float dt;               /* PID 计算周期（秒）                    */
} PID_Handle_t;

/*==============================================================================
 * 初始化函数
 *============================================================================*/
/**
 * @brief 初始化 PID 控制器句柄
 * @param pid PID 句柄指针
 * @param Kp  比例系数
 * @param Ki  积分系数
 * @param Kd  微分系数
 * @param dt  计算周期（秒）
 */
void PID_Init(PID_Handle_t *pid, float Kp, float Ki, float Kd, float dt);

/*==============================================================================
 * 参数设置
 *============================================================================*/
/**
 * @brief 设置 PID 目标值
 * @param pid    PID 句柄指针
 * @param target 目标值
 */
void PID_SetTarget(PID_Handle_t *pid, float target);

/**
 * @brief 设置 PID 参数
 * @param pid PID 句柄指针
 * @param Kp  比例系数
 * @param Ki  积分系数
 * @param Kd  微分系数
 */
void PID_SetParam(PID_Handle_t *pid, float Kp, float Ki, float Kd);

/**
 * @brief 重置 PID 状态（清零误差/积分/输出）
 * @param pid PID 句柄指针
 * @note 切换目标值或电机启动/停止时调用，防止积分饱和
 */
void PID_Reset(PID_Handle_t *pid);

/*==============================================================================
 * PID 控制核心
 *============================================================================*/
/**
 * @brief 位置式 PID 控制
 * @param pid    PID 句柄指针
 * @param actual 当前实际值（如编码器速度）
 * @return PID 输出值
 * @note 输出 = Kp·e(t) + Ki·∫e(t)dt + Kd·de(t)/dt
 *       输出范围不在此限幅，由调用方处理
 */
float PID_PositionControl(PID_Handle_t *pid, float actual);

/**
 * @brief 增量式 PID 控制
 * @param pid    PID 句柄指针
 * @param actual 当前实际值（如编码器速度）
 * @return PID 输出增量
 * @note Δu = Kp·Δe + Ki·e·Δt + Kd·(Δe(t) - Δe(t-1))/Δt
 *       返回增量值，调用方自行累加到全量输出并限幅
 */
float PID_IncrementControl(PID_Handle_t *pid, float actual);

#endif /* __PID_H__ */
