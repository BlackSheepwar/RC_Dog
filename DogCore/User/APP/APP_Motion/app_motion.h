/**
 * @file app_motion.h
 * @brief 单腿运动控制：关节角度 → 舵机角度映射 + 按百分比速度同步到位
 * @author Ricardo
 * @date 2026-11-01
 *
 * @note 这一层解决"两条腿的关节怎么同时到位"：
 *       入参是用户/IK 坐标系的角度与 0.1 ~ 100 的速度百分比，
 *       出参是每条腿两个舵机的目标角与角速度。
 *
 *       每个关节的映射参数（校准偏移、预/后反转、输入限位）写在
 *       app_motion.c 的腿配置表里——那是机械安装差异，不是业务。
 *
 *       调用顺序：
 *         1. APP_Motion_Init()            读取当前舵机位置填入状态池
 *         2. APP_Motion_Set_Target(...)   下目标角 + 速度百分比
 *         3. APP_Motion_Scheduler()       每 10 ms 检测到位（由调度层调用）
 *         4. APP_Motion_Get_Current_Angles() 读当前角度（IK/协议用）
 *
 *       ⚠ 真正让舵机转起来的是 Svc_Servo_Motion_Tick()，
 *         本层只计算目标与速度，不做插值输出。
 *
 *       职责边界（这个文件夹只有三件事）：
 *         app_motion.c            腿级：关节角 → 舵机角（安装映射 + 同步配速 + 到位）
 *         app_motion_scheduler.c  步态：相位推进 + 把一个相位下发给各腿
 *         app_gait_seq.h          数据：GAIT_xxx 序列，只有数值没有逻辑
 *       二连杆逆解不在这里：纯算法在 Common/Common_Ik，
 *       本层只提供每条腿的杆长（APP_Motion_Get_Link）。
 */

#ifndef __APP_Motion_H__
#define __APP_Motion_H__

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include <stdint.h>
#include "common_ik.h"          /* 腿的几何参数：两段杆长 */

/*==============================================================================
 * 宏定义与常量
 *============================================================================*/
/** @brief 腿的数量 */
#define APP_MOTION_MAX_LEGS         4u
/** @brief 每条腿的关节数（髋、膝） */
#define APP_MOTION_JOINTS_PER_LEG   2u
/** @brief 关节到位判定阈值（度） */
#define APP_MOTION_REACHED_DEG      1.0f
/** @brief 速度百分比的合法区间 */
#define APP_MOTION_SPEED_MIN_PCT    0.1f
#define APP_MOTION_SPEED_MAX_PCT    100.0f

/*==============================================================================
 * 类型定义
 *============================================================================*/
/**
 * @brief 单条腿的运行时状态（不透明类型，布局在 app_motion.c）
 * @note 上层不需要读它的成员：要角度有 APP_Motion_Get_Current_Angles，
 *       要运动状态有 APP_Motion_Is_Moving。
 */
typedef struct app_motion_leg app_motion_leg_t;

/*==============================================================================
 * API
 *============================================================================*/
/**
 * @brief 初始化运动控制模块
 * @retval 无
 * @note 必须在 APP_Servo 注册完成之后调用：初始化要读舵机当前位置，
 *       把 current_angle 与 target_angle 对齐，避免上电瞬间"跳一下"。
 */
void APP_Motion_Init(void);

/**
 * @brief 设置一条腿的目标关节角
 * @param leg_id       腿编号，1 ~ APP_MOTION_MAX_LEGS
 * @param target_angle 目标角数组 [髋, 膝]，用户/IK 坐标系（度），不可为 NULL
 * @param speed        速度百分比 0.1 ~ 100.0，超出自动夹紧
 * @retval 无
 * @note 两个关节按"慢的那个匹配"自动配速，保证同时到位。
 *       本函数只写目标值与速度，实际移动由舵机层平滑完成。
 */
void APP_Motion_Set_Target(uint8_t leg_id,
                           const float target_angle[APP_MOTION_JOINTS_PER_LEG],
                           float speed);

/**
 * @brief 直接跳到目标角（不做同步配速）
 * @param leg_id       腿编号
 * @param target_angle 目标角数组 [髋, 膝]（度），不可为 NULL
 * @retval 无
 * @note 舵机层仍有自己的角速度限制，所以这不是"瞬移"，
 *       只是本层不再为它计算同步速度、并立刻标记为已到位。
 */
void APP_Motion_Set_Immediate(uint8_t leg_id,
                              const float target_angle[APP_MOTION_JOINTS_PER_LEG]);

/**
 * @brief 到位检测调度
 * @retval 无
 * @note 每 10 ms 调一次：读舵机当前位置 → 与目标比较 →
 *       两关节都进入阈值内就清 moving 标志，供调度层推进相位。
 */
void APP_Motion_Scheduler(void);

/**
 * @brief 查询某条腿是否还在运动
 * @param leg_id 腿编号
 * @retval 1: 至少一个关节未到位
 * @retval 0: 已到位或编号非法
 */
uint8_t APP_Motion_Is_Moving(uint8_t leg_id);

/**
 * @brief 查询是否有任意一条腿在运动
 * @retval 1: 至少一条腿在运动
 * @retval 0: 全部静止
 */
uint8_t APP_Motion_Is_Any_Moving(void);

/**
 * @brief 冻结某条腿（把舵机停在上一次的目标位置）
 * @param leg_id 腿编号
 * @retval 无
 * @note 停止后状态池同步到当前位置，下次 Set_Target 从这里出发。
 */
void APP_Motion_Stop(uint8_t leg_id);

/**
 * @brief 冻结所有腿
 * @retval 无
 * @note 步态切换与急停用。
 */
void APP_Motion_Stop_All(void);

/**
 * @brief 取某条腿的杆长（逆解用）
 * @param leg_id 腿编号，1 ~ APP_MOTION_MAX_LEGS
 * @retval 非 NULL: 该腿的杆长
 * @retval NULL: 腿号非法
 * @note 杆长是这条腿的机械参数，跟安装偏移、限位一起写在腿配置表里；
 *       逆解算法在 Common/Common_Ik，本模块只负责把参数喂进去。
 */
const common_ik_link_t *APP_Motion_Get_Link(uint8_t leg_id);

/**
 * @brief 读取某条腿当前的关节角
 * @param leg_id    腿编号
 * @param out_angle 输出数组 [髋, 膝]（度），不可为 NULL
 * @retval 无
 * @note 读的是舵机层的实际位置并反算回用户坐标系，可作 IK 反馈。
 */
void APP_Motion_Get_Current_Angles(uint8_t leg_id, float *out_angle);

#endif /* __APP_Motion_H__ */
