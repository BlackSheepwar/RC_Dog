/**
 * @file dev_motor.h
 * @brief 电机设备层：一台电机一个句柄，管"双 PWM 正反转"与编码器计数
 * @author Ricardo
 * @date 2026-11-01
 *
 * @note 一台 AT8236 需要两路 PWM（正转一路、反转一路），配一个正交编码器。
 *       这三样东西合成一个设备句柄，上层从此不必自己配对句柄。
 *
 *       本层**不算速度、不跑 PID**：只给"占空比百分比"和"本周期有多少个脉冲"。
 *       脉冲数 → 速度要除时间，闭环要时间轴，两者都在
 *       Service/Svc_Motor_Speed。
 */

#ifndef __Dev_Motor_H__
#define __Dev_Motor_H__

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include <stdint.h>

/*==============================================================================
 * 类型定义
 *============================================================================*/
/**
 * @brief 电机句柄（不透明类型，布局在 dev_motor.c）
 */
typedef struct dev_motor dev_motor_t;

/*==============================================================================
 * 外部声明：一台电机一份句柄
 *============================================================================*/
/** @brief 1 号电机（左前） */
extern const dev_motor_t dev_motor_1;
/** @brief 2 号电机（右前） */
extern const dev_motor_t dev_motor_2;
/** @brief 3 号电机（左后） */
extern const dev_motor_t dev_motor_3;
/** @brief 4 号电机（右后） */
extern const dev_motor_t dev_motor_4;

/*==============================================================================
 * 初始化与枚举
 *============================================================================*/
/**
 * @brief 启动所有电机的两路 PWM 并把占空比归零，同时启动编码器
 * @retval 0: 全部成功
 * @retval -1: 句柄表里有空指针或 cpr 非法
 * @note 必须在 BSP_PWM_Init()（定时器频率）之后调用。
 *       上电即归零：避免定时器刚启动时比较值残留在别的通道上。
 */
int Dev_Motor_Init(void);

/**
 * @brief 取电机台数
 * @retval 已实例化的电机数量（当前 4）
 */
uint8_t Dev_Motor_Count(void);

/**
 * @brief 按槽位取电机句柄
 * @param index 槽位，0 ~ Dev_Motor_Count()-1
 * @retval 非 NULL: 电机句柄
 * @retval NULL: 槽位越界
 */
const dev_motor_t *Dev_Motor_Get(uint8_t index);

/*==============================================================================
 * 输出
 *============================================================================*/
/**
 * @brief 设置电机输出
 * @param motor 电机句柄，可为 NULL
 * @param duty  占空比 -100.0 ~ +100.0，正值正转、负值反转、0 为停止
 * @retval 0: 已输出
 * @retval -1: 句柄为 NULL
 * @note 内部完成三件事：按 reverse 换算接线极性、把符号拆成 fwd/rev 两路、
 *       另一路给 0；超出 ±100 自动夹紧。
 *       换算后"正值"始终表示"轮子往前"，与安装在哪一侧无关。
 */
int Dev_Motor_Set_Duty(const dev_motor_t *motor, float duty);

/**
 * @brief 停止这台电机（两路占空比归零）
 * @param motor 电机句柄，可为 NULL
 * @retval 无
 * @note AT8236 在双 0 时是滑行还是刹车取决于硬件接法，本层不假设。
 */
void Dev_Motor_Stop(const dev_motor_t *motor);

/*==============================================================================
 * 编码器
 *============================================================================*/
/**
 * @brief 读本周期的计数增量并清零计数器（测速用）
 * @param motor 电机句柄，不可为 NULL
 * @param delta 输出增量，不可为 NULL
 * @retval 0: 成功
 * @retval -1: 参数为 NULL
 * @note 读与清不是原子的：清零瞬间来的脉冲会算进下一周期，不会丢，
 *       但可能让本周期少 1。速度环 10 ms 一次，这个误差可忽略。
 *       返回值已按 reverse 换算：轮子往前时增量为正。
 */
int Dev_Motor_Read_Delta(const dev_motor_t *motor, int16_t *delta);

/**
 * @brief 读累计计数但不清零（标定线数用）
 * @param motor 电机句柄，不可为 NULL
 * @param raw   输出累计计数，不可为 NULL
 * @retval 0: 成功
 * @retval -1: 参数为 NULL
 * @note 同样按 reverse 换算，标定时不用再关心电机装在哪一侧。
 */
int Dev_Motor_Read_Raw(const dev_motor_t *motor, int16_t *raw);

/**
 * @brief 把编码器计数器清零
 * @param motor 电机句柄，可为 NULL
 * @retval 无
 */
void Dev_Motor_Reset_Count(const dev_motor_t *motor);

/**
 * @brief 取输出轴每转脉冲数（含减速比）
 * @param motor 电机句柄，可为 NULL
 * @retval 每转脉冲数；句柄为 NULL 时返回 0
 * @note 速度换算要用它，属机械参数，写在设备表里；
 *       换电机/换减速箱只改设备表，上层不重算。
 */
float Dev_Motor_Cpr(const dev_motor_t *motor);

#endif /* __Dev_Motor_H__ */
