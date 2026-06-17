/**
 * @file app_gait.h
 * @brief 步态控制层：足端轨迹 → IK → 肢体调度
 * @author 李嘉图
 * @date 2026-06-11
 *
 * @note 本模块是"上层"：接收笛卡尔空间的足端轨迹步态，
 *       通过二连杆 IK 解算为关节角度，写入 Limb 层做时间基插值。
 *
 *       架构链路：
 *       Gait_IK_Scheduler()
 *         → IK_TwoLink(x, z → hip, knee)
 *         → Limb_SetTarget(关节角度, 过渡时间)
 *           → Limb_Scheduler() [每 tick 插值]
 *             → APP_Servo_SetTarget()
 *               → APP_Servo_Scheduler() [PWM 输出]
 *
 *       步态参数通过 ik_gait_sequence_t 描述，
 *       只需定义各相位足端位置 + 持续时间，无需手动标定关节角度。
 */

#ifndef __APP_GAIT_H__
#define __APP_GAIT_H__

/*==============================================================================
 * 头文件包含
 *============================================================================*/
#include "common.h"
#include "app_limb.h"
#include "ik_two_link.h"

/*==============================================================================
 * 常量定义
 *============================================================================*/
#define LEG_COUNT       2                  /* 此 MCU 控制的腿数 */

/**
 * @name 腿部几何参数
 * @note 必须根据实际机械结构测量修改！
 * @anchor LEG_L1 大腿长度（髋→膝）
 * @anchor LEG_L2 小腿长度（膝→足）
 */
#define LEG_L1  140.0f
#define LEG_L2  180.0f

/*==============================================================================
 * 数据类型
 *============================================================================*/
/** @brief 足端笛卡尔坐标（相对髋关节） */
typedef struct {
    float x;   /**< 前后位置（+前） */
    float z;   /**< 竖直位置（+下，远离髋关节） */
} ik_foot_pose_t;

/** @brief 一个 IK 步态相位 */
typedef struct {
    ik_foot_pose_t legs[LEG_COUNT]; /**< 所有腿的足端目标位置 */
    uint32_t       duration_ms;     /**< 过渡时间（ms） */
} ik_gait_phase_t;

/** @brief IK 步态序列 */
typedef struct {
    const ik_gait_phase_t *phases;      /**< 相位数组 */
    uint8_t                phase_count; /**< 相位数量 */
    uint8_t                loop;        /**< 0=单次, 1=循环 */
} ik_gait_sequence_t;
/** @brief IK 步态执行器状态 */
typedef struct {
    const ik_gait_sequence_t *sequence;     /* 当前步态 */
    uint8_t    phase_index;                  /* 当前相位索引 */
    uint32_t   phase_start_tick;             /* 当前相位起始 tick（ms） */
    uint8_t    active;                       /* 1=运行中 */
} ik_runner_t;

/*==============================================================================
 * 步态控制 API
 *============================================================================*/
/**
 * @brief 初始化 IK 步态控制器
 * @note 内部调用 Limb_Init()（含舵机映射配置与高速模式设定）。
 *       必须在 APP_Servo_Add 全部完成后调用。
 */
void Gait_IK_Init(void);

/**
 * @brief 开始执行 IK 步态
 * @param gait 步态序列指针
 * @note 立即计算第一相位的足端→IK→Limb_SetTarget，开始运动。
 *       调用前需确保舵机和 Limb 已初始化。
 */
void Gait_IK_Start(const ik_gait_sequence_t *gait);

/**
 * @brief 停止 IK 步态
 */
void Gait_IK_Stop(void);

/**
 * @brief 查询 IK 步态是否正在运行
 * @retval 1 运行中
 * @retval 0 空闲
 */
uint8_t Gait_IK_IsRunning(void);

/**
 * @brief IK 步态调度器（每 SERVO_TICK_MS 调用）
 * @note 检查当前相位持续时间是否已到，
 *       到时则推进到下一相位 足端→IK→Limb_SetTarget。
 *       不到时则跳过（Limb_Scheduler 负责插值）。
 */
void Gait_IK_Scheduler(void);

/*==============================================================================
 * 内置 IK 步态序列（在 app_gait.c 中定义）
 *
 * @note 以下角度值基于 LEG_L1=LEG_L2=50mm 推算，
 *       更换几何参数后足端位置可能需要调整。
 *============================================================================*/
extern const ik_gait_sequence_t GAIT_IK_STAND;   /**< 站立 */
extern const ik_gait_sequence_t GAIT_IK_SIT;      /**< 蹲下 */
extern const ik_gait_sequence_t GAIT_IK_WALK;     /**< 行走（trot 四相位） */

#endif /* __APP_GAIT_H__ */
