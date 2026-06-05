/**
 * @file app_se.h
 * @brief 舵机控制
 * @author 李嘉图
 * @date 2026-5-12
 */

#ifndef __APP_SE_H__
#define __APP_SE_H__

/*==============================================================================
 * 头文件包含
 *============================================================================*/
#include "bsp_se.h"

/*==============================================================================
 * 宏定义与常量
 *============================================================================*/
typedef struct
{
    uint8_t id;              // 舵机ID

    int16_t offset;          // 舵机角度偏移量（-25°~25°）
    int16_t current_angle;   // 当前角度（-125°~125°）
    int16_t target_angle;    // 目标角度（-125°~125°）

    uint8_t speed;           // 每次调度变化步进（1°~10°）

    uint8_t reverse;         // 方向反转: 0=正转, 1=反转（物理装反时用）

    uint8_t enable;          // 是否启用
} APP_SE_t;

/*==============================================================================
 * 初始化函数
 *============================================================================*/
/**
 * @brief 初始化APP控制系统
 */
void APP_SE_Init(void);

/**
 * @brief 注册一个舵机实例到APP控制系统
 * @param id         舵机编号
 * @param htim       舵机定时器句柄
 * @param Channel    舵机通道
 * @param offset     舵机角度偏移量（-25°~25°）
 * @param init_angle 初始角度
 * @param speed      运动步进速度（1°~10°）
 * @param reverse    方向反转: 0=正转, 1=反转
 * @retval 1: 注册成功
 * @retval 0: 注册失败
 */
uint8_t APP_SE_Add(uint8_t id,  TIM_HandleTypeDef *htim,
uint32_t Channel, int16_t offset, int16_t init_angle, uint8_t speed, uint8_t reverse);

/**
 * @brief 设置舵机目标角度（使用查找接口）
 * @param id    舵机编号
 * @param angle 目标角度（0~300）
 * @retval 无
 */
void APP_SE_SetTarget(uint8_t id, int16_t angle);

/**
 * @brief 设置舵机目标角度数（增量）
 * @param id    舵机编号
 * @param angle 目标角度增量
 * @retval 无
 */
void APP_SE_SetincreaseTarget(uint8_t id, int16_t angle);

/**
 * @brief 获取舵机当前角度数
 * @param id    舵机编号
 * @retval 无
 */
int16_t APP_SE_GetCurrent(uint8_t id);

/**
 * @brief 设置舵机运动速度
 * @param id    舵机编号
 * @param speed 步进速度（1°~10°/次调度）
 * @retval 无
 */
void APP_SE_SetSpeed(uint8_t id, uint8_t speed);

/**
 * @brief 舵机平滑调度器（周期调用）
 * @note  建议5~20ms调用一次
 * @retval 无
 */
void APP_SE_Scheduler(void);

#endif