/**
 * @file app_led.c
 * @brief LED 应用层实现（静态注册表驱动）
 * @author 李嘉图
 * @date 2026-07-10
 *
 * @note LED 通过静态注册表 LED_REG_TABLE 管理，led_id → gpio_id 的映射
 *       在此表中定义。所有操作函数通过查表找到 gpio_id 后调用 BSP_GPIO 接口。
 *
 *       硬件引脚映射已下沉到 BSP 层（bsp_gpio.c 的 GPIO_HW_MAP），
 *       本模块不感知具体 GPIO 端口和引脚。
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include <stdint.h>
#include "main.h"
#include "app_led.h"
#include "common.h"
// 功能包含
#include "bsp_gpio.h"

/*==============================================================================
 * LED 注册表
 *============================================================================*/
static const led_register_t LED_REG_TABLE[] = {
    { .led_id = 0, .gpio_id = 0, .active_level = GPIO_LEVEL_LOW },   
    { .led_id = 1, .gpio_id = 13, .active_level = GPIO_LEVEL_HIGH },
    { .led_id = 2, .gpio_id = 14, .active_level = GPIO_LEVEL_HIGH },
    { .led_id = 3, .gpio_id = 15, .active_level = GPIO_LEVEL_HIGH },
    { .led_id = 4, .gpio_id = 16, .active_level = GPIO_LEVEL_HIGH },
    { .led_id = 5, .gpio_id = 17, .active_level = GPIO_LEVEL_HIGH },
    { .led_id = 6, .gpio_id = 18, .active_level = GPIO_LEVEL_HIGH },
};

/*==============================================================================
 * 内部函数
 *============================================================================*/
/**
 * @brief 根据 led_id 查找注册表项
 * @param led_id 逻辑 LED ID
 * @retval 非 NULL：找到
 * @retval NULL：未找到
 */
static const led_register_t *App_LED_FindById(uint8_t led_id)
{
    for (uint8_t i = 0; i < ARRAY_SIZE(LED_REG_TABLE); i++)
    {
        if (LED_REG_TABLE[i].led_id == led_id)
            return &LED_REG_TABLE[i];
    }
    return NULL;
}

/*==============================================================================
 * 初始化函数
 *============================================================================*/
/**
 * @brief LED 模块初始化
 * @note 遍历 LED_REG_TABLE，将所有 LED 设为熄灭状态
 */
void App_LED_Init(void)
{
    for (uint8_t i = 0; i < ARRAY_SIZE(LED_REG_TABLE); i++)
    {
        /* 初始化为熄灭状态（点亮电平的反相） */
        uint8_t off_level = (LED_REG_TABLE[i].active_level == GPIO_LEVEL_HIGH)
                          ? GPIO_LEVEL_LOW : GPIO_LEVEL_HIGH;
        BSP_GPIO_SetLevel(LED_REG_TABLE[i].gpio_id, off_level);
    }
}

/*==============================================================================
 * 控制函数
 *============================================================================*/
/**
 * @brief LED 亮灭控制
 * @param led_id 逻辑 LED ID
 * @param cmd    控制命令：LED_ON 点亮 / LED_OFF 熄灭
 *               （内部根据注册表中的 active_level 自动映射硬件极性）
 * @retval 1: 成功
 * @retval 0: 失败（led_id 无效）
 */
uint8_t App_LED_Control(uint8_t led_id, uint8_t cmd)
{
    const led_register_t *led = App_LED_FindById(led_id);
    if (led == NULL)
        return 0;

    if (cmd == LED_ON)
        return BSP_GPIO_SetLevel(led->gpio_id, led->active_level);
    else
    {
        uint8_t off_level = (led->active_level == GPIO_LEVEL_HIGH)
                          ? GPIO_LEVEL_LOW : GPIO_LEVEL_HIGH;
        return BSP_GPIO_SetLevel(led->gpio_id, off_level);
    }
}

/**
 * @brief LED 批量亮灭控制
 * @param led_ids  LED ID 数组指针
 * @param count   数组长度
 * @param cmd     控制命令：LED_ON 点亮 / LED_OFF 熄灭
 * @retval 1: 全部成功
 * @retval 0: 存在失败（某个 led_id 无效）
 */
uint8_t App_LED_ControlBatch(const uint8_t *led_ids, uint8_t count, uint8_t cmd)
{
    uint8_t ret = 1;

    for (uint8_t i = 0; i < count; i++)
    {
        if (!App_LED_Control(led_ids[i], cmd))
            ret = 0;
    }

    return ret;
}

/**
 * @brief LED 批量翻转
 * @param led_ids  LED ID 数组指针
 * @param count   数组长度
 * @retval 1: 全部成功
 * @retval 0: 存在失败（某个 led_id 无效）
 */
uint8_t App_LED_ToggleBatch(const uint8_t *led_ids, uint8_t count)
{
    uint8_t ret = 1;

    for (uint8_t i = 0; i < count; i++)
    {
        if (!App_LED_Toggle(led_ids[i]))
            ret = 0;
    }

    return ret;
}

/**
 * @brief 翻转指定 LED 状态
 * @param led_id 逻辑 LED ID
 * @retval 1: 成功
 * @retval 0: 失败（led_id 无效）
 */
uint8_t App_LED_Toggle(uint8_t led_id)
{
    const led_register_t *led = App_LED_FindById(led_id);
    if (led == NULL)
        return 0;

    return BSP_GPIO_Toggle(led->gpio_id);
}