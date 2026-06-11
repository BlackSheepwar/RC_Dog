/**
 * @file bsp_pwm.c
 * @brief 通用PWM输出BSP层（内置硬件映射表）
 * @author 李嘉图
 * @date 2026-06-07
 *
 * @note 硬件映射表（pwm_id → TIM+Channel）在此文件中静态定义，
 *       上层通过 pwm_id 调用，无需知道具体定时器句柄。
 *       映射表根据项目需要自由增删，支持任意PWM外设。
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
#include "bsp_pwm.h"
#include "main.h"       // 引用 &htim9~14 等定时器句柄

/*==============================================================================
 * 内部工具函数
 *============================================================================*/
/**
 * @brief 设置指定通道的PWM比较值（计数值，非μs）
 * @param htim     定时器句柄
 * @param channel  通道宏，如 TIM_CHANNEL_1
 * @param compare  比较值（定时器计数值，≤ ARR）
 */
static void BSP_PWM_SetCompare(TIM_HandleTypeDef *htim, uint32_t channel, uint32_t compare)
{
    __HAL_TIM_SET_COMPARE(htim, channel, compare);
}

/*==============================================================================
 * PWM硬件映射表
 *
 * 将逻辑PWM编号（APP层使用的 pwm_id）映射到具体的定时器句柄和 PWM 通道。
 * 当前映射用于舵机控制：
 *   前腿（MCU1）：pwm_id 1-6（左腿 1/2/3，右腿 4/5/6）
 *   后腿（MCU2）：pwm_id 7-8
 *
 * ★ 此表为 BSP 内部实现，上层不可见。可根据需要扩展 ★
 *============================================================================*/
typedef struct {
    uint8_t            pwm_id;
    TIM_HandleTypeDef *htim;
    uint32_t           channel;
} BSP_PwmMap_t;

static const BSP_PwmMap_t PWM_HW_MAP[TIM_MAX_SIZE] = {
    { .pwm_id = 1,  .htim = &htim12, .channel = TIM_CHANNEL_2 },
    { .pwm_id = 2,  .htim = &htim12, .channel = TIM_CHANNEL_1 },
    { .pwm_id = 3,  .htim = &htim10, .channel = TIM_CHANNEL_1 },
    { .pwm_id = 4,  .htim = &htim11, .channel = TIM_CHANNEL_1 },
    { .pwm_id = 5,  .htim = &htim9,  .channel = TIM_CHANNEL_1 },
    { .pwm_id = 6,  .htim = &htim9,  .channel = TIM_CHANNEL_2 },
    { .pwm_id = 7,  .htim = &htim13, .channel = TIM_CHANNEL_1 },
    { .pwm_id = 8,  .htim = &htim14, .channel = TIM_CHANNEL_1 },
};

#define PWM_HW_MAP_SIZE  (sizeof(PWM_HW_MAP) / sizeof(PWM_HW_MAP[0]))

/** @brief 与PWM_HW_MAP同索引的ARR缓存（SRAM），0表示未设频 */
static uint32_t PWM_ARR_CACHE[TIM_MAX_SIZE];

/**
 * @brief 根据pwm_id查找硬件映射
 * @param pwm_id 逻辑PWM编号
 * @retval 非NULL：找到
 * @retval NULL：未找到
 */
static const BSP_PwmMap_t *BSP_PWM_FindMap(uint8_t pwm_id)
{
    for (uint8_t i = 0; i < PWM_HW_MAP_SIZE; i++)
    {
        if (PWM_HW_MAP[i].pwm_id == pwm_id)
            return &PWM_HW_MAP[i];
    }
    return NULL;
}

/*==============================================================================
 * 初始化函数
 *============================================================================*/
void BSP_PWM_Init(void)
{
    /* 无动态资源需要初始化，映射表为编译期常量 */
}

/**
 * @brief 设置PWM频率（采用 BSP_PWM_TIM_FREQ_HZ 计数频率）
 * @param htim           定时器句柄
 * @param TimerClockFreq 定时器时钟源频率，单位Hz
 * @param DesiredFreq    目标PWM频率
 * @note 调用后脉宽映射变为：1次计数 = (1/BSP_PWM_TIM_FREQ_HZ)秒，之前设置的CCR值会失效，
 *       需立即重新设定各通道的脉宽。
 */
void BSP_PWM_SetFreq(TIM_HandleTypeDef *htim, uint32_t TimerClockFreq, uint32_t DesiredFreq)
{
    uint32_t psc = (TimerClockFreq / BSP_PWM_TIM_FREQ_HZ) - 1;
    uint32_t arr = (BSP_PWM_TIM_FREQ_HZ / DesiredFreq) - 1;

    if (arr > TIM_ARR_MAX(htim) || psc > TIM_PSC_MAX) return;

    htim->Instance->PSC = psc;
    htim->Instance->ARR = arr;
    htim->Instance->EGR = TIM_EGR_UG;

    /* 同步更新所有使用此定时器的映射项 */
    for (uint8_t i = 0; i < PWM_HW_MAP_SIZE; i++)
    {
        if (PWM_HW_MAP[i].htim->Instance == htim->Instance)
            PWM_ARR_CACHE[i] = arr;
    }
}

/*==============================================================================
 * 定时器控制函数
 *
 * 所有函数通过 pwm_id 查内置硬件映射表（PWM_HW_MAP），
 * 上层无需知道具体的定时器句柄和通道号。
 *============================================================================*/
/**
 * @brief 启动PWM输出
 * @param pwm_id 逻辑PWM编号
 */
void BSP_PWM_Start(uint8_t pwm_id)
{
    const BSP_PwmMap_t *map = BSP_PWM_FindMap(pwm_id);
    if (map == NULL) return;

    HAL_TIM_PWM_Start(map->htim, map->channel);
}

/**
 * @brief 停止PWM输出
 * @param pwm_id 逻辑PWM编号
 */
void BSP_PWM_Stop(uint8_t pwm_id)
{
    const BSP_PwmMap_t *map = BSP_PWM_FindMap(pwm_id);
    if (map == NULL) return;

    HAL_TIM_PWM_Stop(map->htim, map->channel);
}

/**
 * @brief 重置定时器计数器
 * @param pwm_id 逻辑PWM编号
 */
void BSP_PWM_Reset(uint8_t pwm_id)
{
    const BSP_PwmMap_t *map = BSP_PWM_FindMap(pwm_id);
    if (map == NULL) return;

    __HAL_TIM_SET_COUNTER(map->htim, 0);
}

/*==============================================================================
 * 脉宽控制函数
 *============================================================================*/
 /**
 * @brief 设置PWM脉宽（适用于舵机，直接输μs）
 * @param pwm_id   逻辑PWM编号
 * @param pulse_us 高电平脉宽，单位μs（典型范围500~2500）
 * @note  内部换算：计数值 = pulse_us × BSP_PWM_TIM_FREQ_HZ / 1000000
 */
void BSP_PWM_SetPulseUs(uint8_t pwm_id, uint16_t pulse_us)
{
    const BSP_PwmMap_t *map = BSP_PWM_FindMap(pwm_id);
    if (map == NULL) return;

    uint32_t compare = (uint32_t)((uint64_t)pulse_us * BSP_PWM_TIM_FREQ_HZ / 1000000U);
    uint32_t arr     = PWM_ARR_CACHE[map - PWM_HW_MAP];
    if (arr == 0) arr = map->htim->Instance->ARR;
    if (compare > arr) compare = arr;
    BSP_PWM_SetCompare(map->htim, map->channel, compare);
}

/**
 * @brief 设置PWM占空比（适用于电机、LED等）
 * @param pwm_id       逻辑PWM编号
 * @param duty_percent 占空比百分比（0.0~100.0）
 * @note  占空比 = CCR / (ARR+1) × 100%，读取当前 ARR 计算比较值
 */
void BSP_PWM_SetDuty(uint8_t pwm_id, float duty_percent)
{
    const BSP_PwmMap_t *map = BSP_PWM_FindMap(pwm_id);
    if (map == NULL) return;

    if (duty_percent < 0.0f) duty_percent = 0.0f;
    if (duty_percent > 100.0f) duty_percent = 100.0f;

    uint32_t arr     = PWM_ARR_CACHE[map - PWM_HW_MAP];
    if (arr == 0) arr = map->htim->Instance->ARR;
    uint32_t compare = (uint32_t)((float)(arr + 1) * duty_percent / 100.0f + 0.5f);

    __HAL_TIM_SET_COMPARE(map->htim, map->channel, compare);
}
