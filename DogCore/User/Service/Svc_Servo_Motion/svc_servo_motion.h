/**
 * @file svc_servo_motion.h
 * @brief 舵机运动服务：限速 + 平滑逼近（带时间轴，与产品无关）
 * @author Ricardo
 * @date 2026-11-01
 *
 * @note 为什么这一段在 Service 而不是 APP：它做的事是"把当前角度按给定角速度
 *       挪向目标角度，10 ms 走一步"，里面有时间、有流程，且对任何舵机都成立
 *       ——与"这台机器怎么走"无关。设备层只认"给角度立刻输出"，
 *       业务层只认"我要它到哪"，中间的过渡由本层负责。
 *
 *       本层不含引脚、不含定时器、不含脉宽：脉宽换算是 Dev_Servo 的事，
 *       本层只和"角度"打交道。
 *
 *       句柄化：一台舵机的运动状态（当前/目标/角速度）是一个 svc_servo_motion_t。
 *       Init 时为每台已初始化的舵机建一份，上层用 Svc_Servo_Motion_Of(设备句柄)
 *       拿到它——业务层因此不再需要"舵机几号"这种数字编号。
 *       句柄是不完整类型：能传能比，不能取成员。
 *
 *       本层不看时钟：tick 收 elapsed_ms，时间由 TASK 层差分后传进来，
 *       因此可以直接在 PC 上喂假时间跑单测。
 */

#ifndef __Svc_Servo_Motion_H__
#define __Svc_Servo_Motion_H__

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include <stdint.h>
#include "dev_servo.h"          /* Svc_Servo_Motion_Of 要收设备句柄 */

/*==============================================================================
 * 宏定义与常量
 *============================================================================*/
/** @brief 最多管理的舵机数（要 ≥ Dev_Servo_Count()） */
#define SVC_SERVO_MOTION_MAX         8u
/** @brief 到位判定阈值（度）：小于它就直接贴到目标值 */
#define SVC_SERVO_MOTION_REACHED_DEG 0.001f
/** @brief 缺省角速度（度/秒），Init 后由上层按需覆盖 */
#define SVC_SERVO_MOTION_DEFAULT_DPS 100.0f

/*==============================================================================
 * 类型定义
 *============================================================================*/
/**
 * @brief 单台舵机的运动句柄（不透明类型，布局在 svc_servo_motion.c）
 */
typedef struct svc_servo_motion svc_servo_motion_t;

/*==============================================================================
 * 建立与查找
 *============================================================================*/
/**
 * @brief 为每台已初始化的舵机建立运动槽位
 * @retval 0: 成功
 * @retval -1: 设备数量为 0 或超过 SVC_SERVO_MOTION_MAX
 * @note 必须在 Dev_Servo_Init() 之后调用：此时"当前角"才有意义。
 *       会沿用设备层记录的当前角作为初始 target，避免上电瞬间跳一下。
 */
int Svc_Servo_Motion_Init(void);

/**
 * @brief 取已建立的槽位数
 * @retval 槽位数（0 表示未初始化）
 */
uint8_t Svc_Servo_Motion_Count(void);

/**
 * @brief 按顺序取句柄（遍历/Tick 用）
 * @param index 0 ~ Count-1
 * @retval 非 NULL: 句柄
 * @retval NULL: 序号越界
 */
svc_servo_motion_t *Svc_Servo_Motion_Get(uint8_t index);

/**
 * @brief 由设备句柄查出对应的运动句柄
 * @param dev 舵机设备句柄，可为 NULL
 * @retval 非 NULL: 该舵机的运动句柄
 * @retval NULL: 设备为空或未被本服务接管
 * @note 业务层把自己的安装表写成"设备句柄"，初始化时用本函数换成运动句柄，
 *       之后所有调用都用句柄，不再出现编号。
 *       返回的是非 const 指针：句柄本身就是"可改的状态"（目标角、角速度），
 *       而类型不完整，上层既解不了成员也 sizeof 不了，只能原样传回给本层函数。
 */
svc_servo_motion_t *Svc_Servo_Motion_Of(const dev_servo_t *dev);

/*==============================================================================
 * 目标与参数
 *============================================================================*/
/**
 * @brief 设置目标角度
 * @param motion 运动句柄，可为 NULL（安全返回）
 * @param deg    目标角（度），内部按设备限位夹紧
 * @retval 无
 */
void Svc_Servo_Motion_Set_Target(svc_servo_motion_t *motion, float deg);

/**
 * @brief 在当前目标角上叠加增量
 * @param motion 运动句柄，可为 NULL
 * @param delta  角度增量（度），可为负
 * @retval 无
 */
void Svc_Servo_Motion_Step_Target(svc_servo_motion_t *motion, float delta);

/**
 * @brief 设置角速度
 * @param motion 运动句柄，可为 NULL
 * @param dps    角速度（度/秒），负值按 0 处理（等于不再移动）
 * @retval 无
 */
void Svc_Servo_Motion_Set_Speed(svc_servo_motion_t *motion, float dps);

/*==============================================================================
 * 查询
 *============================================================================*/
/**
 * @brief 取当前角（系统认为它现在在哪）
 * @param motion 运动句柄，可为 NULL
 * @retval 当前角（度）；句柄为空时返回 0
 */
float Svc_Servo_Motion_Get_Current(const svc_servo_motion_t *motion);

/**
 * @brief 取目标角
 * @param motion 运动句柄，可为 NULL
 * @retval 目标角（度）；句柄为空时返回 0
 */
float Svc_Servo_Motion_Get_Target(const svc_servo_motion_t *motion);

/**
 * @brief 查询是否还在动
 * @param motion 运动句柄，可为 NULL
 * @retval 1: 未到位
 * @retval 0: 已到位或句柄为空
 */
uint8_t Svc_Servo_Motion_Is_Moving(const svc_servo_motion_t *motion);

/*==============================================================================
 * 推进
 *============================================================================*/
/**
 * @brief 推进一帧：所有已接管舵机朝各自目标走一步并输出
 * @param elapsed_ms 距上次调用的毫秒数
 * @retval 1: 至少有一台还没到位
 * @retval 0: 全部到位（或未初始化）
 * @note 周期建议 10 ms。elapsed_ms 为 0 时只重发输出、不推进。
 *       为什么是"全部一起推进"而不是逐个：同一条腿的两个关节必须同一拍起步，
 *       否则会看到两个关节交替领先，走路时表现为腿在抖。
 */
uint8_t Svc_Servo_Motion_Tick_All(uint32_t elapsed_ms);

/**
 * @brief 把所有舵机冻结在当前角度（目标 := 当前）
 * @retval 无
 * @note 急停/步态切换用：之后的新目标从当前位置重新起步，不会回跳。
 */
void Svc_Servo_Motion_Hold_All(void);

#endif /* __Svc_Servo_Motion_H__ */
