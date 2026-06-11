/**
 * @file app_servo_gait.h
 * @brief 步态控制：腿的相位序列 → 舵机目标角度（两自由度：膝/踝）
 * @author 李嘉图
 * @date 2026-06-05
 *
 * @note 在单个舵机调度器之上，提供「腿」级别的步态控制。
 *       每条腿由2个舵机（膝/踝）组成，一个步态是一系列相位，
 *       每个相位定义所有腿的目标角度 + 持续时间。
 *       Gait_Scheduler() 每个 tick 调用，持续向舵机调度器发送目标。
 */

#ifndef __APP_SERVO_GAIT_H__
#define __APP_SERVO_GAIT_H__

/*==============================================================================
 * 头文件包含
 *============================================================================*/
#include "common.h"

/*==============================================================================
 * 常量定义
 *============================================================================*/
#define SERVO_PER_LEG    2    /* 机械肢自由度数量*/
#define LEG_COUNT        2    /* 每块MCU控制的腿数 */

/*==============================================================================
 * 类型定义
 *============================================================================*/
/**
 * @brief 机械肢姿态
 */
typedef struct {
    int16_t joint[SERVO_PER_LEG];
} leg_pose_t;

/**
 * @brief 一个步态相位
 * @note 所有腿的目标姿态 + 持续 tick 数
 */
typedef struct {
    leg_pose_t legs[LEG_COUNT];
    uint16_t   duration_tick;    /* 持续 tick 数（默认10ms/tick）*/
} gait_phase_t;

/**
 * @brief 步态相位序列
 * @note phases 指向静态常量表，loop=1 时自动循环
 */
typedef struct {
    const gait_phase_t *phases;
    uint8_t             phase_count;
    uint8_t             loop;          /* 0=单次, 1=循环 */
} gait_sequence_t;

/**
 * @brief 步态执行器状态
 * @note 内部使用，记录当前步态进度
 */
typedef struct {
    const gait_sequence_t *current_gait;
    uint8_t  phase_index;          /* 当前相位 */
    uint16_t phase_tick;           /* 当前相位内进度 */
    uint8_t  active;               /* 1=正在执行步态 */
} gait_runner_t;

/*==============================================================================
 * API
 *============================================================================*/
/**
 * @brief 初始化步态控制器
 */
void Gait_Init(void);

/**
 * @brief 开始执行步态
 * @param gait 步态序列指针（指向静态常量，不拷贝）
 */
void Gait_Start(const gait_sequence_t *gait);

/**
 * @brief 停止步态
 */
void Gait_Stop(void);

/**
 * @brief 查询是否正在执行步态
 * @retval 1 运行中
 * @retval 0 空闲
 */
uint8_t Gait_IsRunning(void);

/**
 * @brief 步态调度器（每 tick 调用）
 * @note 设置舵机目标角度，推进相位，处理循环/结束。
 *       在 task_servo 中先调 APP_Servo_Scheduler() 再调此函数。
 */
void Gait_Scheduler(void);

/*==============================================================================
 * 内置步态序列（在 app_servo_gait.c 中定义）
 *============================================================================*/
extern const gait_sequence_t GAIT_STAND;
extern const gait_sequence_t GAIT_SIT;

#endif /* __APP_SERVO_GAIT_H__ */
