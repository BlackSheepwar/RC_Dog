/**
 * @file bsp_ADC.h
 * @brief ADC底层驱动（DMA连续采集）
 * @author 李嘉图
 * @date 2026-05-28
 *
 * @note 每个结构体代表一个ADC外设，使用DMA回环+连续模式自动采集。
 *       规则组各通道的ADC值实时更新在 dma_buf 中，上层按索引读取。
 */

#ifndef __ADC_BSP_H__
#define __ADC_BSP_H__

/*==============================================================================
 * 头文件包含
 *============================================================================*/
#include "adc.h"

/*==============================================================================
 * 宏定义与常量
 *============================================================================*/
#define ADC_MAX_NUM     2       // ADC外设最大数量
#define ADC_CH_MAX_NUM  7       // 单个ADC最大规则组通道数

/*==============================================================================
 * ADC外设结构体
 *============================================================================*/
typedef struct {
    uint8_t id;                         // ADC外设ID
    ADC_HandleTypeDef *hadc;            // ADC句柄
    uint8_t ch_num;                     // 规则组通道数量（对应CubeMX NbrOfConversion）
    uint16_t dma_buf[ADC_CH_MAX_NUM];   // DMA环形缓冲区，自动刷新各通道ADC值
} BSP_ADC_t;

/*==============================================================================
 * 初始化函数
 *============================================================================*/
/**
 * @brief ADC初始化（清空资源池）
 */
void BSP_ADC_Init(void);

/**
 * @brief 注册一个ADC外设，开启DMA连续采集
 * @param id     ADC外设ID
 * @param hadc   ADC句柄
 * @param ch_num 规则组通道数（需与CubeMX中NbrOfConversion一致）
 * @retval 0:  成功
 * @retval -1: 无效句柄（hadc == NULL）
 * @retval -2: 通道数错误（0 或超出最大值）
 * @retval -3: 重复注册
 * @retval -4: 资源池已满
 */
int8_t BSP_ADC_Register(uint8_t id, ADC_HandleTypeDef *hadc, uint8_t ch_num);

/*==============================================================================
 * 功能函数
 *============================================================================*/
/**
 * @brief 获取指定ADC某规则组的计数值
 * @param id       ADC外设ID
 * @param ch_index 规则组索引（1 ~ ch_num）
 * @param out_val  输出指针，指向存放ADC原始值的变量（12位，0~4095）
 * @retval 0:  成功
 * @retval -1: 未找到该ADC
 * @retval -2: 通道索引越界
 */
int8_t BSP_ADC_GetValue(uint8_t id, uint8_t ch_index, uint16_t *out_val);

#endif
