/**
 * @file app_limb.h
 * @brief 肢体控制层：时间基插值调度
 * @author 李嘉图
 * @date 2026-06-11
 *
 * @note 本模块位于步态层与舵机层之间：
 *       上层（步态/IK）调用 Limb_SetTarget 设定目标角度+过渡时间，
 *       Limb_Scheduler 每 SERVO_TICK_MS 插值一次并写入 APP_Servo_SetTarget，
 *       由 APP_Servo 完成最终的 PWM 输出。
 *
 *       舵机→肢体的映射在 app_limb.c 中定义为 static const 表，
 *       编译期完成绑定，无需运行时软注册。
 */

#ifndef __APP_LIMB_H__
#define __APP_LIMB_H__

/*==============================================================================
 * 头文件包含
 *============================================================================*/
 // 固定包含
#include <stdint.h>

/*==============================================================================
 * 常量
 *============================================================================*/
#define LIMB_JOINT_COUNT    2   /* 每肢体关节数（髋、膝） */
#define LIMB_MAX            4   /* 最大肢体数 */

/*==============================================================================
 * 肢体状态
 *============================================================================*/
/** @brief 肢体运行时状态 */
typedef struct {
    /* — 插值状态 — */
    int16_t current_joint[LIMB_JOINT_COUNT]; /* 当前插值角度 */
    int16_t start_joint[LIMB_JOINT_COUNT];   /* 本次运动起始角度 */
    int16_t target_joint[LIMB_JOINT_COUNT];  /* 目标角度 */

    uint32_t start_tick;     /* 运动起始钟（ms） */
    uint32_t duration_ms;    /* 运动持续时间（ms） */

    uint8_t registered : 1;  /* 1=已注册（编译期预置） */
    uint8_t moving : 1;      /* 1=正在插值运动中 */
} limb_t;

/*==============================================================================
 * 初始化
 *============================================================================*/
/**
 * @brief 初始化肢体控制模块
 * @note 初始化运行状态，设定各关节舵机为高速跟随模式。
 *       所有肢体的 registered 标记由编译期常量和 Init 共同完成，
 *       无需显式注册调用。
 */
void Limb_Init(void);

/*==============================================================================
 * 运动控制
 *============================================================================*/
/**
 * @brief 设置肢体目标角度（时间基插值）
 * @param limb_id     肢体编号
 * @param target      目标关节角度数组
 * @param duration_ms 过渡时间（ms）
 * @note 自动快照当前角度为起始值，开始时间基插值。
 *       若目标与当前一致，moving 会立即清零。
 */
void Limb_SetTarget(uint8_t limb_id,
                    const int16_t target[LIMB_JOINT_COUNT],
                    uint32_t duration_ms);

/**
 * @brief 强制跳转到指定角度（瞬间到位）
 * @param limb_id 肢体编号
 * @param target  目标关节角度数组
 */
void Limb_SetImmediate(uint8_t limb_id,
                       const int16_t target[LIMB_JOINT_COUNT]);

/*==============================================================================
 * 状态查询
 *============================================================================*/
/**
 * @brief 查询指定肢体是否正在运动
 * @param limb_id 肢体编号
 * @retval 1 运动中
 * @retval 0 已到位或未注册
 */
uint8_t Limb_IsMoving(uint8_t limb_id);

/**
 * @brief 查询是否有任何肢体在运动
 * @retval 1 存在运动中肢体
 * @retval 0 全部静止
 */
uint8_t Limb_IsAnyMoving(void);

/**
 * @brief 获取肢体当前插值角度
 * @param limb_id 肢体编号
 * @param out     [out] 当前角度数组
 */
void Limb_GetCurrent(uint8_t limb_id, int16_t out[LIMB_JOINT_COUNT]);

/**
 * @brief 停止肢体运动（保持在当前角度）
 * @param limb_id 肢体编号
 */
void Limb_Stop(uint8_t limb_id);

/**
 * @brief 停止所有肢体运动
 */
void Limb_StopAll(void);

/*==============================================================================
 * 调度器（每 SERVO_TICK_MS 调用一次）
 *============================================================================*/
/**
 * @brief 肢体时间基插值调度器
 * @note 遍历所有注册的肢体，计算时间进度 t ∈ [0,1] 并线性插值，
 *       将结果通过 APP_Servo_SetTarget 写入舵机目标。
 *
 *       舵机层的 speed_dps 由 Limb_Init 设为高速（如 5000 dps），
 *       使单个 tick 内即可跟上 Limb 的目标值。
 */
void Limb_Scheduler(void);

#endif /* __APP_LIMB_H__ */
