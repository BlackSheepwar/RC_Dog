/**
 * @file app_key.h
 * @brief 按键应用层
 * @author 李嘉图
 * @date 2026-06-17
 *
 * @note GPIO 引脚映射已下沉到 BSP 层（bsp_gpio.c 的 GPIO_HW_MAP），
 *       应用层注册按键时只需传逻辑参数，不再传 GPIOx/Pin。
 */

#ifndef __APP_KEY_H__
#define __APP_KEY_H__

/*==============================================================================
 * 头文件包含
 *============================================================================*/
#include "bsp_gpio.h"
#include "debounce.h"

/*==============================================================================
 * 初始化函数
 *============================================================================*/
/**
 * @brief 初始化
 */
void App_Key_Init(void);

/**
 * @brief 注册一个按键到静态资源池
 * @param key_id      按键ID（须与 GPIO_HW_MAP 的 gpio_id 一致）
 * @param long_press  长按判定阈值（单位:任务周期）
 * @param active_level 按键激活电平（0:低电平，1:高电平）
 * @retval 0: 参数错误或 gpio_id 无效
 * @retval 1: 成功
 * @retval 2: 资源池已满
 * @note GPIO 引脚映射由 BSP 层内置的 GPIO_HW_MAP 决定，无需传入
 */
uint8_t App_Key_Register(uint8_t key_id,
                         uint8_t long_press,
                         uint8_t active_level);

/*==============================================================================
 * 更新按键状态
 *============================================================================*/
/**
 * @brief 更新按键状态
 */
void App_Key_Update(void);

#endif
