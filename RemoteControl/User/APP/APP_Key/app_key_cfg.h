/**
 * @file app_key_cfg.h
 * @brief 按键注册参数配置表
 * @author 李嘉图
 * @date 2026-06-17
 *
 * @note 将 task_key.c 中的按键注册参数提取到此处，
 *       Task 层通过循环遍历配置表完成注册。
 *
 *       GPIO 引脚映射已下沉到 BSP 层（bsp_gpio.c 的 GPIO_HW_MAP），
 *       配置表只保留逻辑参数：key_id、长按阈值、有效电平。
 */

#ifndef __APP_KEY_CFG_H__
#define __APP_KEY_CFG_H__

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include <stdint.h>

/*==============================================================================
 * 宏定义与常量
 *============================================================================*/
#define KEY_SCAN_PERIOD_MS 10

/*==============================================================================
 * 按键注册参数配置
 *============================================================================*/
typedef struct {
    uint8_t  key_id;           /* 逻辑按键 ID（分发表匹配用） */
    uint8_t  gpio_id;          /* GPIO 映射 ID（BSP_GPIO 寻址用） */
    uint8_t  long_press;       /* 长按判定阈值（单位:任务周期） */
    uint8_t  active_level;     /* 按键激活电平（0:低电平，1:高电平） */
    uint8_t  debounce_ticks;   /* 消抖稳定计数（单位:任务周期，推荐 2~3） */
} key_register_t;

/* 按键注册表：{key_id, gpio_id, long_press, active_level, debounce_ticks} */
static const key_register_t KEY_REG_TABLE[] = {
    // 按钮按键
    { .key_id = 1, .gpio_id = 1, .long_press = 18, .active_level = 1, .debounce_ticks = 2 },
    { .key_id = 2, .gpio_id = 2, .long_press = 18, .active_level = 1, .debounce_ticks = 2 },
    { .key_id = 3, .gpio_id = 3, .long_press = 18, .active_level = 1, .debounce_ticks = 2 },
    { .key_id = 4, .gpio_id = 4, .long_press = 18, .active_level = 1, .debounce_ticks = 2 },
    { .key_id = 5, .gpio_id = 5, .long_press = 18, .active_level = 1, .debounce_ticks = 2 },
    { .key_id = 6, .gpio_id = 6, .long_press = 18, .active_level = 1, .debounce_ticks = 2 },
};

#endif /* __APP_KEY_CFG_H__ */
