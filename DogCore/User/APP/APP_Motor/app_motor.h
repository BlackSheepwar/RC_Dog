/**
 * @file app_motor.h
 * @brief 电机控制（APP层）- PID调速 + 速度闭环控制
 * @author 李嘉图
 * @date 2026-07-05
 *
 * @note 每个电机使用双PWM控制 AT8236 驱动芯片：
 *         pwm_fwd = 正向PWM，pwm_rev = 反向PWM
 *         正向转时 pwm_fwd = duty, pwm_rev = 0
 *         反向转时 pwm_fwd = 0, pwm_rev = duty
 *         停止时 pwm_fwd = pwm_rev = 0
 *
 *       PID状态由本层维护（每个电机一个 PID_Handle_t），
 *       PID算法层（Middlewares/PID）只做纯数学运算。
 *
 *       调度器周期缺省 10ms，由 CubeMX 创建的 RTOS 任务周期调用
 *       APP_Motor_RunPID() 触发一次完整闭环。
 */

#ifndef __APP_MOTOR_H__
#define __APP_MOTOR_H__

/*==============================================================================
 * 头文件包含
 *============================================================================*/
#include <stdint.h>

/*==============================================================================
 * 宏定义与常量
 *============================================================================*/
#define MOTOR_NUM              4            /* 电机总数                    */
#define MOTOR_PID_TICK_S       0.01f         /* PID调度周期（秒）          */
#define MOTOR_PID_TICK_MS      10            /* PID调度周期（毫秒）        */
#define MOTOR_SPEED_MAX        500.0f        /* 最大速度（标定后 target=100 = 满速） */

/*==============================================================================
 * 初始化函数
 *============================================================================*/
/**
 * @brief 电机 APP 层初始化
 * @note 初始化所有电机 PID 句柄、启动 PWM 输出、清零编码器。
 *       需在创建 RTOS 任务前调用一次。
 */
void APP_Motor_Init(void);

/*==============================================================================
 * 控制函数
 *============================================================================*/
/**
 * @brief 设置电机目标速度
 * @param motor_id 电机编号（Motor_ID_t）
 * @param speed    目标速度（-MOTOR_SPEED_MAX ~ +MOTOR_SPEED_MAX）
 *                 正数 = 正转，负数 = 反转，0 = 停止
 * @note 速度值会被限幅到 ±MOTOR_SPEED_MAX 范围内。
 *       内部调用 PID_SetTarget 更新目标值，PID 调度时自动追踪。
 */
void APP_Motor_SetSpeed(uint8_t motor_id, float speed);

/**
 * @brief 获取电机当前实际速度
 * @param motor_id 电机编号
 * @param speed    输出速度值的指针
 * @retval 1: 成功
 * @retval 0: 失败（motor_id 无效或 speed == NULL）
 */
uint8_t APP_Motor_GetSpeed(uint8_t motor_id, float *speed);

/**
 * @brief 设置电机 PID 参数
 * @param motor_id 电机编号
 * @param Kp       比例系数
 * @param Ki       积分系数
 * @param Kd       微分系数
 * @retval 1: 成功
 * @retval 0: 失败
 */
uint8_t APP_Motor_SetPID(uint8_t motor_id, float Kp, float Ki, float Kd);

/**
 * @brief 紧急停止所有电机
 * @note 清零所有 PWM 输出，重置所有 PID 状态。
 *       调用后电机自由停止（AT8236 输出高阻/制动取决于硬件）。
 */
void APP_Motor_StopAll(void);

/*==============================================================================
 * 状态查询（供调试模块等外部调用）
 *============================================================================*/
/**
 * @brief 获取指定电机的 PID 状态
 * @param motor_id 电机编号（1 ~ MOTOR_NUM）
 * @param target   输出目标值指针
 * @param actual   输出实际值指针
 * @param out      输出 PID 输出值指针
 * @retval 1: 成功
 * @retval 0: 失败（motor_id 无效或指针为 NULL）
 */
uint8_t APP_Motor_GetPIDState(uint8_t motor_id, float *target, float *actual, float *out);

/*==============================================================================
 * PID 调度函数（由 TASK 层周期调用）
 *============================================================================*/
/**
 * @brief 电机 PID 闭环调度（MOTOR_PID_TICK_MS 周期调用）
 * @note 遍历所有电机：
 *       1. 读取编码器计数值
 *       2. 换算为实际速度
 *       3. 调用增量式 PID 计算（输出自动累加到 pid.out）
 *       4. 输出限幅到 ±100
 *       5. 通过 BSP_PWM_SetDuty 写入硬件
 *
 *       此函数放在 RTOS 任务或定时器中断中周期执行。
 *       调度周期需与 MOTOR_PID_TICK_MS 一致。
 */
void APP_Motor_RunPID(void);

#endif /* __APP_MOTOR_H__ */
