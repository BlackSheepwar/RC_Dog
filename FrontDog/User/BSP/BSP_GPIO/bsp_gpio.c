/**
 * @file bsp_gpio.c
 * @brief GPIO 通用 BSP 层实现（内置硬件映射表）
 * @author 李嘉图
 * @date 2026-06-17
 *
 * @note 硬件映射表（gpio_id → GPIOx + Pin）在此文件中静态定义，
 *       上层通过 gpio_id 调用，无需知道具体 GPIO 端口和引脚。
 *       映射表根据项目需要自由增删，修改硬件时只需改这张表。
 *
 *       适用场景：按键、限位开关、编码器索引、传感器数字输出、
 *       LED、继电器、蜂鸣器等所有 GPIO 输入/输出场合。
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include <stdint.h>
#include "main.h"       /* 引用 GPIOC 等宏和 GPIO_PIN_x 定义 */
#include "common.h"
#include "bsp_gpio.h"
// 功能包含
#include "gpio.h"

/*==============================================================================
 * GPIO 硬件映射表
 *
 * 将逻辑编号（上层使用的 gpio_id）映射到具体的 GPIO 端口、引脚和方向。
 * 当前映射：
 *   gpio_id 1 → GPIOC, Pin 13（板载用户按键，输入）
 *   gpio_id 2 → GPIOB, Pin 2 （通用输出）
 *
 * ★ 此表为 BSP 内部实现，上层不可见。可根据需要扩展 ★
 *
 * dir 字段记录该引脚的预期方向，BSP 函数在执行操作前会做方向检查：
 *   读操作仅允许 GPIO_DIR_IN，写/翻转操作仅允许 GPIO_DIR_OUT。
 *   方向不匹配时函数直接返回，不调用 HAL，避免误操作。
 *============================================================================*/
typedef struct {
    uint8_t      gpio_id;
    GPIO_TypeDef *GPIOx;
    uint16_t     Pin;
    uint8_t      dir;        /* GPIO_DIR_IN / GPIO_DIR_OUT */
} BSP_GPIO_Map_t;

static const BSP_GPIO_Map_t GPIO_HW_MAP[GPIO_MAX_NUM] = {
    { .gpio_id = 1, .GPIOx = GPIOC, .Pin = GPIO_PIN_13, .dir = GPIO_DIR_IN  },
    { .gpio_id = 2, .GPIOx = GPIOB, .Pin = GPIO_PIN_2,  .dir = GPIO_DIR_OUT },
};

/**
 * @brief 根据 gpio_id 查找硬件映射
 * @param gpio_id 逻辑编号
 * @retval 非 NULL：找到
 * @retval NULL：未找到
 */
static const BSP_GPIO_Map_t *BSP_GPIO_FindMap(uint8_t gpio_id)
{
    for (uint8_t i = 0; i < ARRAY_SIZE(GPIO_HW_MAP); i++)
    {
        if (GPIO_HW_MAP[i].gpio_id == gpio_id)
            return &GPIO_HW_MAP[i];
    }
    return NULL;
}

/*==============================================================================
 * 初始化函数
 *============================================================================*/
 /**
 * @brief GPIO BSP 层初始化
 */
void BSP_GPIO_Init(void)
{
    /* 映射表为编译期常量，无需运行时初始化 */
}

/*==============================================================================
 * 数字输入
 *============================================================================*/
/**
 * @brief 读取 GPIO 引脚电平
 * @param gpio_id 逻辑编号（对应 GPIO_HW_MAP 中的映射）
 * @retval 1: 高电平
 * @retval 0: 低电平
 * @retval 0xFF: 失败（gpio_id 无效或非输入引脚）
 */
uint8_t BSP_GPIO_ReadLevel(uint8_t gpio_id)
{
    const BSP_GPIO_Map_t *map = BSP_GPIO_FindMap(gpio_id);
    if (map == NULL || map->dir != GPIO_DIR_IN)
        return 0xFF;

    return HAL_GPIO_ReadPin(map->GPIOx, map->Pin);
}

/*==============================================================================
 * 数字输出
 *============================================================================*/
/**
 * @brief 设置 GPIO 引脚电平
 * @param gpio_id 逻辑编号
 * @param level   电平：GPIO_LEVEL_HIGH 或 GPIO_LEVEL_LOW
 * @retval 1: 成功
 * @retval 0: 失败（gpio_id 无效或非输出引脚）
 */
uint8_t BSP_GPIO_SetLevel(uint8_t gpio_id, uint8_t level)
{
    const BSP_GPIO_Map_t *map = BSP_GPIO_FindMap(gpio_id);
    if (map == NULL || map->dir != GPIO_DIR_OUT)
        return 0;

    HAL_GPIO_WritePin(map->GPIOx, map->Pin,
                      level ? GPIO_PIN_SET : GPIO_PIN_RESET);
    return 1;
}

/**
 * @brief 翻转 GPIO 引脚电平
 * @param gpio_id 逻辑编号
 * @retval 1: 成功
 * @retval 0: 失败（gpio_id 无效或非输出引脚）
 * @note 高→低 或 低→高
 */
uint8_t BSP_GPIO_Toggle(uint8_t gpio_id)
{
    const BSP_GPIO_Map_t *map = BSP_GPIO_FindMap(gpio_id);
    if (map == NULL || map->dir != GPIO_DIR_OUT)
        return 0;

    HAL_GPIO_TogglePin(map->GPIOx, map->Pin);
    return 1;
}

/*==============================================================================
 * 查询
 *============================================================================*/
/**
 * @brief 检查 gpio_id 是否在映射表中有效
 * @param gpio_id 逻辑编号
 * @retval 1: 有效
 * @retval 0: 无效（未在映射表中）
 */
 uint8_t BSP_GPIO_IsValid(uint8_t gpio_id)
{
    return (BSP_GPIO_FindMap(gpio_id) != NULL) ? 1 : 0;
}
