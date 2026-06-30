/**
 * @file app_motion_scheduler.h
 * @brief 周期调度层：步态序列调度
 * @author 李嘉图
 * @date 2026-06-29
 *
 * @note 核心功能：
 *       - 管理步态序列（多个相位）
 *       - 每10ms推进相位
 *       - 调用Motion层进行角度控制
 *
 *       架构：
 *       步态序列 → MotionSched_Start
 *              → MotionSched_Scheduler (每10ms)
 *                    → Motion_SetTarget
 *                         → Motion_Scheduler (每10ms)
 *                              → APP_Servo_SetTarget
 *
 * @warning 使用前必须先调用MotionSched_Init()初始化
 */

#ifndef __APP_MOTION_SCHEDULER_H__
#define __APP_MOTION_SCHEDULER_H__

#include <stdint.h>
#include "app_motion.h"  /* 获取 MOTION_MAX_LEGS / MOTION_JOINTS_PER_LEG */

/*==============================================================================
 * 常量
 *============================================================================*/
/** @brief 单相位最大执行时间(ms)，超时后强制推进防止死锁 */
#define MOTION_PHASE_TIMEOUT_MS  1000

/*==============================================================================
 * 相位配置结构体
 *============================================================================*/
/**
 * @brief 关节角度（髋、膝）
 * @details
 *       hip:  髋关节角度 (°)
 *            正=前摆, 负=后摆
 *       knee: 膝关节角度 (°)
 *            正=弯曲, 负=过伸
 */
typedef struct {
    int16_t hip;   /* 髋关节角度(°) */
    int16_t knee;  /* 膝关节角度(°) */
} joint_angle_t;

/**
 * @brief 单个相位中一条腿的数据
 */
typedef struct {
    uint8_t leg_id;          /**< 腿编号 */
    joint_angle_t angle;    /**< 该腿的关节角度目标 */
} leg_phase_data_t;

/**
 * @brief 单个步态相位
 * @details 每条腿 + 相位统一速度打包在一起
 */
typedef struct {
    const leg_phase_data_t *legs;  /**< 该相位的腿数据数组 */
    uint8_t leg_count;             /**< 该相位控制的腿数量 */
    float speed;                   /**< 相位速度 0.1 ~ 100.0 */
} motion_phase_t;

/**
 * @brief 完整步态序列
 * @details
 *       phases:     相位数组，phase_count 项
 *       phase_count: 相位总数
 *       loop:       1=循环执行, 0=仅执行一遍
 */
typedef struct {
    const motion_phase_t *phases;  /**< 相位数组 */
    uint16_t phase_count;          /**< 相位总数 */
    uint8_t loop;                  /**< 1=循环，0=仅执行一遍 */
} motion_gait_t;

/*==============================================================================
 * 调度API
 *============================================================================*/
/**
 * @brief 初始化调度器
 * @details
 *       - 清空步态运行状态
 *       - 准备接收新的步态序列
 * @note 必须在Motion_Init之后调用
 */
void MotionSched_Init(void);

/**
 * @brief 启动一个步态序列
 * @param gait 步态数据指针
 * @details
 *       - 立即应用第一个相位（无延迟）
 *       - 标记step态为活跃(active=1)
 *       - 从第0个相位开始
 * @warning gait不能为NULL
 */
void MotionSched_Start(const motion_gait_t *gait);

/**
 * @brief 停止当前步态
 * @details
 *       - 清零active标记
 *       - 调用Motion_StopAll停止所有腿
 */
void MotionSched_Stop(void);

/**
 * @brief 周期调度器（每10ms调用一次）
 * @details
 *       流程：
 *       1. 先调用Motion_Scheduler更新腿关节到达状态
 *       2. 检查当前相位所有腿是否完成（Motion_IsMoving==0）
 *       3. 若全部完成，推进到下一相位
 *       4. 若已是最后相位：
 *          - 若loop=1，重置为第0相位
 *          - 若loop=0，标记步态完成(active=0)
 *
 * @note 必须由Task_Motion每10ms调用一次
 */
void MotionSched_Scheduler(void);

/**
 * @brief 查询是否有步态正在执行
 * @return 1=步态运行中, 0=停止或空闲
 */
uint8_t MotionSched_IsRunning(void);

#endif /* __APP_MOTION_SCHEDULER_H__ */
