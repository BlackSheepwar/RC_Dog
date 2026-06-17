/**
 * @file bsp_ADC.c
 * @brief ADC底层驱动实现（DMA连续采集）
 * @author 李嘉图
 * @date 2026-05-28
 *
 * @note 基于ADC连续转换 + DMA回环模式，注册时启动DMA传输，
 *       各规则组通道值自动刷入 dma_buf 数组，上层直接索引读取。
 *       CubeMX需配置：ScanMode=ENABLE, ContinuousConvMode=ENABLE,
 *       DMA Circular=ENABLE, NbrOfConversion=实际通道数。
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
#include "main.h"
#include "stm32f1xx_hal_adc_ex.h"
#include "bsp_ADC.h"

/*==============================================================================
 * 静态资源池
 *============================================================================*/
static BSP_ADC_t bsp_adc_pool[ADC_MAX_NUM];
static uint8_t bsp_adc_count = 0;

/*==============================================================================
 * 内部函数 
 *============================================================================*/
/**
 * @brief 根据ID查找ADC结构体（O(n)）
 * @param id ADC外设ID
 * @retval 非NULL：对应结构体指针
 * @retval NULL：未找到
 */
static BSP_ADC_t* BSP_ADC_GetById(uint8_t id)
{
    /* ---------- 遍历已注册ADC ---------- */
    for(uint8_t i = 0; i < bsp_adc_count; i++)
    {
        if(bsp_adc_pool[i].id == id)
        {
            return &bsp_adc_pool[i];
        }
    }

    return NULL; // 未找到
}

/*==============================================================================
 * 初始化函数
 *============================================================================*/
/**
 * @brief ADC初始化（清空资源池）
 */
void BSP_ADC_Init(void)
{
    bsp_adc_count = 0;
}

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
 *
 * @note 注册后自动调用 HAL_ADC_Start_Dma() 启动连续转换，
 *       转换结果通过DMA回环不断刷新 dma_buf。
 */
int8_t BSP_ADC_Register(uint8_t id, ADC_HandleTypeDef *hadc, uint8_t ch_num)
{
    if(hadc == NULL) return -1;
    if(ch_num == 0 || ch_num > ADC_CH_MAX_NUM) return -2;

    /* ---------- 查重（O(n)） ---------- */
    if(BSP_ADC_GetById(id) != NULL)
    {
        return -3; // 已存在
    }

    /* ---------- 容量检查 ---------- */
    if(bsp_adc_count >= ADC_MAX_NUM)
    {
        return -4;
    }

    /* ---------- 注册 ---------- */
    bsp_adc_pool[bsp_adc_count].id      = id;
    bsp_adc_pool[bsp_adc_count].hadc    = hadc;
    bsp_adc_pool[bsp_adc_count].ch_num  = ch_num;

    /* ---------- 清空DMA缓冲区 ---------- */
    for(uint8_t i = 0; i < ch_num; i++)
    {
        bsp_adc_pool[bsp_adc_count].dma_buf[i] = 0;
    }

    bsp_adc_count++;

    /* ---------- 启动DMA连续采集 ---------- */
    /* 使用ADC连续模式 + DMA回环，启动后无需干预 */
    BSP_ADC_t *p = BSP_ADC_GetById(id);
    // ADC内部校准
    HAL_ADCEx_Calibration_Start(p->hadc);
    HAL_ADC_Start_DMA(p->hadc, (uint32_t *)p->dma_buf, p->ch_num);

    return 0;
}

/*==============================================================================
 * 功能函数
 *============================================================================*/
/**
 * @brief 获取指定ADC某规则组的计数值
 * @param id       ADC外设ID
 * @param ch_index 规则组索引（1 ~ ch_num，对应CubeMX中Rank顺序）
 * @param out_val  输出指针，指向存放ADC原始值的变量（12位，0~4095）
 * @retval 0:  成功
 * @retval -1: 未找到该ADC
 * @retval -2: 通道索引越界
 */
int8_t BSP_ADC_GetValue(uint8_t id, uint8_t ch_index, uint16_t *out_val)
{
    BSP_ADC_t *p = BSP_ADC_GetById(id);
    if(p == NULL) return -1;
    if(ch_index < 1 || ch_index > p->ch_num) return -2;

    *out_val = p->dma_buf[ch_index-1];
    return 0;
}
