/**
 * @file bsp_encoder.c
 * @brief 定时器编码器读取（BSP层）实现
 * @author 李嘉图
 * @date 2026-07-05
 *
 * @note 使用 STM32 定时器编码器模式（Encoder Mode TI1+TI2），
 *       硬件自动根据 A/B 相脉冲计数和判向。
 *       每次读取后自动清零计数器，供上层做增量速度计算。
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include <stdint.h>
#include "main.h"
#include "common.h"
#include "bsp_encoder.h"
// 功能包含
#include "tim.h"

/*==============================================================================
 * 编码器硬件映射表
 *
 * 将逻辑 encoder_id 映射到具体的定时器句柄。
 * 当前映射：
 *   encoder 0 → TIM1（左前腿电机）
 *   encoder 1 → TIM2（右前腿电机）
 *   encoder 2 → TIM3（左后腿电机）
 *   encoder 3 → TIM4（右后腿电机）
 *
 * ★ 此表为 BSP 内部实现，上层不可见。换硬件时改表即可 ★
 *============================================================================*/
typedef struct {
    uint8_t            encoder_id;
    TIM_HandleTypeDef *htim;
} BSP_EncoderMap_t;

static const BSP_EncoderMap_t ENCODER_HW_MAP[BSP_ENCODER_NUM] = {
    { .encoder_id = 0, .htim = &htim1 },
    { .encoder_id = 1, .htim = &htim2 },
    { .encoder_id = 2, .htim = &htim3 },
    { .encoder_id = 3, .htim = &htim4 },
};

/**
 * @brief 根据 encoder_id 查找硬件映射
 * @param id 编码器逻辑编号
 * @retval 非 NULL：找到
 * @retval NULL：未找到
 */
static const BSP_EncoderMap_t *BSP_Encoder_FindMap(uint8_t id)
{
    for (uint8_t i = 0; i < BSP_ENCODER_NUM; i++)
    {
        if (ENCODER_HW_MAP[i].encoder_id == id)
            return &ENCODER_HW_MAP[i];
    }
    return NULL;
}

/*==============================================================================
 * 初始化函数
 *============================================================================*/
/**
 * @brief 编码器 BSP 层初始化
 * @note 启动所有编码器定时器的编码器模式，计数器归零
 */
void BSP_Encoder_Init(void)
{
    for (uint8_t i = 0; i < BSP_ENCODER_NUM; i++)
    {
        const BSP_EncoderMap_t *map = &ENCODER_HW_MAP[i];

        __HAL_TIM_SET_COUNTER(map->htim, 0);
        HAL_TIM_Encoder_Start(map->htim, TIM_CHANNEL_ALL);
    }
}

/*==============================================================================
 * 编码器读数函数
 *============================================================================*/
/**
 * @brief 获取指定编码器的计数值（读取后自动清零）
 * @param id    编码器索引（0 ~ BSP_ENCODER_NUM-1）
 * @param count 输出计数值的指针
 * @retval 1: 成功
 * @retval 0: 失败
 */
uint8_t BSP_Encoder_GetCount(uint8_t id, int16_t *count)
{
    if (id >= BSP_ENCODER_NUM || count == NULL)
        return 0;

    const BSP_EncoderMap_t *map = BSP_Encoder_FindMap(id);
    if (map == NULL)
        return 0;

    *count = (int16_t)__HAL_TIM_GET_COUNTER(map->htim);
    __HAL_TIM_SET_COUNTER(map->htim, 0);

    return 1;
}
