/**
 * @file bsp_se.h
 * @brief 定时器控制PWM输出
 * @author 李嘉图
 * @date 2026-5-12
 */

#ifndef __BSP_SE_H__
#define __BSP_SE_H__

/*==============================================================================
 * 头文件包含
 *============================================================================*/
#include <tim.h>

/*==============================================================================
 * 宏定义与常量
 *============================================================================*/
#define TIM_Hz          72000000     // 定时器时钟频率，单位：Hz
#define SE_MAX_SIZE     6            // 支持最大舵机数量

typedef struct{
    uint8_t            id;             // 舵机编号
    TIM_HandleTypeDef  *htim;          // 定时器句柄
    uint32_t           Channel;        // PWM通道
} BSP_SE_t;

/*==============================================================================
 * 初始化函数
 *============================================================================*/
 /**
 * @brief 初始化舵机定时器
 */
void BSP_SE_Init(void);
/**
 * @brief 注册一个 BSP 舵机实例
 * @note 初始化 FIFO 和 DMA 接收，使用结构体内部的 DMA 缓冲区
 * @param id        舵机编号（逻辑编号，不要求等于数组下标）
 * @param htim     TIM_HandleTypeDef句柄
 * @param Channel   PWM通道
 * @retval 1: 注册成功
 * @retval 0: 注册失败
 */
uint8_t BSP_SE_RegisterPort(uint8_t id, TIM_HandleTypeDef *htim, uint32_t Channel);

/*==============================================================================
 * 定时器控制函数
 *============================================================================*/
/**
 * @brief 舵机定时器重置计数器
 * @param  htim  定时器句柄
 */
void BSP_SE_Reset(TIM_HandleTypeDef *htim);

/**
 * @brief 启动舵机定时器
 */ 
void BSP_SE_Start(TIM_HandleTypeDef *htim, uint32_t Channel);

/**
 * @brief 停止舵机定时器
 */ 
void BSP_SE_Stop(TIM_HandleTypeDef *htim, uint32_t Channel);

/*==============================================================================
 * 舵机控制函数
 *============================================================================*/
/**
 * @brief  180°舵机角度控制
 * @param  id   舵机编号
 * @param  angle 角度（0~180）
 * @retval 无
 */
void BSP_SE_180Angle(uint8_t id, uint16_t angle);

/**
 * @brief  270°舵机角度控制
 * @param  id   舵机编号
 * @param  angle 角度（0~270）
 * @retval 无
 */
void BSP_SE_270Angle(uint8_t id, uint16_t angle);

/**
 * @brief  300°舵机角度控制
 * @param  id   舵机编号
 * @param  angle 角度（0~300）
 * @retval 无
 */
void BSP_SE_300Angle(uint8_t id, uint16_t angle);

#endif