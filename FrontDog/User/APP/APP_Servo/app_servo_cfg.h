/**
 * @file app_servo_cfg.h
 * @brief 舵机配置表（前腿/后腿分离）
 * @author 李嘉图
 * @date 2026-06-05
 *
 * @note 将 task_servo.c 中的舵机注册参数和定时器频率配置提取到此处，
 *       通过 HW_VERSION 编译宏自动选择前腿或后腿配置。
 *       Task 层只需循环遍历配置表完成初始化，不再硬编码具体参数。
 */

#ifndef __APP_SERVO_CFG_H__
#define __APP_SERVO_CFG_H__

/*==============================================================================
 * 头文件包含
 *============================================================================*/
#include "main.h"       // 引用 &htim9~14 句柄
#include "bsp_pwm.h"    // 引用 TIM_APB1_Hz / TIM_APB2_Hz
#include "app_servo.h"  // 引用 Servo_HwConfig_t

/*==============================================================================
 * 舵机注册参数配置
 *
 * 注意：htim/Channel 不在这里配置，硬件映射由 BSP 层内置。
 *       新增舵机需同时在 bsp_pwm.c 的 PWM_HW_MAP 中添加映射。
 *============================================================================*/
typedef struct {
    uint8_t           id;
    Servo_HwConfig_t  hw;         // 硬件配置（安装后固定，含 offset/reverse/limit/phys_range）
    float             speed_dps;  // 角速度 (°/s)，如 300.0f = 300°/s
} servo_cfg_t;

/*==============================================================================
 * 舵机配置表
 *============================================================================*/

static const servo_cfg_t SERVO_CFG[] = {
    // { .id = 1,  .hw = { .phys_range = 300, .offset = 7, .offset_max = 20, 
    //      .limit_min = -130, .limit_max = 130, .init_angle = 0, .reverse = 0, 
    //      .pulse_min = 500, .pulse_max = 2500 }, .speed_dps = 50.0f },

    // { .id = 2,  .hw = { .phys_range = 300, .offset = 3, .offset_max = 20, 
    //      .limit_min = -130, .limit_max = 130, .init_angle = 0, .reverse = 0, 
    //      .pulse_min = 500, .pulse_max = 2500 }, .speed_dps = 125.0f },

    // { .id = 3,  .hw = { .phys_range = 300, .offset = -11, .offset_max = 20,
    //      .limit_min = -130, .limit_max = 130, .init_angle = 0, .reverse = 1,
    //       .pulse_min = 500, .pulse_max = 2500 }, .speed_dps = 50.0f },

    // { .id = 4,  .hw = { .phys_range = 300, .offset = -15, .offset_max = 20,
    //      .limit_min = -130, .limit_max = 130, .init_angle = 0, .reverse = 1,
    //       .pulse_min = 500, .pulse_max = 2500 }, .speed_dps = 125.0f },

    // { .id = 5,  .hw = { .phys_range = 300, .offset = 12, .offset_max = 20, 
    //      .limit_min = -130, .limit_max = 130, .init_angle = 0, .reverse = 0, 
    //      .pulse_min = 500, .pulse_max = 2500 }, .speed_dps = 50.0f },

    // { .id = 6,  .hw = { .phys_range = 300, .offset = 13, .offset_max = 20, 
    //      .limit_min = -130, .limit_max = 130, .init_angle = 0, .reverse = 0, 
    //      .pulse_min = 500, .pulse_max = 2500 }, .speed_dps = 125.0f },

    // { .id = 7,  .hw = { .phys_range = 300, .offset = -5, .offset_max = 20,
    //      .limit_min = -130, .limit_max = 130, .init_angle = 0, .reverse = 1,
    //       .pulse_min = 500, .pulse_max = 2500 }, .speed_dps = 50.0f },

    // { .id = 8,  .hw = { .phys_range = 300, .offset = -3, .offset_max = 20,
    //      .limit_min = -130, .limit_max = 130, .init_angle = 0, .reverse = 1,
    //       .pulse_min = 500, .pulse_max = 2500 }, .speed_dps = 125.0f },

    { .id = 1,  .hw = { .phys_range = 300, .offset = 0, .offset_max = 20, 
        .limit_min = -130, .limit_max = 130, .init_angle = 0, .reverse = 0, 
        .pulse_min = 500, .pulse_max = 2500 }, .speed_dps = 75.0f },
    { .id = 2,  .hw = { .phys_range = 300, .offset = 0, .offset_max = 20,
         .limit_min = -130, .limit_max = 130, .init_angle = 0, .reverse = 0, 
        .pulse_min = 500, .pulse_max = 2500 }, .speed_dps = 75.0f },
};

#endif /* __APP_SERVO_CFG_H__ */