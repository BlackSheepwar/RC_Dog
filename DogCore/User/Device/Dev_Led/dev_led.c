/**
 * @file dev_led.c
 * @brief 指示灯设备层实现：逻辑亮灭 → 引脚电平
 * @author Ricardo
 * @date 2026-11-01
 *
 * @note 只做极性换算与 BSP 调用，不含任何闪烁/呼吸逻辑——
 *       需要时间轴的花样（闪烁、呼吸）是 Svc_Led_Effect 的活，
 *       本层保持"给什么就亮什么"。
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include <stddef.h>
#include "dev_led.h"
#include "bsp_gpio.h"           /* BSP 类型只在这里出现 */

/*==============================================================================
 * 类型定义
 *============================================================================*/
/**
 * @brief 指示灯句柄（一颗灯一份）
 */
struct dev_led {
    const bsp_gpio_t *pin;      /* 引脚句柄，引脚号在 BSP 里 */
    uint8_t           active_low; /* 1: 低电平点亮 */
};

/*==============================================================================
 * 句柄实例
 *============================================================================*/
/** @brief 板载指示灯 PE3：高电平点亮 */
const dev_led_t dev_led_0 = {
    .pin        = &bsp_gpio_pe3,
    .active_low = 0u,
};

/*==============================================================================
 * 接口实现
 *============================================================================*/
/**
 * @brief 设置灯的亮灭
 * @param led 灯句柄，可为 NULL
 * @param on  1 点亮，0 熄灭
 * @retval 无
 */
void Dev_LED_Set(const dev_led_t *led, uint8_t on)
{
    uint8_t level;

    if (led == NULL) {
        return;
    }

    level = (on != 0u) ? 1u : 0u;
    if (led->active_low != 0u) {
        level = (level != 0u) ? 0u : 1u;
    }

    BSP_GPIO_Write(led->pin, level);
}

/**
 * @brief 翻转灯的亮灭
 * @param led 灯句柄，可为 NULL
 * @retval 无
 */
void Dev_LED_Toggle(const dev_led_t *led)
{
    if (led == NULL) {
        return;
    }

    BSP_GPIO_Toggle(led->pin);
}

/**
 * @brief 查询灯是否处于点亮状态
 * @param led 灯句柄，可为 NULL
 * @retval 1: 点亮
 * @retval 0: 熄灭，或句柄为 NULL
 */
uint8_t Dev_LED_Is_On(const dev_led_t *led)
{
    uint8_t level;

    if (led == NULL) {
        return 0u;
    }

    level = BSP_GPIO_Read(led->pin);

    return (led->active_low != 0u) ? ((level == 0u) ? 1u : 0u)
                                   : ((level != 0u) ? 1u : 0u);
}
