/**
 * @file dev_key.c
 * @brief 按键设备层实现：电平 + 极性 → 逻辑按下状态
 * @author Ricardo
 * @date 2026-11-01
 *
 * @note 只调用 BSP_GPIO_Read，不做消抖、不做延时、不判长短按。
 *       极性（active_low）写在句柄实例里，换按键接法只改这一处。
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include <stddef.h>
#include "common.h"             /* ARRAY_SIZE */
#include "dev_key.h"
#include "bsp_gpio.h"           /* BSP 类型只在这里出现 */

/*==============================================================================
 * 类型定义
 *============================================================================*/
/**
 * @brief 按键句柄（一个按键一份）
 */
struct dev_key {
    const bsp_gpio_t *pin;      /* 引脚句柄，引脚号在 BSP 里 */
    uint8_t           active_low; /* 1: 按下为低电平；0: 按下为高电平 */
};

/*==============================================================================
 * 句柄实例
 *============================================================================*/
/** @brief 板载按键 PC13：CubeMX 配下拉，按下读到高电平 */
const dev_key_t dev_key_1 = {
    .pin        = &bsp_gpio_pc13,
    .active_low = 0u,
};

/*==============================================================================
 * 接口实现
 *============================================================================*/
/**
 * @brief 按键设备层初始化（校验极性配置）
 * @retval 0: 成功
 * @retval -1: 句柄表里有空指针
 */
int Dev_Key_Init(void)
{
    static const dev_key_t *const s_all[] = { &dev_key_1 };

    for (uint8_t i = 0u; i < ARRAY_SIZE(s_all); i++) {
        if (s_all[i] == NULL) {
            return -1;
        }
    }

    return 0;
}

/**
 * @brief 判断按键当前是否按下
 * @param key 按键句柄，可为 NULL
 * @retval 1: 已按下
 * @retval 0: 未按下，或句柄为 NULL
 */
uint8_t Dev_Key_Is_Pressed(const dev_key_t *key)
{
    uint8_t level;

    if (key == NULL) {
        return 0u;
    }

    level = BSP_GPIO_Read(key->pin);

    return (key->active_low != 0u) ? ((level == 0u) ? 1u : 0u)
                                   : ((level != 0u) ? 1u : 0u);
}
