/**
 * @file app_motion.h
 * @brief 单腿运动控制层：百分比速度同步 + 关节级状态池
 * @author 李嘉图
 * @date 2026-07-01
 *
 * @note 全链路浮点运算，所有角度接口均使用 float（亚度精度）。
 *       核心功能：输入两个关节角度（用户/IK 坐标系）+ 速度百分比
 *       两个舵机自动同步到达（按百分比分配各关节速度）
 *       内部自动进行 校准偏移 + 方向反转 + 输入限幅 映射到舵机坐标系
 *       维护每关节 current/target/speed/reached 状态池。
 *       无需 IK，直接角度控制。
 *
 *       调用顺序：
 *       1. Motion_Init()          - 初始化（读取舵机位置填入状态池）
 *       2. Motion_SetTarget(...)  - 设置目标角度和速度百分比（用户坐标系）
 *       3. Motion_Scheduler()     - 每10ms调用一次（角度到达检测）
 *       4. Motion_GetCurrentAngles() — 读取当前实际角度（浮点）
 */

#ifndef __APP_MOTION_H__
#define __APP_MOTION_H__

#include <stdint.h>

/*==============================================================================
 * 常量
 *============================================================================*/
#define MOTION_MAX_LEGS    4        /* 最多支持4条腿 */
#define MOTION_JOINTS_PER_LEG  2    /* 每条腿2个关节 */
#define ANGLE_REACHED_THRESHOLD  1  /* 关节到达判定阈值(°) */

/*==============================================================================
 * 数据结构
 *============================================================================*/
/** @brief 单条腿的运动控制状态 */
typedef struct {
    /* ── 关节级状态池（用户空间，反转/校准前） ── */
    float   current_angle[MOTION_JOINTS_PER_LEG]; /**< 当前实际角度(°) */
    float   target_angle[MOTION_JOINTS_PER_LEG];  /**< 目标角度(°) */
    float   speed[MOTION_JOINTS_PER_LEG];         /**< 当前运行速度(°/s) */
    uint8_t reached[MOTION_JOINTS_PER_LEG];       /**< 1=该关节已到达目标 */

    /* ── 腿级状态 ── */
    uint32_t finish_tick;      /**< 预计完成时刻(ms) */
    uint8_t registered : 1;   /**< 1=已注册 */
    uint8_t moving : 1;       /**< 1=正在运动(至少一个关节未到达) */
} motion_leg_t;

/*==============================================================================
 * API
 *============================================================================*/
/**
 * @brief 初始化运动控制模块
 * @note 必须在舵机层初始化后调用
 */
void Motion_Init(void);

/**
 * @brief 设置腿的目标关节角度（百分比速度同步）
 * @param leg_id       腿编号 [1, MOTION_MAX_LEGS]
 * @param target_angle 目标角度数组 [hip_angle, knee_angle]，用户/IK 坐标系
 * @param speed        速度 [0.1 ~ 100.0]，浮点精度 0.1
 * @note 两个关节自动同步到达
 *       - speed=100.0 表示各关节以 speed_max 全速运动
 *       - speed=50.0  表示各关节以 50% speed_max 运动
 *       - 内部根据关节实际位移量自动配速，保证两关节同时到位
 */
void Motion_SetTarget(uint8_t leg_id, const float target_angle[MOTION_JOINTS_PER_LEG],
                      float speed);

/**
 * @brief 强制腿瞬间跳转到角度（无插值）
 * @param leg_id      腿编号
 * @param target_angle 目标角度数组
 */
void Motion_SetImmediate(uint8_t leg_id, const float target_angle[MOTION_JOINTS_PER_LEG]);

/**
 * @brief 周期调度器（每10ms调用一次）
 * @note 计算插值、限位、输出到舵机
 */
void Motion_Scheduler(void);

/**
 * @brief 查询腿是否正在运动
 * @param leg_id 腿编号
 * @return 1=运动中, 0=已到位或未注册
 */
uint8_t Motion_IsMoving(uint8_t leg_id);

/**
 * @brief 查询是否有任何腿在运动
 * @return 1=至少一条腿运动中, 0=全部静止
 */
uint8_t Motion_IsAnyMoving(void);

/**
 * @brief 停止腿的运动（保持当前角度）
 * @param leg_id 腿编号
 */
void Motion_Stop(uint8_t leg_id);

/**
 * @brief 停止所有腿
 */
void Motion_StopAll(void);

/**
 * @brief 读取腿的当前实际关节角度（从舵机层）
 * @param leg_id    腿编号
 * @param out_angle 输出数组 [hip_angle, knee_angle]，须由调用者分配
 * @note 用于 IK 层获取当前姿态反馈
 */
void Motion_GetCurrentAngles(uint8_t leg_id, float *out_angle);

/**
 * @brief 获取腿级状态池指针（只读）
 * @param leg_id 腿编号
 * @return 状态指针，可用于读取 current_angle/target_angle/speed/reached 等
 * @note 调试/监控用，配合 Motion_Scheduler 的实时更新
 */
const motion_leg_t *Motion_GetLegState(uint8_t leg_id);

#endif /* __APP_MOTION_H__ */
