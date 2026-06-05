/**
 * @file app_knob.c
 * @brief 电位器旋钮驱动实现
 * @author 李嘉图
 * @date 2026-05-28
 *
 * @note 通过静态资源池管理所有旋钮实例，注册时保存ADC绑定关系与映射区间。
 *       APP_KNOB_GetValue 调用 BSP_ADC_GetValue 获取原始值后，
 *       按线性比例映射到 [min_val, max_val] 区间返回。
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
#include "app_knob.h"

/*==============================================================================
 * 静态资源池
 *============================================================================*/
static APP_Knob_t knob_pool[KNOB_MAX_NUM];
static uint8_t knob_count = 0;

/*==============================================================================
 * 内部函数
 *============================================================================*/
/**
 * @brief 根据旋钮ID查找旋钮结构体（O(n)）
 * @param knob_id 旋钮ID
 * @retval 非NULL：对应结构体指针
 * @retval NULL：未找到
 */
static APP_Knob_t* APP_KNOB_GetById(uint8_t knob_id)
{
    /* ---------- 遍历已注册旋钮 ---------- */
    for(uint8_t i = 0; i < knob_count; i++)
    {
        if(knob_pool[i].knob_id == knob_id)
        {
            return &knob_pool[i];
        }
    }

    return NULL; // 未找到
}

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
 *
 * @note min_val 对应 ADC=0 时的输出，max_val 对应 ADC=4095 时的输出。
 *       若 min_val > max_val，映射方向自动反转，无需额外配置。 @retval -3: 重复注册
 * @retval -4: 资源池已满
 *
 * @note 注册后不启动任何硬件，仅保存配置。ADC的DMA采集需事先通过
 *       BSP_ADC_Register 启动。
 */
int8_t APP_KNOB_Register(uint8_t knob_id, uint8_t adc_id,
                          uint8_t adc_ch_index,
                          int16_t min_val, int16_t max_val)
{
    /* ---------- 参数检查 ---------- */
    if(adc_ch_index < 1) return -1;
    if(min_val == max_val) return -2;

    /* ---------- 查重（O(n)） ---------- */
    if(APP_KNOB_GetById(knob_id) != NULL)
    {
        return -3; // 已存在
    }

    /* ---------- 容量检查 ---------- */
    if(knob_count >= KNOB_MAX_NUM)
    {
        return -4;
    }

    /* ---------- 写入资源池 ---------- */
    knob_pool[knob_count].knob_id      = knob_id;
    knob_pool[knob_count].adc_id       = adc_id;
    knob_pool[knob_count].adc_ch_index = adc_ch_index;
    knob_pool[knob_count].enable       = 1;     // 默认启用
    knob_pool[knob_count].min_val      = min_val;
    knob_pool[knob_count].max_val      = max_val;
    knob_count++;

    return 0;
}

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
 * @retval -3: 旋钮未使能（enable == 0）
 *
 * @note min_val 对应 ADC=0 时的输出，max_val 对应 ADC=4095 时的输出。
 *       若 min_val > max_val，映射方向自动反转，无需额外配置。
 *       计算公式: out = min_val + raw * (max_val - min_val) / 4095
 */
int8_t APP_KNOB_GetValue(uint8_t knob_id, int32_t *out_val)
{
    APP_Knob_t *p = APP_KNOB_GetById(knob_id);
    if(p == NULL) return -1;

    /* ---------- 检查旋钮是否使能 ---------- */
    if(!p->enable) return -3;

    /* ---------- 从BSP层读取ADC原始值 ---------- */
    uint16_t raw;
    if(BSP_ADC_GetValue(p->adc_id, p->adc_ch_index, &raw) != 0)
    {
        return -2;
    }

    /* ---------- 线性映射 ---------- */
    int32_t range = p->max_val - p->min_val;
    *out_val = (int32_t)(p->min_val + (raw * range) / 4095);

    return 0;
}
