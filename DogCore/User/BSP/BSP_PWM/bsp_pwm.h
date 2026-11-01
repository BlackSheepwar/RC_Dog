/**
 * @file bsp_pwm.h
 * @brief PWM 硬件驱动层：一个定时器通道一个句柄，只做脉宽与占空比
 * @author Ricardo
 * @date 2026-11-01
 *
 * @note BSP 不知道什么是"度"、什么是"速度"：它只接受微秒脉宽或百分比占空比。
 *       角度→脉宽的换算在 APP_Servo，速度→占空比的换算在 APP_Motor。
 *
 *       定时器、通道、计数器频率全部写在 bsp_pwm.c 的句柄实例里：
 *       换引脚只改那一个文件，Device / APP 一行不动。
 *
 *       舵机与电机的定时器频率不同（舵机 1 MHz 计数 → 1 计数 = 1 μs；
 *       电机 10 MHz 计数 → 20 kHz、500 级分辨率），因此频率写在句柄里，
 *       换算时按句柄自己的 count_hz 走，不需要查表。
 */

#ifndef __BSP_PWM_H__
#define __BSP_PWM_H__

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include <stdint.h>

/*==============================================================================
 * 类型定义
 *============================================================================*/
/**
 * @brief PWM 通道句柄（不透明类型，布局在 bsp_pwm.c）
 */
typedef struct bsp_pwm bsp_pwm_t;

/*==============================================================================
 * 外部声明：每个用到的通道一份句柄
 *============================================================================*/
// 舵机 1~8（前腿 1~4，后腿 5~8）
extern const bsp_pwm_t bsp_pwm_servo_1;
extern const bsp_pwm_t bsp_pwm_servo_2;
extern const bsp_pwm_t bsp_pwm_servo_3;
extern const bsp_pwm_t bsp_pwm_servo_4;
extern const bsp_pwm_t bsp_pwm_servo_5;
extern const bsp_pwm_t bsp_pwm_servo_6;
extern const bsp_pwm_t bsp_pwm_servo_7;
extern const bsp_pwm_t bsp_pwm_servo_8;
// 电机 1~4，每路两个通道（正转 / 反转）
extern const bsp_pwm_t bsp_pwm_motor_1_fwd;
extern const bsp_pwm_t bsp_pwm_motor_1_rev;
extern const bsp_pwm_t bsp_pwm_motor_2_fwd;
extern const bsp_pwm_t bsp_pwm_motor_2_rev;
extern const bsp_pwm_t bsp_pwm_motor_3_fwd;
extern const bsp_pwm_t bsp_pwm_motor_3_rev;
extern const bsp_pwm_t bsp_pwm_motor_4_fwd;
extern const bsp_pwm_t bsp_pwm_motor_4_rev;

/*==============================================================================
 * 初始化
 *============================================================================*/
/**
 * @brief 按句柄表配置所有 PWM 定时器的预分频与自动重装值
 * @retval 无
 * @note 必须在任何 BSP_PWM_Start 之前调用一次：
 *       这里改了 PSC/ARR，之前设过的比较值全部失效。
 *       同一个定时器被多个通道共用时只会被配置一次，重复调用结果一样。
 */
void BSP_PWM_Init(void);

/*==============================================================================
 * 启停
 *============================================================================*/
/**
 * @brief 启动通道输出
 * @param pwm 通道句柄，可为 NULL
 * @retval 无
 * @note 高级定时器（TIM1/8/15/16/17）需要 BDTR.MOE 置位才有波形，
 *       这里在 HAL 启动后再写一次，重复调用安全。
 */
void BSP_PWM_Start(const bsp_pwm_t *pwm);

/**
 * @brief 停止通道输出
 * @param pwm 通道句柄，可为 NULL
 * @retval 无
 */
void BSP_PWM_Stop(const bsp_pwm_t *pwm);

/*==============================================================================
 * 占空比 / 脉宽
 *============================================================================*/
/**
 * @brief 设置高电平脉宽（舵机用）
 * @param pwm      通道句柄，可为 NULL
 * @param pulse_us 脉宽，单位 μs；超出周期时夹到周期值
 * @retval 无
 * @note 换算：比较值 = pulse_us × count_hz / 1e6，count_hz 取自句柄。
 */
void BSP_PWM_Set_Pulse_Us(const bsp_pwm_t *pwm, uint16_t pulse_us);

/**
 * @brief 设置占空比（电机、LED 用）
 * @param pwm     通道句柄，可为 NULL
 * @param percent 占空比 0.0 ~ 100.0，超出自动夹紧
 * @retval 无
 * @note 比较值 = (ARR + 1) × percent / 100，四舍五入。
 */
void BSP_PWM_Set_Duty_Percent(const bsp_pwm_t *pwm, float percent);

#endif /* __BSP_PWM_H__ */
