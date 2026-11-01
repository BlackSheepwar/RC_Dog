/**
 * @file bsp_gpio.h
 * @brief GPIO 硬件驱动层：一个引脚一个句柄，只做电平读写
 * @author Ricardo
 * @date 2026-11-01
 *
 * @note 本层不知道引脚接的是什么（按键？灯？继电器？），只知道"第几号引脚、
 *       输出高还是低"。业务语义在 Device 层（Dev_Key / Dev_LED）。
 *
 *       方向、上下拉、初值全部由 CubeMX 生成的 MX_GPIO_Init() 配置，
 *       本层只负责把句柄翻译成 GPIOx + Pin，不重复初始化硬件。
 *
 *       句柄里的 port 故意用 void *：HAL 的 GPIO_TypeDef 是匿名 struct 的
 *       typedef，没法前置声明，一旦写进头文件，stm32h7xx_hal.h 就会顺着
 *       Device / Service / APP 的头文件链一路传染上去。
 *       真正的类型转换只发生在 bsp_gpio.c 内部。
 *
 *       引脚号只出现在 bsp_gpio.c：换板子只改那一个文件，上层一行不动。
 */

#ifndef __BSP_GPIO_H__
#define __BSP_GPIO_H__

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include <stdint.h>

/*==============================================================================
 * 类型定义
 *============================================================================*/
/**
 * @brief 引脚方向
 * @note STM32 的方向由 MX_GPIO_Init() 定死，只有 IN / OUT 两种；
 *       IN_OUT 是给"运行时可换向"的 MCU（如 ESP32）留的位置，本板不用。
 */
typedef enum {
    BSP_GPIO_DIR_IN     = 0,    /* 输入：只读，写它会被忽略 */
    BSP_GPIO_DIR_OUT    = 1,    /* 输出：可写，也可读回引脚实际电平 */
    BSP_GPIO_DIR_IN_OUT = 2,    /* 双向：输入输出可运行时切换 */
} bsp_gpio_dir_t;

/**
 * @brief GPIO 引脚句柄（不透明类型，成员定义在 bsp_gpio.c）
 */
typedef struct bsp_gpio bsp_gpio_t;

/*==============================================================================
 * 外部声明：每个用到的引脚一份句柄
 *
 * 上层拿到的是不完整类型：能取地址、能传指针，但取不了成员也 sizeof 不了。
 *============================================================================*/
/** @brief 板载按键（PC13，输入，CubeMX 已配下拉） */
extern const bsp_gpio_t bsp_gpio_pc13;
/** @brief 板载指示灯（PE3，输出，低电平点亮由 Dev_LED 负责换算） */
extern const bsp_gpio_t bsp_gpio_pe3;

/*==============================================================================
 * 接口
 *============================================================================*/
/**
 * @brief 读取引脚电平
 * @param gpio 引脚句柄，可为 NULL
 * @retval 1: 高电平
 * @retval 0: 低电平，或句柄为 NULL
 * @note 输出引脚读回来的是引脚实际电平（可能被外部拉低），不是 ODR 的值。
 */
uint8_t BSP_GPIO_Read(const bsp_gpio_t *gpio);

/**
 * @brief 设置引脚输出电平
 * @param gpio  引脚句柄，可为 NULL
 * @param level 1 为高，0 为低
 * @retval 无
 * @note 方向为 BSP_GPIO_DIR_IN 的句柄会被直接忽略：
 *       写输入不会产生任何电平变化，悄悄"成功"会让上层以为灯亮了。
 */
void BSP_GPIO_Write(const bsp_gpio_t *gpio, uint8_t level);

/**
 * @brief 翻转引脚输出电平
 * @param gpio 引脚句柄，可为 NULL
 * @retval 无
 * @note 读 ODR 再写，只适用于单写者场景；输入句柄同样被忽略。
 */
void BSP_GPIO_Toggle(const bsp_gpio_t *gpio);

#endif /* __BSP_GPIO_H__ */
