/**
 * @file bsp_pwm.h
 * @brief 通用PWM输出BSP层（通过逻辑ID映射到定时器通道）
 * @author 李嘉图
 * @date 2026-06-07
 *
 * @note BSP层不知道什么是"度"，只提供脉宽设定接口。
 *       角度→脉宽的换算由上层（APP_Servo）负责。
 *
 *       硬件映射表（pwm_id → TIM+Channel）内置在 bsp_pwm.c 中，
 *       上层调用时只需传 pwm_id，无需知道具体的定时器句柄。
 *
 *       适用于舵机、直流电机、LED调光、蜂鸣器等所有PWM设备。
 */

#ifndef __BSP_PWM_H__
#define __BSP_PWM_H__

/*==============================================================================
 * 头文件包含
 *============================================================================*/
#include <tim.h>

/*==============================================================================
 * 宏定义与常量
 *============================================================================*/
#define TIM_APB2_Hz     168000000       // APB2定时器时钟（TIM9/10/11）
#define TIM_APB1_Hz     84000000        // APB1定时器时钟（TIM12/13/14）
#define TIM_MAX_SIZE      8             // PWM通道数量
#define BSP_PWM_TIM_FREQ_HZ  1000000U   // 定时器计数频率，1MHz → 1计数值=1μs
#define TIM_PSC_MAX          0xFFFFU   // PSC在所有STM32F4定时器上均为16位
#define TIM_ARR_MAX(htim)    (IS_TIM_32B_COUNTER_INSTANCE((htim)->Instance) ? 0xFFFFFFFFU : 0xFFFFU)

/*==============================================================================
 * 初始化函数
 *============================================================================*/
/**
 * @brief 初始化PWM BSP层
 */
void BSP_PWM_Init(void);

/**
 * @brief  设置PWM频率（采用 BSP_PWM_TIM_FREQ_HZ 计数频率）
 * @param  htim            定时器句柄
 * @param  TimerClockFreq  定时器时钟源频率，单位Hz
 * @param  DesiredFreq     目标PWM频率
 * @note   调用后脉宽映射变为：1次计数 = (1/BSP_PWM_TIM_FREQ_HZ)秒，之前设置的CCR值会失效，
 *         需立即重新设定各通道的脉宽。
 */
void BSP_PWM_SetFreq(TIM_HandleTypeDef *htim, uint32_t TimerClockFreq, uint32_t DesiredFreq);

/*==============================================================================
 * 定时器控制函数
 *============================================================================*/
/**
 * @brief 启动PWM输出（通过pwm_id查内置硬件映射表）
 * @param pwm_id 逻辑PWM编号
 */
void BSP_PWM_Start(uint8_t pwm_id);

/**
 * @brief 停止PWM输出
 * @param pwm_id 逻辑PWM编号
 */
void BSP_PWM_Stop(uint8_t pwm_id);

/**
 * @brief 重置定时器计数器
 * @param pwm_id 逻辑PWM编号
 */
void BSP_PWM_Reset(uint8_t pwm_id);

/*==============================================================================
 * 脉宽控制函数
 *============================================================================*/
/**
 * @brief 设置PWM脉宽（适用于舵机，直接输μs）
 * @param pwm_id   逻辑PWM编号
 * @param pulse_us 高电平脉宽，单位μs（典型范围500~2500）
 */
void BSP_PWM_SetPulseUs(uint8_t pwm_id, uint16_t pulse_us);

/**
 * @brief 设置PWM占空比（适用于电机、LED等）
 * @param pwm_id       逻辑PWM编号
 * @param duty_percent 占空比百分比（0.0~100.0）
 */
void BSP_PWM_SetDuty(uint8_t pwm_id, float duty_percent);

#endif /* __BSP_PWM_H__ */
