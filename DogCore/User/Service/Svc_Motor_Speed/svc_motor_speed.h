/**
 * @file svc_motor_speed.h
 * @brief 电机速度环服务：编码器测速 + PID 闭环 + 输出限幅
 * @author Ricardo
 * @date 2026-11-01
 *
 * @note 为什么在 Service：这里有时间（dt）、有流程（采样→换算→PID→限幅→输出），
 *       与"这台机器用轮子做什么"无关；换成任何一台带编码器的电机都成立。
 *       PID 算法本身在 Common/Common_Pid（纯数学），本层只是调用者。
 *
 *       速度刻度：1.0 ≈ 输出轴 0.01 转/秒。
 *       公式 speed = count × 100 / (cpr × dt)，cpr 取自设备层。
 *       沿用这一刻度是为了让现场整定的目标值（200 / 350 等）继续有效。
 *
 *       句柄化：一台电机的速度环状态是一个 svc_motor_speed_t。
 *       Init 时为每台已初始化的电机建一份，上层用 Svc_Motor_Speed_Of(设备句柄)
 *       拿到它，业务层因此不再出现"电机几号"。
 *
 *       本层不看时钟：tick 收 elapsed_ms，TASK 层负责差分。
 */

#ifndef __Svc_Motor_Speed_H__
#define __Svc_Motor_Speed_H__

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include <stdint.h>
#include "dev_motor.h"          /* Svc_Motor_Speed_Of 要收设备句柄 */

/*==============================================================================
 * 宏定义与常量
 *============================================================================*/
/** @brief 最多管理的电机数（要 ≥ Dev_Motor_Count()） */
#define SVC_MOTOR_SPEED_MAX         8u
/** @brief 目标速度上限（刻度值），超出自动夹紧 */
#define SVC_MOTOR_SPEED_LIMIT       500.0f
/** @brief 停车判定阈值：目标为 0 且实测小于它就算停稳 */
#define SVC_MOTOR_SPEED_STOP_BAND   0.5f
/** @brief 缺省 PID 参数（速度环，现场整定值） */
#define SVC_MOTOR_SPEED_KP          1.7f
#define SVC_MOTOR_SPEED_KI          1.0f
#define SVC_MOTOR_SPEED_KD          0.8f

/*==============================================================================
 * 类型定义
 *============================================================================*/
/**
 * @brief 单台电机的速度环句柄（不透明类型，布局在 svc_motor_speed.c）
 */
typedef struct svc_motor_speed svc_motor_speed_t;

/*==============================================================================
 * 建立与查找
 *============================================================================*/
/**
 * @brief 为每台已初始化的电机建立速度环槽位并初始化 PID
 * @retval 0: 成功
 * @retval -1: 设备数量为 0 或超过 SVC_MOTOR_SPEED_MAX
 * @note 必须在 Dev_Motor_Init() 之后调用。
 */
int Svc_Motor_Speed_Init(void);

/**
 * @brief 取已建立的槽位数
 * @retval 槽位数（0 表示未初始化）
 */
uint8_t Svc_Motor_Speed_Count(void);

/**
 * @brief 按顺序取句柄（遍历/调试显示用）
 * @param index 0 ~ Count-1
 * @retval 非 NULL: 句柄
 * @retval NULL: 序号越界
 */
svc_motor_speed_t *Svc_Motor_Speed_Get(uint8_t index);

/**
 * @brief 由设备句柄查出对应的速度环句柄
 * @param dev 电机设备句柄，可为 NULL
 * @retval 非 NULL: 该电机的速度环句柄
 * @retval NULL: 设备为空或未被本服务接管
 */
svc_motor_speed_t *Svc_Motor_Speed_Of(const dev_motor_t *dev);

/*==============================================================================
 * 目标与参数
 *============================================================================*/
/**
 * @brief 设置目标速度
 * @param speed 速度环句柄，可为 NULL（安全返回）
 * @param value 目标速度（刻度值），超出 ±SVC_MOTOR_SPEED_LIMIT 自动夹紧
 * @retval 无
 * @note 正值表示轮子往前——镜像安装的电机已在设备层换算过极性。
 */
void Svc_Motor_Speed_Set_Target(svc_motor_speed_t *speed, float value);

/**
 * @brief 在线改 PID 增益
 * @param speed 速度环句柄，可为 NULL
 * @param kp    比例系数
 * @param ki    积分系数
 * @param kd    微分系数
 * @retval 0: 成功
 * @retval -1: 句柄为 NULL
 * @note 只改增益、不清积分，调参瞬间输出不跳变。
 */
int Svc_Motor_Speed_Set_Gain(svc_motor_speed_t *speed, float kp, float ki, float kd);

/*==============================================================================
 * 查询
 *============================================================================*/
/**
 * @brief 取当前实测速度
 * @param speed 速度环句柄，可为 NULL
 * @retval 实测速度（刻度值）；句柄为空时返回 0
 */
float Svc_Motor_Speed_Get_Actual(const svc_motor_speed_t *speed);

/**
 * @brief 取目标/实测/输出三件套（调参与上位机显示用）
 * @param speed  速度环句柄，不可为 NULL
 * @param target 输出目标值，可为 NULL（不需要就不取）
 * @param actual 输出实测值，可为 NULL
 * @param out    输出 PID 输出值（占空比百分比），可为 NULL
 * @retval 0: 成功
 * @retval -1: 句柄为 NULL
 */
int Svc_Motor_Speed_Get_State(const svc_motor_speed_t *speed,
                              float *target, float *actual, float *out);

/*==============================================================================
 * 推进
 *============================================================================*/
/**
 * @brief 跑一轮闭环：所有电机读增量 → 换算速度 → PID → 限幅 → 输出
 * @param elapsed_ms 距上次调用的毫秒数
 * @retval 无
 * @note 周期建议 10 ms，必须与整定时使用的采样周期一致，否则实测速度整体缩放。
 *       elapsed_ms 为 0 时整轮跳过（无法换算速度，强算会除零）。
 *       所有电机同一拍采样：否则两侧轮子的时间基准不同，直行会跑偏。
 */
void Svc_Motor_Speed_Tick_All(uint32_t elapsed_ms);

/**
 * @brief 全部停机：清零目标、复位 PID、占空比归零
 * @retval 无
 * @note 急停/故障处理用；之后要重新动必须先 Set_Target。
 */
void Svc_Motor_Speed_Stop_All(void);

#endif /* __Svc_Motor_Speed_H__ */
