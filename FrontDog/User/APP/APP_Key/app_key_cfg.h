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
 * 宏定义与常量
 *============================================================================*/
#define KEY_SCAN_PERIOD_MS 10

/*==============================================================================
 * 按键注册参数配置
 *============================================================================*/
typedef struct {
    uint8_t  key_id;
    uint8_t  long_press;     /* 长按判定阈值（单位:任务周期） */
    uint8_t  active_level;   /* 按键激活电平（0:低电平，1:高电平） */
} key_register_t;

/* 按键注册表：{id, long_press, active_level} */
static const key_register_t KEY_REG_TABLE[] = {
    { .key_id = 1, .long_press = 18, .active_level = 1 },
};

#endif /* __APP_KEY_CFG_H__ */
