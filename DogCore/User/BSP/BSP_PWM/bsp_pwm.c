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
// 固定包含
#include <stdint.h>
#include "main.h"       /* 引用 &htim9~14 等定时器句柄 */
#include "common.h"
#include "bsp_pwm.h"
// 功能包含
#include "stm32h7xx_hal_tim.h"
#include "tim.h"         /* TIM_HandleTypeDef */

/*==============================================================================
 * 宏定义与常量
 *============================================================================*/
#define TIM_APB2_Hz     240000000        // APB2定时器时钟（TIM1, TIM8, TIM15, TIM16, TIM17）120MHz
#define TIM_APB1_Hz     240000000        // APB1定时器时钟（TIM2, TIM3, TIM4, TIM5, TIM6, TIM7, TIM12, TIM13, TIM14）120MHz
#define BSP_SERVO_PWM_TIM_FREQ_HZ  1000000U   // 定时器计数频率，1MHz → 1计数值=1μs
#define TIM_PSC_MAX          0xFFFFU    // PSC在所有STM32定时器上均为16位
#define TIM_ARR_MAX(htim)    (IS_TIM_32B_COUNTER_INSTANCE((htim)->Instance) ? 0xFFFFFFFFU : 0xFFFFU)

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
    { .pwm_id = 1,  .htim = &htim15, .channel = TIM_CHANNEL_1 },
    { .pwm_id = 2,  .htim = &htim15, .channel = TIM_CHANNEL_2 },
    { .pwm_id = 3,  .htim = &htim14, .channel = TIM_CHANNEL_1 },
    { .pwm_id = 4,  .htim = &htim13, .channel = TIM_CHANNEL_1 },
    { .pwm_id = 5,  .htim = &htim12, .channel = TIM_CHANNEL_1 },
    { .pwm_id = 6,  .htim = &htim12, .channel = TIM_CHANNEL_2 },
    { .pwm_id = 7,  .htim = &htim16, .channel = TIM_CHANNEL_1 },
    { .pwm_id = 8,  .htim = &htim17, .channel = TIM_CHANNEL_1 },
};

/*==============================================================================
 * 定时器频率配置表（BSP 内置，上层不可见）
 *
 * 将定时器句柄映射到其总线时钟频率，供 BSP_PWM_SetAllFreq 批量初始化。
 * 修改硬件（换定时器/换时钟树）时只需改这张表。
 *============================================================================*/
typedef struct {
    TIM_HandleTypeDef *htim;
    uint32_t           clock_hz;      /* 定时器总线时钟（APB1/APB2），与 bsp_pwm.h 宏对应 */
    uint32_t           tim_freq_hz;   /* 定时器计数频率（PSC 分频后的计数器时钟） */
    uint32_t           freq;          /* 目标 PWM 频率（Hz） */
} BSP_TimerFreqMap_t;

static const BSP_TimerFreqMap_t PWM_TIM_CFG[] = {
    { .htim = &htim12, .clock_hz = TIM_APB1_Hz, .tim_freq_hz = BSP_SERVO_PWM_TIM_FREQ_HZ, .freq = 330 },
    { .htim = &htim13, .clock_hz = TIM_APB1_Hz, .tim_freq_hz = BSP_SERVO_PWM_TIM_FREQ_HZ, .freq = 330 },
    { .htim = &htim14, .clock_hz = TIM_APB1_Hz, .tim_freq_hz = BSP_SERVO_PWM_TIM_FREQ_HZ, .freq = 330 },
    { .htim = &htim15, .clock_hz = TIM_APB2_Hz, .tim_freq_hz = BSP_SERVO_PWM_TIM_FREQ_HZ, .freq = 330 },
    { .htim = &htim16, .clock_hz = TIM_APB2_Hz, .tim_freq_hz = BSP_SERVO_PWM_TIM_FREQ_HZ, .freq = 330 },
    { .htim = &htim17, .clock_hz = TIM_APB2_Hz, .tim_freq_hz = BSP_SERVO_PWM_TIM_FREQ_HZ, .freq = 330 },
};

/**
 * @brief 设置PWM频率（按 cfg->tim_freq_hz 计数频率）
 * @param cfg 定时器频率配置表项（含预计算 ARR）
 * @note 调用后脉宽映射变为：1次计数 = (1/tim_freq_hz)秒，之前设置的CCR值会失效，
 *       需立即重新设定各通道的脉宽。
 */
static void BSP_PWM_SetFreq(const BSP_TimerFreqMap_t *cfg)
{
    uint32_t psc = (cfg->clock_hz / cfg->tim_freq_hz) - 1U;
    uint32_t arr = (cfg->tim_freq_hz / cfg->freq) - 1U;

    if (arr > TIM_ARR_MAX(cfg->htim) || psc > TIM_PSC_MAX) return;

    cfg->htim->Instance->PSC = psc;
    cfg->htim->Instance->ARR = arr;
    cfg->htim->Instance->EGR = TIM_EGR_UG;
}

/**
 * @brief 批量设置所有 PWM 定时器频率（按 PWM_TIM_CFG 表）
 * @note 一行调用替代上层循环，定时器→时钟→频率映射由 BSP 内置。
 *       在 APP_Servo_Add 之前调用。
 */
static void BSP_PWM_SetAllFreq(void)
{
    for (uint8_t i = 0; i < ARRAY_SIZE(PWM_TIM_CFG); i++)
        BSP_PWM_SetFreq(&PWM_TIM_CFG[i]);
}

/*==============================================================================
 * 内部工具函数
 *============================================================================*/
/**
 * @brief 根据pwm_id查找硬件映射
 * @param pwm_id 逻辑PWM编号
 * @retval 非NULL：找到
 * @retval NULL：未找到
 */
static const BSP_PwmMap_t *BSP_PWM_FindMap(uint8_t pwm_id)
{
    for (uint8_t i = 0; i < ARRAY_SIZE(PWM_HW_MAP); i++)
    {
        if (PWM_HW_MAP[i].pwm_id == pwm_id)
            return &PWM_HW_MAP[i];
    }
    return NULL;
}

/**
 * @brief 根据定时器句柄查找对应的频率配置项
 * @param htim 定时器句柄
 * @retval 非NULL：找到
 * @retval NULL：未找到
 */
static const BSP_TimerFreqMap_t *BSP_PWM_FindTimerCfg(const TIM_HandleTypeDef *htim)
{
    for (uint8_t i = 0; i < ARRAY_SIZE(PWM_TIM_CFG); i++)
    {
        if (PWM_TIM_CFG[i].htim == htim)
            return &PWM_TIM_CFG[i];
    }
    return NULL;
}

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
 * 初始化函数
 *============================================================================*/
void BSP_PWM_Init(void)
{
    // 定时器频率设置
    BSP_PWM_SetAllFreq();
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
 * @note  内部换算：计数值 = pulse_us × tim_freq_hz / 1000000
 */
void BSP_PWM_SetPulseUs(uint8_t pwm_id, uint16_t pulse_us)
{
    const BSP_PwmMap_t *map = BSP_PWM_FindMap(pwm_id);
    if (map == NULL) return;

    const BSP_TimerFreqMap_t *tcfg = BSP_PWM_FindTimerCfg(map->htim);
    if (tcfg == NULL) return;

    uint32_t compare = (uint32_t)((uint64_t)pulse_us * tcfg->tim_freq_hz / 1000000U);
    uint32_t arr     = map->htim->Instance->ARR;
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

    uint32_t arr     = map->htim->Instance->ARR;
    uint32_t compare = (uint32_t)((float)(arr + 1) * duty_percent / 100.0f + 0.5f);

    BSP_PWM_SetCompare(map->htim, map->channel, compare);
}
