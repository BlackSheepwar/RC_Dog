/**
 * @file se_cfg.h
 * @brief 舵机配置表（前腿/后腿分离）
 * @author 李嘉图
 * @date 2026-06-05
 *
 * @note 将 task_se.c 中的舵机注册参数和定时器频率配置提取到此处，
 *       通过 HW_VERSION 编译宏自动选择前腿或后腿配置。
 *       Task 层只需循环遍历配置表完成初始化，不再硬编码具体参数。
 */

#ifndef __SE_CFG_H__
#define __SE_CFG_H__

/*==============================================================================
 * 头文件包含
 *============================================================================*/
#include "main.h"       // 引用 &htim9~14 句柄
#include "bsp_se.h"     // 引用 TIM_APB1_Hz / TIM_APB2_Hz

/*==============================================================================
 * 配置数量
 *============================================================================*/
#define SE_TIM_CFG_COUNT    5   // 需配置的定时器数量（除TIM11）
#define SE_SERVO_COUNT      6   // 每块MCU控制的舵机数量

/*==============================================================================
 * 定时器频率配置
 *============================================================================*/
typedef struct {
    TIM_HandleTypeDef *htim;
    uint32_t           clock_hz;   // 定时器总线时钟（APB1/APB2）
    uint32_t           freq;       // 目标PWM频率
} se_tim_cfg_t;

/*==============================================================================
 * 舵机注册参数配置
 *============================================================================*/
typedef struct {
    uint8_t           id;
    TIM_HandleTypeDef *htim;
    uint32_t          channel;
    int16_t           offset;       // 舵机角度偏移量（-25°~25°）
    int16_t           init_angle;   // 初始角度
    uint8_t           speed;        // 运动步进速度（1°~10°）
    uint8_t           reverse;      // 方向反转: 0=正转, 1=反转
} se_servo_cfg_t;

/*==============================================================================
 * 前腿配置（MCU1）：舵机ID 1-6，左腿=1/2/3，右腿=4/5/6
 *============================================================================*/
#if HW_VERSION == HW_MCU_FRONT

static const se_tim_cfg_t SE_TIM_CFG[SE_TIM_CFG_COUNT] = {
    { .htim = &htim9,  .clock_hz = TIM_APB2_Hz, .freq = 330 },
    { .htim = &htim10, .clock_hz = TIM_APB2_Hz, .freq = 330 },
    // TIM11 不使用
    { .htim = &htim12, .clock_hz = TIM_APB1_Hz, .freq = 330 },
    { .htim = &htim13, .clock_hz = TIM_APB1_Hz, .freq = 330 },
    { .htim = &htim14, .clock_hz = TIM_APB1_Hz, .freq = 330 },
};

static const se_servo_cfg_t SE_SERVO_CFG[SE_SERVO_COUNT] = {
    { .id = 1,  .htim = &htim12, .channel = TIM_CHANNEL_2, .offset = 20,  .init_angle = 0, .speed = 3, .reverse = 1 },
    { .id = 2,  .htim = &htim12, .channel = TIM_CHANNEL_1, .offset = 16,  .init_angle = 0, .speed = 3, .reverse = 1 },
    { .id = 3,  .htim = &htim10, .channel = TIM_CHANNEL_1, .offset = 15,  .init_angle = 0, .speed = 3, .reverse = 1 },
    { .id = 4,  .htim = &htim14, .channel = TIM_CHANNEL_1, .offset = 8,   .init_angle = 0, .speed = 3, .reverse = 0 },
    { .id = 5,  .htim = &htim13, .channel = TIM_CHANNEL_1, .offset = 21,  .init_angle = 0, .speed = 3, .reverse = 0 },
    { .id = 6,  .htim = &htim9,  .channel = TIM_CHANNEL_2, .offset = 12,  .init_angle = 0, .speed = 3, .reverse = 0 },
};

/*==============================================================================
 * 后腿配置（MCU2）：舵机ID 7-12，左腿=7/8/9，右腿=10/11/12
 *============================================================================*/
#else

static const se_tim_cfg_t SE_TIM_CFG[SE_TIM_CFG_COUNT] = {
    { .htim = &htim9,  .clock_hz = TIM_APB2_Hz, .freq = 330 },
    { .htim = &htim10, .clock_hz = TIM_APB2_Hz, .freq = 330 },
    // TIM11 不使用
    { .htim = &htim12, .clock_hz = TIM_APB1_Hz, .freq = 330 },
    { .htim = &htim13, .clock_hz = TIM_APB1_Hz, .freq = 330 },
    { .htim = &htim14, .clock_hz = TIM_APB1_Hz, .freq = 330 },
};

static const se_servo_cfg_t SE_SERVO_CFG[SE_SERVO_COUNT] = {
    { .id = 7,  .htim = &htim12, .channel = TIM_CHANNEL_2, .offset = 20,  .init_angle = 0, .speed = 3, .reverse = 1 },
    { .id = 8,  .htim = &htim12, .channel = TIM_CHANNEL_1, .offset = -12, .init_angle = 0, .speed = 3, .reverse = 1 },
    { .id = 9,  .htim = &htim10, .channel = TIM_CHANNEL_1, .offset = 14,  .init_angle = 0, .speed = 3, .reverse = 1 },
    { .id = 10, .htim = &htim14, .channel = TIM_CHANNEL_1, .offset = 9,   .init_angle = 0, .speed = 3, .reverse = 0 },
    { .id = 11, .htim = &htim13, .channel = TIM_CHANNEL_1, .offset = 0,   .init_angle = 0, .speed = 3, .reverse = 0 },
    { .id = 12, .htim = &htim9,  .channel = TIM_CHANNEL_2, .offset = 10,  .init_angle = 0, .speed = 3, .reverse = 0 },
};

#endif /* HW_VERSION */

#endif /* __SE_CFG_H__ */
