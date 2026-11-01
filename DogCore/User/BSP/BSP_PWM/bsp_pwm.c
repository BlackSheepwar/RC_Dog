/**
 * @file bsp_pwm.c
 * @brief PWM 硬件驱动层实现：句柄 → 定时器通道
 * @author Ricardo
 * @date 2026-11-01
 *
 * @note 定时器、通道、频率的映射全部集中在本文件的句柄实例里：
 *       换板子改这一处，Device / APP 不重编也能链接（头文件只有名字）。
 *
 *       两类通道的计数频率不同，都写进句柄：
 *         舵机 1 MHz → 1 计数 = 1 μs，脉宽直接按微秒下发；
 *         电机 10 MHz → 20 kHz 时 ARR = 499，占空比有 500 级分辨率。
 *
 *       Init 只配 PSC/ARR，不碰比较值；比较值由上层在启动前设定，
 *       否则第一个周期会出现宽度不确定的脉冲，舵机表现为上电抖动。
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include <stddef.h>
#include "main.h"               /* HAL 与 &htimN */
#include "common.h"             /* ARRAY_SIZE */
#include "bsp_pwm.h"
// 功能包含
#include "tim.h"                /* TIM_HandleTypeDef 与 htim 实例 */

/*==============================================================================
 * 宏定义与常量
 *============================================================================*/
/** @brief 定时器总线时钟（H7 定时器内核时钟，APB1/APB2 均为 240 MHz） */
#define BSP_PWM_TIM_CLK_HZ      240000000U
/** @brief 舵机通道计数频率：1 MHz（1 计数 = 1 μs） */
#define BSP_PWM_SERVO_COUNT_HZ  1000000U
/** @brief 舵机 PWM 频率：330 Hz（周期约 3 ms，兼容模拟舵机） */
#define BSP_PWM_SERVO_FREQ_HZ   330U
/** @brief 电机通道计数频率：10 MHz */
#define BSP_PWM_MOTOR_COUNT_HZ  10000000U
/** @brief 电机 PWM 频率：20 kHz（超出人耳，电流纹波小） */
#define BSP_PWM_MOTOR_FREQ_HZ   20000U

/*==============================================================================
 * 类型定义
 *============================================================================*/
/**
 * @brief PWM 通道句柄（一个定时器通道一份）
 * @note 只放配置，不放运行时状态，所以能 const 进 .rodata。
 */
struct bsp_pwm {
    void     *htim;      /* 定时器句柄（本文件内还原成 TIM_HandleTypeDef *） */
    uint32_t  channel;   /* 通道宏，如 TIM_CHANNEL_1 */
    uint32_t  count_hz;  /* 计数器频率（PSC 分频后），脉宽换算用 */
};

/**
 * @brief 定时器级配置（频率是定时器的属性，不是通道的）
 * @note Init 按这张表逐个配置 PSC/ARR；同一定时器只出现一次。
 */
typedef struct {
    void     *htim;      /* 定时器句柄 */
    uint32_t  count_hz;  /* 目标计数器频率 */
    uint32_t  pwm_hz;    /* 目标 PWM 频率 */
} bsp_pwm_tim_t;

/*==============================================================================
 * 句柄实例：定时器与通道只出现在这里
 *============================================================================*/
// 舵机（前腿 1~4：TIM15/TIM14/TIM13，后腿 5~8：TIM12/TIM16/TIM17）
const bsp_pwm_t bsp_pwm_servo_1 = { &htim15, TIM_CHANNEL_2, BSP_PWM_SERVO_COUNT_HZ };
const bsp_pwm_t bsp_pwm_servo_2 = { &htim15, TIM_CHANNEL_1, BSP_PWM_SERVO_COUNT_HZ };
const bsp_pwm_t bsp_pwm_servo_3 = { &htim14, TIM_CHANNEL_1, BSP_PWM_SERVO_COUNT_HZ };
const bsp_pwm_t bsp_pwm_servo_4 = { &htim13, TIM_CHANNEL_1, BSP_PWM_SERVO_COUNT_HZ };
const bsp_pwm_t bsp_pwm_servo_5 = { &htim12, TIM_CHANNEL_2, BSP_PWM_SERVO_COUNT_HZ };
const bsp_pwm_t bsp_pwm_servo_6 = { &htim12, TIM_CHANNEL_1, BSP_PWM_SERVO_COUNT_HZ };
const bsp_pwm_t bsp_pwm_servo_7 = { &htim16, TIM_CHANNEL_1, BSP_PWM_SERVO_COUNT_HZ };
const bsp_pwm_t bsp_pwm_servo_8 = { &htim17, TIM_CHANNEL_1, BSP_PWM_SERVO_COUNT_HZ };
// 电机（1/2 在 TIM5，3/4 在 TIM8；每路一正一反）
const bsp_pwm_t bsp_pwm_motor_1_fwd = { &htim5, TIM_CHANNEL_1, BSP_PWM_MOTOR_COUNT_HZ };
const bsp_pwm_t bsp_pwm_motor_1_rev = { &htim5, TIM_CHANNEL_2, BSP_PWM_MOTOR_COUNT_HZ };
const bsp_pwm_t bsp_pwm_motor_2_fwd = { &htim5, TIM_CHANNEL_3, BSP_PWM_MOTOR_COUNT_HZ };
const bsp_pwm_t bsp_pwm_motor_2_rev = { &htim5, TIM_CHANNEL_4, BSP_PWM_MOTOR_COUNT_HZ };
const bsp_pwm_t bsp_pwm_motor_3_fwd = { &htim8, TIM_CHANNEL_1, BSP_PWM_MOTOR_COUNT_HZ };
const bsp_pwm_t bsp_pwm_motor_3_rev = { &htim8, TIM_CHANNEL_2, BSP_PWM_MOTOR_COUNT_HZ };
const bsp_pwm_t bsp_pwm_motor_4_fwd = { &htim8, TIM_CHANNEL_3, BSP_PWM_MOTOR_COUNT_HZ };
const bsp_pwm_t bsp_pwm_motor_4_rev = { &htim8, TIM_CHANNEL_4, BSP_PWM_MOTOR_COUNT_HZ };

/*==============================================================================
 * 内部变量
 *============================================================================*/
/** @brief 需要配置频率的定时器（每个定时器一条） */
static const bsp_pwm_tim_t s_tim_cfg[] = {
    { &htim12, BSP_PWM_SERVO_COUNT_HZ, BSP_PWM_SERVO_FREQ_HZ },
    { &htim13, BSP_PWM_SERVO_COUNT_HZ, BSP_PWM_SERVO_FREQ_HZ },
    { &htim14, BSP_PWM_SERVO_COUNT_HZ, BSP_PWM_SERVO_FREQ_HZ },
    { &htim15, BSP_PWM_SERVO_COUNT_HZ, BSP_PWM_SERVO_FREQ_HZ },
    { &htim16, BSP_PWM_SERVO_COUNT_HZ, BSP_PWM_SERVO_FREQ_HZ },
    { &htim17, BSP_PWM_SERVO_COUNT_HZ, BSP_PWM_SERVO_FREQ_HZ },
    { &htim5,  BSP_PWM_MOTOR_COUNT_HZ, BSP_PWM_MOTOR_FREQ_HZ },
    { &htim8,  BSP_PWM_MOTOR_COUNT_HZ, BSP_PWM_MOTOR_FREQ_HZ },
};

/** @brief 是否已配置过（幂等标记：统一初始化在调度前执行一次，守卫防将来重复调用） */
static uint8_t s_inited = 0u;

/*==============================================================================
 * 内部函数
 *============================================================================*/
/**
 * @brief 把定时器配置成目标计数频率与 PWM 频率
 * @param cfg 定时器配置项，不可为 NULL
 * @retval 无
 * @note 直接写 PSC/ARR 再置 EGR.UG 让影子寄存器立即生效；
 *       32 位定时器（TIM2/TIM5）的 ARR 上限是 0xFFFFFFFF，16 位是 0xFFFF，
 *       超限时放弃本次配置，避免把定时器写成不可预期的周期。
 */
static void BSP_PWM_Config_Timer(const bsp_pwm_tim_t *cfg)
{
    TIM_HandleTypeDef *htim;
    uint32_t           psc;
    uint32_t           arr;
    uint32_t           arr_max;

    if (cfg == NULL) {
        return;
    }

    htim = (TIM_HandleTypeDef *)cfg->htim;
    if (htim == NULL) {
        return;
    }

    psc = (BSP_PWM_TIM_CLK_HZ / cfg->count_hz) - 1U;
    arr = (cfg->count_hz / cfg->pwm_hz) - 1U;

    arr_max = IS_TIM_32B_COUNTER_INSTANCE(htim->Instance) ? 0xFFFFFFFFU : 0xFFFFU;
    if ((psc > 0xFFFFU) || (arr > arr_max)) {
        return;
    }

    htim->Instance->PSC = psc;
    htim->Instance->ARR = arr;
    htim->Instance->EGR = TIM_EGR_UG;
}

/*==============================================================================
 * 初始化
 *============================================================================*/
/**
 * @brief 按句柄表配置所有 PWM 定时器的预分频与自动重装值
 * @retval 无
 * @note 幂等：只配置第一次。本函数在 freertos.c 的调度前统一调用一次；
 *       守卫是给将来的重复调用兜底——EGR.UG 会在任意相位把计数器清零，
 *       重配一次等于在所有通道上凭空多出一个不完整周期，
 *       已经转起来的舵机/电机会因此抖一下。
 */
void BSP_PWM_Init(void)
{
    if (s_inited != 0u) {
        return;
    }

    for (uint8_t i = 0u; i < ARRAY_SIZE(s_tim_cfg); i++) {
        BSP_PWM_Config_Timer(&s_tim_cfg[i]);
    }

    s_inited = 1u;
}

/*==============================================================================
 * 启停
 *============================================================================*/
/**
 * @brief 启动通道输出
 * @param pwm 通道句柄，可为 NULL
 * @retval 无
 */
void BSP_PWM_Start(const bsp_pwm_t *pwm)
{
    TIM_HandleTypeDef *htim;

    if (pwm == NULL) {
        return;
    }

    htim = (TIM_HandleTypeDef *)pwm->htim;
    if (htim == NULL) {
        return;
    }

    (void)HAL_TIM_PWM_Start(htim, pwm->channel);

    /* 高级定时器（TIM1/8/15/16/17）必须有 BDTR.MOE 才有输出；
     * 通用定时器没有 BDTR，写这一位是空操作，所以统一加一句最稳 */
    __HAL_TIM_MOE_ENABLE(htim);
}

/**
 * @brief 停止通道输出
 * @param pwm 通道句柄，可为 NULL
 * @retval 无
 */
void BSP_PWM_Stop(const bsp_pwm_t *pwm)
{
    TIM_HandleTypeDef *htim;

    if (pwm == NULL) {
        return;
    }

    htim = (TIM_HandleTypeDef *)pwm->htim;
    if (htim == NULL) {
        return;
    }

    (void)HAL_TIM_PWM_Stop(htim, pwm->channel);
}

/*==============================================================================
 * 占空比 / 脉宽
 *============================================================================*/
/**
 * @brief 设置高电平脉宽（舵机用）
 * @param pwm      通道句柄，可为 NULL
 * @param pulse_us 脉宽，单位 μs；超出周期时夹到周期值
 * @retval 无
 */
void BSP_PWM_Set_Pulse_Us(const bsp_pwm_t *pwm, uint16_t pulse_us)
{
    TIM_HandleTypeDef *htim;
    uint32_t           compare;
    uint32_t           arr;

    if (pwm == NULL) {
        return;
    }

    htim = (TIM_HandleTypeDef *)pwm->htim;
    if (htim == NULL) {
        return;
    }

    compare = (uint32_t)(((uint64_t)pulse_us * pwm->count_hz) / 1000000U);

    arr = htim->Instance->ARR;
    if (compare > arr) {
        compare = arr;
    }

    __HAL_TIM_SET_COMPARE(htim, pwm->channel, compare);
}

/**
 * @brief 设置占空比（电机、LED 用）
 * @param pwm     通道句柄，可为 NULL
 * @param percent 占空比 0.0 ~ 100.0，超出自动夹紧
 * @retval 无
 */
void BSP_PWM_Set_Duty_Percent(const bsp_pwm_t *pwm, float percent)
{
    TIM_HandleTypeDef *htim;
    uint32_t           arr;
    uint32_t           compare;

    if (pwm == NULL) {
        return;
    }

    htim = (TIM_HandleTypeDef *)pwm->htim;
    if (htim == NULL) {
        return;
    }

    if (percent < 0.0f) {
        percent = 0.0f;
    }
    if (percent > 100.0f) {
        percent = 100.0f;
    }

    arr     = htim->Instance->ARR;
    compare = (uint32_t)(((float)arr + 1.0f) * percent / 100.0f + 0.5f);

    __HAL_TIM_SET_COMPARE(htim, pwm->channel, compare);
}
