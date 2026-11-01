/**
 * @file bsp_gpio.c
 * @brief GPIO 硬件驱动层实现：把句柄翻译成 HAL 的端口与引脚
 * @author Ricardo
 * @date 2026-11-01
 *
 * @note 本文件只做三件事：判空、把句柄字段喂给 HAL、返回结果。
 *       引脚的方向与上下拉由 CubeMX 生成的 MX_GPIO_Init() 配置，
 *       这里不重复初始化硬件，也没有 Init 函数可调。
 *
 *       头文件里端口是不透明的 void *，真正的 GPIO_TypeDef * 只在本文件
 *       还原，HAL 因此不会顺着 bsp_gpio.h 泄漏到 Device / APP。
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include <stddef.h>
#include "main.h"               /* HAL 与 GPIO_TypeDef / GPIO_PIN_x */
#include "bsp_gpio.h"

/*==============================================================================
 * 类型定义
 *============================================================================*/
/**
 * @brief GPIO 句柄（一个引脚一份）
 * @note 端口、引脚号、方向三个字段；上下拉属于硬件配置，不进句柄。
 */
struct bsp_gpio {
    void          *port;        /* 端口基址（本文件内还原成 GPIO_TypeDef *） */
    uint16_t       pin;         /* 引脚掩码 GPIO_PIN_x */
    bsp_gpio_dir_t dir;         /* 方向，见 bsp_gpio_dir_t */
};

/*==============================================================================
 * 句柄实例：端口与引脚号只出现在这里
 *============================================================================*/
/** @brief 板载按键 PC13：输入，CubeMX 里配了下拉，按下为高 */
const bsp_gpio_t bsp_gpio_pc13 = {
    .port = GPIOC,
    .pin  = GPIO_PIN_13,
    .dir  = BSP_GPIO_DIR_IN,
};

/** @brief 板载指示灯 PE3：输出 */
const bsp_gpio_t bsp_gpio_pe3 = {
    .port = GPIOE,
    .pin  = GPIO_PIN_3,
    .dir  = BSP_GPIO_DIR_OUT,
};

/*==============================================================================
 * 接口实现
 *============================================================================*/
/**
 * @brief 读取引脚电平
 * @param gpio 引脚句柄，可为 NULL
 * @retval 1: 高电平
 * @retval 0: 低电平，或句柄为 NULL
 */
uint8_t BSP_GPIO_Read(const bsp_gpio_t *gpio)
{
    GPIO_TypeDef *port;

    if (gpio == NULL) {
        return 0u;
    }

    port = (GPIO_TypeDef *)gpio->port;
    if (port == NULL) {
        return 0u;
    }

    return (HAL_GPIO_ReadPin(port, gpio->pin) == GPIO_PIN_SET) ? 1u : 0u;
}

/**
 * @brief 设置引脚输出电平
 * @param gpio  引脚句柄，可为 NULL
 * @param level 1 为高，0 为低
 * @retval 无
 * @note 输入句柄直接挡掉，不调用 HAL。
 */
void BSP_GPIO_Write(const bsp_gpio_t *gpio, uint8_t level)
{
    GPIO_TypeDef *port;

    if ((gpio == NULL) || (gpio->dir == BSP_GPIO_DIR_IN)) {
        return;
    }

    port = (GPIO_TypeDef *)gpio->port;
    if (port == NULL) {
        return;
    }

    HAL_GPIO_WritePin(port, gpio->pin,
                      (level != 0u) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/**
 * @brief 翻转引脚输出电平
 * @param gpio 引脚句柄，可为 NULL
 * @retval 无
 * @note 输入句柄直接挡掉，不调用 HAL。
 */
void BSP_GPIO_Toggle(const bsp_gpio_t *gpio)
{
    GPIO_TypeDef *port;

    if ((gpio == NULL) || (gpio->dir == BSP_GPIO_DIR_IN)) {
        return;
    }

    port = (GPIO_TypeDef *)gpio->port;
    if (port == NULL) {
        return;
    }

    HAL_GPIO_TogglePin(port, gpio->pin);
}
