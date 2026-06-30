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
// 功能包含
#include "app_servo.h"  // 引用 Servo_HwConfig_t

/*==============================================================================
 * 舵机注册参数配置
 *
 * 注意：htim/Channel 不在这里配置，硬件映射由 BSP 层内置。
 *       新增舵机需同时在 bsp_pwm.c 的 PWM_HW_MAP 中添加映射。
 *============================================================================*/
typedef struct {
    uint8_t           servo_id;   /**< 逻辑舵机 ID（分发表匹配用） */
    uint8_t           pwm_id;     /**< PWM 映射 ID（BSP_PWM 寻址用） */
    Servo_HwConfig_t  hw;         // 硬件配置（安装后固定，含 offset/reverse/limit/phys_range）
    float             speed_dps;  // 角速度 (°/s)，如 200.0f = 200°/s
} servo_cfg_t;

/*==============================================================================
 * 舵机配置表
 *============================================================================*/

static const servo_cfg_t SERVO_CFG[] = {
    { .servo_id = 1, .pwm_id = 1,                 // ID
          .hw = { .phys_range = 300,                  // 物理范围 (°)
          .offset = +2, .offset_max = 20,              // 偏移量 (°)
          .limit_min = -130, .limit_max = 130,        // 限制范围 (°)
          .init_angle = 0, .reverse = 0,              // 初始角度 (°)
          .pulse_min = 500, .pulse_max = 2500 },      // 脉冲范围 (ms)
          .speed_dps = 200.0f },                      // 初始角速度 (°/s) 

    { .servo_id = 2, .pwm_id = 2,                 // ID
          .hw = { .phys_range = 300,                  // 物理范围 (°)
          .offset = 5, .offset_max = 20,              // 偏移量 (°)
          .limit_min = -130, .limit_max = 130,        // 限制范围 (°)
          .init_angle = 90, .reverse = 0,              // 初始角度 (°)
          .pulse_min = 500, .pulse_max = 2500 },      // 脉冲范围 (ms)
          .speed_dps = 200.0f },                      // 初始角速度 (°/s) 

    { .servo_id = 3, .pwm_id = 3,                 // ID
          .hw = { .phys_range = 300,                  // 物理范围 (°)
          .offset = 0, .offset_max = 20,              // 偏移量 (°)
          .limit_min = -130, .limit_max = 130,        // 限制范围 (°)
          .init_angle = 45, .reverse = 0,              // 初始角度 (°)
          .pulse_min = 500, .pulse_max = 2500 },      // 脉冲范围 (ms)
          .speed_dps = 200.0f },                      // 初始角速度 (°/s) 

    { .servo_id = 4, .pwm_id = 4,                 // ID
          .hw = { .phys_range = 300,                  // 物理范围 (°)
          .offset = 0, .offset_max = 20,              // 偏移量 (°)
          .limit_min = -130, .limit_max = 130,        // 限制范围 (°)
          .init_angle = 0, .reverse = 0,              // 初始角度 (°)
          .pulse_min = 500, .pulse_max = 2500 },      // 脉冲范围 (ms)
          .speed_dps = 200.0f },                      // 初始角速度 (°/s) 

    { .servo_id = 5, .pwm_id = 5,                 // ID
          .hw = { .phys_range = 300,                  // 物理范围 (°)
          .offset = 0, .offset_max = 20,              // 偏移量 (°)
          .limit_min = -130, .limit_max = 130,        // 限制范围 (°)
          .init_angle = 0, .reverse = 0,              // 初始角度 (°)
          .pulse_min = 500, .pulse_max = 2500 },      // 脉冲范围 (ms)
          .speed_dps = 200.0f },                      // 初始角速度 (°/s) 

    { .servo_id = 6, .pwm_id = 6,                 // ID
          .hw = { .phys_range = 300,                  // 物理范围 (°)
          .offset = 0, .offset_max = 20,              // 偏移量 (°)
          .limit_min = -130, .limit_max = 130,        // 限制范围 (°)
          .init_angle = 0, .reverse = 0,              // 初始角度 (°)
          .pulse_min = 500, .pulse_max = 2500 },      // 脉冲范围 (ms)
          .speed_dps = 200.0f },                      // 初始角速度 (°/s) 

    { .servo_id = 7, .pwm_id = 7,                 // ID
          .hw = { .phys_range = 300,                  // 物理范围 (°)
          .offset = 0, .offset_max = 20,              // 偏移量 (°)
          .limit_min = -130, .limit_max = 130,        // 限制范围 (°)
          .init_angle = 0, .reverse = 0,              // 初始角度 (°)
          .pulse_min = 500, .pulse_max = 2500 },      // 脉冲范围 (ms)
          .speed_dps = 200.0f },                      // 初始角速度 (°/s) 

    { .servo_id = 8, .pwm_id = 8,                 // ID
          .hw = { .phys_range = 300,                  // 物理范围 (°)
          .offset = 0, .offset_max = 20,              // 偏移量 (°)
          .limit_min = -130, .limit_max = 130,        // 限制范围 (°)
          .init_angle = 0, .reverse = 0,              // 初始角度 (°)
          .pulse_min = 500, .pulse_max = 2500 },      // 脉冲范围 (ms)
          .speed_dps = 200.0f },                      // 初始角速度 (°/s) 
};

#endif /* __APP_SERVO_CFG_H__ */