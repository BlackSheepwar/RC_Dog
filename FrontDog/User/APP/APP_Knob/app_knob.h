/**
 * @file app_knob.h
 * @brief 电位器旋钮驱动（应用层）
 * @author 李嘉图
 * @date 2026-05-28
 *
 * @note 在BSP_ADC基础上封装旋钮概念，每个旋钮绑定一个ADC规则组通道。
 *       调用APP_KNOB_GetValue传入旋钮ID，自动完成ADC原始值读取与线性映射。
 *       旋钮的映射区间 [min_val, max_val] 在注册时确定，适用于电位器、
 *       滑条等模拟量输入设备的值归一化。
 */

#ifndef __APP_KNOB_H__
#define __APP_KNOB_H__

/*==============================================================================
 * 头文件包含
 *============================================================================*/
#include "bsp_adc.h"

/*==============================================================================
 * 宏定义与常量
 *============================================================================*/
#define KNOB_MAX_NUM     2       // 旋钮最大数量

/*==============================================================================
 * 旋钮结构体定义
 *============================================================================*/
typedef struct {
    uint8_t  knob_id;            // 旋钮ID（上层标识）
    uint8_t  adc_id;             // 绑定的ADC外设ID
    uint8_t  adc_ch_index;       // ADC规则组通道索引（1 ~ ch_num）
    int16_t min_val;             // 映射最小值（支持负值）
    int16_t max_val;             // 映射最大值
} APP_Knob_t;

/*==============================================================================
 * 注册函数
 *============================================================================*/
/**
 * @brief 注册一个旋钮
 * @param knob_id       旋钮ID
 * @param adc_id        绑定ADC外设ID（需已通过BSP_ADC_Register注册）
 * @param adc_ch_index  ADC规则组通道索引（1 ~ ch_num，对应CubeMX中Rank顺序）
 * @param min_val       映射最小值
 * @param max_val       映射最大值
 * @retval 0:  成功
 * @retval -1: 通道索引错误（adc_ch_index < 1）
 * @retval -2: 映射区间错误（min_val == max_val，无变化范围）
 * @retval -3: 重复注册
 * @retval -4: 资源池已满
 */
int8_t APP_KNOB_Register(uint8_t knob_id, uint8_t adc_id,
                          uint8_t adc_ch_index,
                          int16_t min_val, int16_t max_val);

/*==============================================================================
 * 功能函数
 *============================================================================*/
/**
 * @brief 获取旋钮映射值
 * @param knob_id  旋钮ID
 * @param out_val  输出指针，指向存放映射值的变量
 * @retval 0:  成功
 * @retval -1: 未找到该旋钮
 * @retval -2: 底层ADC读取失败
 *
 * @note min_val 对应 ADC=0 时的输出，max_val 对应 ADC=4095 时的输出。
 *       若 min_val > max_val，映射方向自动反转，无需额外配置。
 *       计算公式: out = min_val + raw * (max_val - min_val) / 4095
 */
int8_t APP_KNOB_GetValue(uint8_t knob_id, int32_t *out_val);

#endif
