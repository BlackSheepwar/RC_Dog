/**
 * @file bsp_encoder.h
 * @brief 正交编码器硬件驱动层：一路编码器一个句柄，只做计数读写
 * @author Ricardo
 * @date 2026-11-01
 *
 * @note 用定时器的 Encoder Mode（TI1+TI2）由硬件自动判向计数，
 *       本层只负责"读计数、清零计数"，不做速度换算——
 *       什么算一圈、一圈多少脉冲是机械参数，属于 APP_Motor。
 *
 *       定时器与方向取反写在 bsp_encoder.c 的句柄实例里。
 */

#ifndef __BSP_ENCODER_H__
#define __BSP_ENCODER_H__

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include <stdint.h>

/*==============================================================================
 * 类型定义
 *============================================================================*/
/**
 * @brief 编码器句柄（不透明类型，布局在 bsp_encoder.c）
 */
typedef struct bsp_encoder bsp_encoder_t;

/*==============================================================================
 * 外部声明：一路电机编码器一份句柄
 *============================================================================*/
/** @brief 电机 1 编码器（TIM1） */
extern const bsp_encoder_t bsp_encoder_1;
/** @brief 电机 2 编码器（TIM2） */
extern const bsp_encoder_t bsp_encoder_2;
/** @brief 电机 3 编码器（TIM3） */
extern const bsp_encoder_t bsp_encoder_3;
/** @brief 电机 4 编码器（TIM4） */
extern const bsp_encoder_t bsp_encoder_4;

/*==============================================================================
 * 初始化
 *============================================================================*/
/**
 * @brief 启动所有编码器定时器并把计数器清零
 * @retval 无
 * @note 每个定时器只启动一次；重复调用会把计数清零（相当于重新开始计步）。
 */
void BSP_Encoder_Init(void);

/*==============================================================================
 * 读数
 *============================================================================*/
/**
 * @brief 读取本周期的计数增量并清零计数器（速度环用）
 * @param enc   编码器句柄，不可为 NULL
 * @param delta 输出增量，不可为 NULL；已按句柄的取反配置换算好符号
 * @retval 0: 读取成功
 * @retval -1: 句柄或输出指针为 NULL
 * @note 读与清不是原子的：清零前若又来了脉冲，这个脉冲会算进下一周期，
 *       不会丢，但也可能让本周期少 1。速度环 10 ms 一次，影响可忽略。
 */
int BSP_Encoder_Read_Delta(const bsp_encoder_t *enc, int16_t *delta);

/**
 * @brief 读取当前累计计数但不清零（标定线数用）
 * @param enc 编码器句柄，不可为 NULL
 * @param raw 输出累计计数，不可为 NULL
 * @retval 0: 读取成功
 * @retval -1: 句柄或输出指针为 NULL
 */
int BSP_Encoder_Read_Raw(const bsp_encoder_t *enc, int16_t *raw);

/**
 * @brief 把计数器清零
 * @param enc 编码器句柄，可为 NULL
 * @retval 无
 * @note 标定前手动归零用；不影响编码器模式的运行状态。
 */
void BSP_Encoder_Reset(const bsp_encoder_t *enc);

#endif /* __BSP_ENCODER_H__ */
