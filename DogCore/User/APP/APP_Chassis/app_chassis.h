/**
 * @file app_chassis.h
 * @brief 底盘业务层：四个轮子成组下发速度
 * @author Ricardo
 * @date 2026-11-01
 *
 * @note 这一层只回答"这台机器有几个轮子、每个轮子该多快"，
 *       不碰 PWM、不碰 PID、更不记"2 号轮要给负速度"这种接线细节
 *       （镜像极性已经在 Dev_Motor 里换算过）。
 *
 *       存在的理由：命令表里有 5 条命令都要"四个轮子一起设"，
 *       电机句柄只有这里知道；散在命令表里就是 4 个 static 指针各写一遍。
 */

#ifndef __APP_Chassis_H__
#define __APP_Chassis_H__

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include <stdint.h>

/*==============================================================================
 * 宏定义与常量
 *============================================================================*/
/** @brief 轮子数量（与 Dev_Motor 的实例数一致） */
#define APP_CHASSIS_WHEELS      4u
/** @brief 缺省巡航速度（刻度值，1.0 ≈ 0.01 转/秒） */
#define APP_CHASSIS_SPEED_WALK  200.0f
/** @brief 轮子直驱速度（抬腿只靠轮子跑时用） */
#define APP_CHASSIS_SPEED_ROLL  350.0f

/*==============================================================================
 * 接口
 *============================================================================*/
/**
 * @brief 把设备层的电机句柄换成速度环句柄
 * @retval 0: 成功
 * @retval -1: 设备数量不足，或某个电机没被速度环接管
 * @note 必须在 Dev_Motor_Init() 与 Svc_Motor_Speed_Init() 之后调用。
 */
int APP_Chassis_Init(void);

/**
 * @brief 设置单个轮子的速度
 * @param wheel    轮号，1 ~ APP_CHASSIS_WHEELS（1/3 左，2/4 右）
 * @param velocity 速度刻度值，正值 = 轮子往前
 * @retval 无
 * @note 越界轮号静默忽略；限幅在速度环里做。
 */
void APP_Chassis_Set_Wheel(uint8_t wheel, float velocity);

/**
 * @brief 四个轮子同时给同一速度
 * @param velocity 速度刻度值，正值 = 整车前进
 * @retval 无
 * @note 前进/后退/"原地滚"这类命令用它；镜像极性由设备层负责，
 *       所以这里不需要再写 200/-200/200/-200。
 */
void APP_Chassis_Set_All(float velocity);

/**
 * @brief 四个轮子全部停转
 * @retval 无
 * @note 只清目标，速度环会把占空比收到 0（并清积分）。
 */
void APP_Chassis_Stop(void);

/**
 * @brief 查询某个轮子的目标速度
 * @param wheel 轮号，1 ~ APP_CHASSIS_WHEELS
 * @retval 目标速度；轮号非法时返回 0
 */
float APP_Chassis_Get_Wheel_Velocity(uint8_t wheel);

#endif /* __APP_Chassis_H__ */
