/**
 * @file app_key_cfg.h
 * @brief 按键注册参数配置表
 * @author 李嘉图
 * @date 2026-06-05
 *
 * @note 将 task_key.c 中的按键注册参数提取到此处，
 *       Task 层通过循环遍历配置表完成注册，不再硬编码具体引脚。
 */

#ifndef __APP_KEY_CFG_H__
#define __APP_KEY_CFG_H__

/*==============================================================================
 * 头文件包含
 *============================================================================*/
#include "main.h"

/*==============================================================================
 * 按键注册参数配置
 *============================================================================*/
typedef struct {
    uint8_t       key_id;
    uint8_t       long_press;
    uint8_t       active_level;
    GPIO_TypeDef *GPIOx;
    uint16_t      Pin;
} key_register_t;

/* 按键注册表：{id, long_press, active_level, GPIOx, Pin} */
static const key_register_t KEY_REG_TABLE[] = {
    { .key_id = 1, .long_press = 18, .active_level = 1, .GPIOx = GPIOC, .Pin = GPIO_PIN_13 },
};

#endif /* __APP_KEY_CFG_H__ */
