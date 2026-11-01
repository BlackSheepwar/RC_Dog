/**
 * @file dev_motor.c
 * @brief 电机设备层实现：双 PWM 占空比 + 编码器计数
 * @author Ricardo
 * @date 2026-11-01
 *
 * @note 四台电机的 "正转通道 / 反转通道 / 编码器 / 每转脉冲数" 都在本文件的
 *       实例表里。上层拿到的是设备句柄，不再自己配 pwm_fwd + pwm_rev。
 *
 *       本文件不出现时间：没有 dt、没有速度、没有 PID。
 *       速度是"脉冲数 ÷ 时间"的商，所以要时间的地方一定在 Service。
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include <stddef.h>
#include "common.h"             /* ARRAY_SIZE */
#include "dev_motor.h"
// 功能包含
#include "bsp_pwm.h"            /* BSP_PWM_Set_Duty_Percent / Start */
#include "bsp_encoder.h"        /* BSP_Encoder_Read_Delta / Reset */

/*==============================================================================
 * 类型定义
 *============================================================================*/
/**
 * @brief 电机句柄（一台一份）
 * @note 全部是配置，可 const 进 .rodata。
 */
struct dev_motor {
    const bsp_pwm_t     *pwm_fwd;    /* 正转 PWM 通道 */
    const bsp_pwm_t     *pwm_rev;    /* 反转 PWM 通道 */
    const bsp_encoder_t *encoder;    /* 正交编码器 */
    float                cpr;        /* 输出轴每转脉冲数（含减速比） */
    uint8_t              reverse;    /* 1: 镜像安装（PWM 与编码器同时取反） */
};

/*==============================================================================
 * 句柄实例：通道与编码器只出现在这里
 *============================================================================*/
/** @brief 1 号电机（左前）：TIM5 CH1/CH2 + TIM1 编码器，正装 */
const dev_motor_t dev_motor_1 = {
    .pwm_fwd = &bsp_pwm_motor_1_fwd, .pwm_rev = &bsp_pwm_motor_1_rev,
    .encoder = &bsp_encoder_1, .cpr = 60000.0f, .reverse = 0u,
};
/** @brief 2 号电机（右前）：TIM5 CH3/CH4 + TIM2 编码器，镜像安装 */
const dev_motor_t dev_motor_2 = {
    .pwm_fwd = &bsp_pwm_motor_2_fwd, .pwm_rev = &bsp_pwm_motor_2_rev,
    .encoder = &bsp_encoder_2, .cpr = 60000.0f, .reverse = 1u,
};
/** @brief 3 号电机（左后）：TIM8 CH1/CH2 + TIM3 编码器，正装 */
const dev_motor_t dev_motor_3 = {
    .pwm_fwd = &bsp_pwm_motor_3_fwd, .pwm_rev = &bsp_pwm_motor_3_rev,
    .encoder = &bsp_encoder_3, .cpr = 60000.0f, .reverse = 0u,
};
/** @brief 4 号电机（右后）：TIM8 CH3/CH4 + TIM4 编码器，镜像安装 */
const dev_motor_t dev_motor_4 = {
    .pwm_fwd = &bsp_pwm_motor_4_fwd, .pwm_rev = &bsp_pwm_motor_4_rev,
    .encoder = &bsp_encoder_4, .cpr = 60000.0f, .reverse = 1u,
};

/*==============================================================================
 * 内部变量
 *============================================================================*/
/** @brief 电机句柄表（顺序即槽位） */
static const dev_motor_t *const s_motor_list[] = {
    &dev_motor_1, &dev_motor_2, &dev_motor_3, &dev_motor_4,
};

/** @brief 是否已初始化过（幂等标记：统一初始化在调度前执行一次，守卫防将来重复调用） */
static uint8_t s_inited = 0u;

/*==============================================================================
 * 初始化与枚举
 *============================================================================*/
/**
 * @brief 启动所有电机的两路 PWM 并把占空比归零，同时启动编码器
 * @retval 0: 全部成功（含"已经初始化过"的情况）
 * @retval -1: 句柄表里有空指针或 cpr 非法
 * @note 幂等：重复调用只执行第一次。本函数在 freertos.c 的调度前统一调用一次；
 *       守卫给将来的重复调用兜底——否则会把电机归零、把编码器计数清掉，
 *       正在跑的速度环会丢一拍。
 */
int Dev_Motor_Init(void)
{
    if (s_inited != 0u) {
        return 0;
    }

    for (uint8_t i = 0u; i < ARRAY_SIZE(s_motor_list); i++) {
        const dev_motor_t *motor = s_motor_list[i];

        if ((motor == NULL) || (motor->pwm_fwd == NULL)
            || (motor->pwm_rev == NULL) || (motor->encoder == NULL)) {
            return -1;
        }
        if (motor->cpr <= 0.0f) {
            return -1;
        }

        /* 先归零再启动，避免定时器被别的通道带起来时输出旧值 */
        (void)Dev_Motor_Set_Duty(motor, 0.0f);
        BSP_PWM_Start(motor->pwm_fwd);
        BSP_PWM_Start(motor->pwm_rev);
        BSP_Encoder_Reset(motor->encoder);
    }

    s_inited = 1u;

    return 0;
}

/**
 * @brief 取电机台数
 * @retval 已实例化的电机数量
 */
uint8_t Dev_Motor_Count(void)
{
    return (uint8_t)ARRAY_SIZE(s_motor_list);
}

/**
 * @brief 按槽位取电机句柄
 * @param index 槽位，0 ~ Dev_Motor_Count()-1
 * @retval 非 NULL: 电机句柄
 * @retval NULL: 槽位越界
 */
const dev_motor_t *Dev_Motor_Get(uint8_t index)
{
    if (index >= ARRAY_SIZE(s_motor_list)) {
        return NULL;
    }

    return s_motor_list[index];
}

/*==============================================================================
 * 输出
 *============================================================================*/
/**
 * @brief 设置电机输出
 * @param motor 电机句柄，可为 NULL
 * @param duty  占空比 -100.0 ~ +100.0
 * @retval 0: 已输出
 * @retval -1: 句柄为 NULL
 */
int Dev_Motor_Set_Duty(const dev_motor_t *motor, float duty)
{
    if ((motor == NULL) || (motor->pwm_fwd == NULL) || (motor->pwm_rev == NULL)) {
        return -1;
    }

    /* 镜像安装的电机：驱动方向与"轮子往前"相反，先在这里换算成物理方向 */
    if (motor->reverse != 0u) {
        duty = -duty;
    }

    if (duty > 0.0f) {
        BSP_PWM_Set_Duty_Percent(motor->pwm_fwd, duty);
        BSP_PWM_Set_Duty_Percent(motor->pwm_rev, 0.0f);
    } else if (duty < 0.0f) {
        BSP_PWM_Set_Duty_Percent(motor->pwm_fwd, 0.0f);
        BSP_PWM_Set_Duty_Percent(motor->pwm_rev, -duty);
    } else {
        BSP_PWM_Set_Duty_Percent(motor->pwm_fwd, 0.0f);
        BSP_PWM_Set_Duty_Percent(motor->pwm_rev, 0.0f);
    }

    return 0;
}

/**
 * @brief 停止这台电机（两路占空比归零）
 * @param motor 电机句柄，可为 NULL
 * @retval 无
 */
void Dev_Motor_Stop(const dev_motor_t *motor)
{
    (void)Dev_Motor_Set_Duty(motor, 0.0f);
}

/*==============================================================================
 * 编码器
 *============================================================================*/
/**
 * @brief 读本周期的计数增量并清零计数器（测速用）
 * @param motor 电机句柄，不可为 NULL
 * @param delta 输出增量，不可为 NULL
 * @retval 0: 成功
 * @retval -1: 参数为 NULL
 */
int Dev_Motor_Read_Delta(const dev_motor_t *motor, int16_t *delta)
{
    int16_t raw = 0;

    if ((motor == NULL) || (delta == NULL)) {
        return -1;
    }

    if (BSP_Encoder_Read_Delta(motor->encoder, &raw) != 0) {
        return -1;
    }

    /* 与 Set_Duty 同一套极性：正占空比 ⇒ 正增量 */
    *delta = (motor->reverse != 0u) ? (int16_t)(-raw) : raw;
    return 0;
}

/**
 * @brief 读累计计数但不清零（标定线数用）
 * @param motor 电机句柄，不可为 NULL
 * @param raw   输出累计计数，不可为 NULL
 * @retval 0: 成功
 * @retval -1: 参数为 NULL
 */
int Dev_Motor_Read_Raw(const dev_motor_t *motor, int16_t *raw)
{
    int16_t value = 0;

    if ((motor == NULL) || (raw == NULL)) {
        return -1;
    }

    if (BSP_Encoder_Read_Raw(motor->encoder, &value) != 0) {
        return -1;
    }

    *raw = (motor->reverse != 0u) ? (int16_t)(-value) : value;
    return 0;
}

/**
 * @brief 把编码器计数器清零
 * @param motor 电机句柄，可为 NULL
 * @retval 无
 */
void Dev_Motor_Reset_Count(const dev_motor_t *motor)
{
    if (motor == NULL) {
        return;
    }

    BSP_Encoder_Reset(motor->encoder);
}

/**
 * @brief 取输出轴每转脉冲数（含减速比）
 * @param motor 电机句柄，可为 NULL
 * @retval 每转脉冲数；句柄为 NULL 时返回 0
 */
float Dev_Motor_Cpr(const dev_motor_t *motor)
{
    return (motor != NULL) ? motor->cpr : 0.0f;
}
