/**
 * @file app_motion_scheduler.h
 * @brief 步态调度层：相位推进（到位 → 下一相位）
 * @author Ricardo
 * @date 2026-06-29
 *
 * @note 核心功能：
 *       - 管理步态序列（多个相位）
 *       - 每10ms推进相位
 *       - 调用Motion层进行角度控制
 *
 *       架构：
 *       步态序列 → APP_Motion_Sched_Start
 *              → APP_Motion_Sched_Scheduler (每10ms)
 *                    → APP_Motion_Set_Target
 *                         → APP_Motion_Scheduler (每10ms)
 *                              → Svc_Servo_Motion_Set_Target
 *
 * @warning 使用前必须先调用 APP_Motion_Sched_Init() 初始化
 */

#ifndef __APP_MOTION_SCHEDULER_H__
#define __APP_MOTION_SCHEDULER_H__

#include <stdint.h>
#include "app_motion.h"  /* 获取 APP_MOTION_MAX_LEGS / APP_MOTION_JOINTS_PER_LEG */

/*==============================================================================
 * 常量
 *============================================================================*/
/** @brief 单相位最大执行时间(ms)，超时后强制推进防止死锁 */
#define APP_MOTION_PHASE_TIMEOUT_MS  3000

/**
 * @brief 相位的控制方式
 * @note 角度直通用于"我知道每个关节要多少度"，坐标控制用于"我要足端到某个点"；
 *       后者走 Common_Ik 解算，杆长取该腿配置表里的值。
 */
#define APP_MOTION_MODE_ANGLE        0u   /* 直接用 leg->angle（度） */
#define APP_MOTION_MODE_POS          1u   /* 用 leg->pos（足端坐标）走逆解 */

/**
 * @brief 逆解取哪一组
 * @note 0 = 膝向后（θ2 ≥ 0），1 = 膝向前（θ2 ≤ 0）。
 *       本机装配取膝向前；若某天腿"朝反方向弯"，改这一个数字即可，
 *       不要去动 Common/Common_Ik 的公式。
 */
#define APP_MOTION_IK_SOLUTION       1u

/*==============================================================================
 * 相位配置结构体
 *
 * 这四个类型都是**值类型**：步态表在 app_gait_seq.h 里按值写死，
 * 编译器要算 sizeof 才能构造数组，所以布局必须写在头文件里。
 * 它们只描述"要走到哪"，不含任何运行时状态，改布局只影响配置表。
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
    float hip;   /**< 髋关节角度(°) */
    float knee;  /**< 膝关节角度(°) */
} joint_angle_t;

/**
 * @brief 二连杆足端坐标(mm)
 * @details
 *       x: 足端 x 坐标（+x 前）
 *       y: 足端 y 坐标（+y 下）
 * @note 仅当 gait->mode = APP_MOTION_MODE_POS 时使用
 */
typedef struct {
    float x;  /**< 足端x坐标(mm) */
    float y;  /**< 足端y坐标(mm) */
} foot_pos_t;

/**
 * @brief 单个相位中一条腿的数据
 * @details
 *       angle: 关节角度目标，APP_MOTION_MODE_ANGLE 时使用
 *       pos:   足端坐标，APP_MOTION_MODE_POS 时使用
 */
typedef struct {
    uint8_t leg_id;          /**< 腿编号 */
    joint_angle_t angle;    /**< 关节角度目标(°)，MODE_ANGLE 时使用 */
    foot_pos_t pos;         /**< 足端坐标(mm)，MODE_POS 时使用；y = 向下伸展距离 */
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
 *       mode:       取 APP_MOTION_MODE_ANGLE / APP_MOTION_MODE_POS
 */
typedef struct {
    const motion_phase_t *phases;  /**< 相位数组 */
    uint16_t phase_count;          /**< 相位总数 */
    uint8_t loop;                  /**< 1=循环，0=仅执行一遍 */
    uint8_t mode;                  /**< 控制方式，取 APP_MOTION_MODE_xxx */
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
void APP_Motion_Sched_Init(void);

/**
 * @brief 启动一个步态序列
 * @param gait 步态数据指针
 * @details
 *       - 立即应用第一个相位（无延迟）
 *       - 标记step态为活跃(active=1)
 *       - 从第0个相位开始
 * @warning gait不能为NULL
 */
void APP_Motion_Sched_Start(const motion_gait_t *gait);

/**
 * @brief 停止当前步态
 * @details
 *       - 清零active标记
 *       - 调用Motion_StopAll停止所有腿
 */
void APP_Motion_Sched_Stop(void);

/**
 * @brief 周期调度器（每10ms调用一次）
 * @details
 *       流程：
 *       1. 先调用Motion_Scheduler更新腿关节到达状态
 *       2. 检查当前相位所有腿是否完成（APP_Motion_Is_Moving==0）
 *       3. 若全部完成，推进到下一相位
 *       4. 若已是最后相位：
 *          - 若loop=1，重置为第0相位
 *          - 若loop=0，标记步态完成(active=0)
 *
 * @note 必须由Task_Motion每10ms调用一次
 */
void APP_Motion_Sched_Scheduler(void);

/**
 * @brief 查询是否有步态正在执行
 * @retval 1: 步态运行中
 * @retval 0: 停止或空闲
 */
uint8_t APP_Motion_Sched_Is_Running(void);

#endif /* __APP_MOTION_SCHEDULER_H__ */
