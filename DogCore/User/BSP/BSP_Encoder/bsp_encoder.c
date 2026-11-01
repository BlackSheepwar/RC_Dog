/**
 * @file bsp_encoder.c
 * @brief 正交编码器驱动实现：句柄 → 定时器
 * @author Ricardo
 * @date 2026-11-01
 *
 * @note 定时器映射与方向取反只写在本文件的句柄实例里。
 *       计数器是 16 位的，两个 10 ms 周期之间最多 65535 个脉冲，
 *       远超电机实际转速，不需要软件扩展位宽。
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include <stddef.h>
#include "main.h"               /* HAL 与 &htimN */
#include "bsp_encoder.h"
// 功能包含
#include "tim.h"                /* TIM_HandleTypeDef 与 htim 实例 */

/*==============================================================================
 * 类型定义
 *============================================================================*/
/**
 * @brief 编码器句柄（一路一份）
 * @note direction_invert 用来把"电机装在另一侧、A/B 相反"的差别收在 BSP，
 *       上层拿到的增量正方向永远是"电机正转"。
 */
struct bsp_encoder {
    void    *htim;               /* 定时器句柄（本文件内还原成 TIM_HandleTypeDef *） */
    uint8_t  direction_invert;   /* 1: 计数取反 */
};

/*==============================================================================
 * 句柄实例：定时器只出现在这里
 *============================================================================*/
/** @brief 电机 1 编码器（TIM1，左前） */
const bsp_encoder_t bsp_encoder_1 = { &htim1, 0u };
/** @brief 电机 2 编码器（TIM2，右前） */
const bsp_encoder_t bsp_encoder_2 = { &htim2, 0u };
/** @brief 电机 3 编码器（TIM3，左后） */
const bsp_encoder_t bsp_encoder_3 = { &htim3, 0u };
/** @brief 电机 4 编码器（TIM4，右后） */
const bsp_encoder_t bsp_encoder_4 = { &htim4, 0u };

/*==============================================================================
 * 内部函数
 *============================================================================*/
/**
 * @brief 读计数器原始值（含符号取反）
 * @param enc 编码器句柄，不可为 NULL
 * @retval 计数器值，已按句柄配置取反
 */
static int16_t BSP_Encoder_Read_Counter(const bsp_encoder_t *enc)
{
    TIM_HandleTypeDef *htim = (TIM_HandleTypeDef *)enc->htim;
    int16_t            raw;

    raw = (int16_t)__HAL_TIM_GET_COUNTER(htim);
    return (enc->direction_invert != 0u) ? (int16_t)(-raw) : raw;
}

/*==============================================================================
 * 初始化
 *============================================================================*/
/**
 * @brief 启动所有编码器定时器并把计数器清零
 * @retval 无
 */
void BSP_Encoder_Init(void)
{
    static const bsp_encoder_t *const s_list[] = {
        &bsp_encoder_1, &bsp_encoder_2, &bsp_encoder_3, &bsp_encoder_4,
    };

    for (uint8_t i = 0u; i < 4u; i++) {
        BSP_Encoder_Reset(s_list[i]);
        (void)HAL_TIM_Encoder_Start((TIM_HandleTypeDef *)s_list[i]->htim,
                                    TIM_CHANNEL_ALL);
    }
}

/*==============================================================================
 * 读数
 *============================================================================*/
/**
 * @brief 读取本周期的计数增量并清零计数器（速度环用）
 * @param enc   编码器句柄，不可为 NULL
 * @param delta 输出增量，不可为 NULL
 * @retval 0: 读取成功
 * @retval -1: 句柄或输出指针为 NULL
 */
int BSP_Encoder_Read_Delta(const bsp_encoder_t *enc, int16_t *delta)
{
    if ((enc == NULL) || (delta == NULL)) {
        return -1;
    }

    *delta = BSP_Encoder_Read_Counter(enc);
    BSP_Encoder_Reset(enc);
    return 0;
}

/**
 * @brief 读取当前累计计数但不清零（标定线数用）
 * @param enc 编码器句柄，不可为 NULL
 * @param raw 输出累计计数，不可为 NULL
 * @retval 0: 读取成功
 * @retval -1: 句柄或输出指针为 NULL
 */
int BSP_Encoder_Read_Raw(const bsp_encoder_t *enc, int16_t *raw)
{
    if ((enc == NULL) || (raw == NULL)) {
        return -1;
    }

    *raw = BSP_Encoder_Read_Counter(enc);
    return 0;
}

/**
 * @brief 把计数器清零
 * @param enc 编码器句柄，可为 NULL
 * @retval 无
 */
void BSP_Encoder_Reset(const bsp_encoder_t *enc)
{
    if (enc == NULL) {
        return;
    }

    __HAL_TIM_SET_COUNTER((TIM_HandleTypeDef *)enc->htim, 0u);
}
