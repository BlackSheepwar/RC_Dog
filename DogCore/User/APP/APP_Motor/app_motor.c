/**
 * @file app_motor.c
 * @brief 电机控制（APP层）实现 - PID调速 + 速度闭环控制
 * @author 李嘉图
 * @date 2026-07-05
 *
 * @note 架构说明：
 *       APP_Motor 是电机控制的中间层，对上提供速度设置接口，
 *       对下调用 PID 算法、编码器 BSP 和 PWM BSP。
 *
 *       每个电机使用 AT8236 驱动芯片，控制方式为双PWM：
 *         正转 → 正向PWM输出占空比，反向PWM保持0
 *         反转 → 反向PWM输出占空比，正向PWM保持0
 *         停止 → 两个PWM均为0
 *
 *       速度闭环流程（每 MOTOR_PID_TICK_MS 执行一次）：
 *         编码器读数 → 换算速度% → PID增量计算 → 限幅 → PWM输出
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include <stdint.h>
#include <math.h>
#include <string.h>
#include "app_motor.h"
// 功能包含
#include "pid.h"
#include "bsp_encoder.h"
#include "bsp_pwm.h"

/*==============================================================================
 * 电机硬件映射表
 *
 * 将 Motor_ID_t 映射到具体的 PWM 通道和编码器。
 * 每个电机使用两个 PWM 通道（正转/反转）驱动 AT8236。
 *
 *            PWM正向    PWM反向    编码器
 *   左前腿   pwm_id=9  pwm_id=10  encoder=0 (TIM1)
 *   右前腿   pwm_id=11 pwm_id=12  encoder=1 (TIM2)
 *   左后腿   pwm_id=13 pwm_id=14  encoder=2 (TIM3)
 *   右后腿   pwm_id=15 pwm_id=16  encoder=3 (TIM4)
 *============================================================================*/
typedef struct {
    uint8_t  motor_id;          /* 电机编号                              */
    uint8_t  pwm_fwd;           /* 正转 PWM ID（BSP_PWM 寻址用）         */
    uint8_t  pwm_rev;           /* 反转 PWM ID（BSP_PWM 寻址用）         */
    uint8_t  encoder_id;        /* 编码器索引（BSP_Encoder 寻址用）      */
    float    encoder_cpr;       /* 每转定时器计数值（PPR×4, STM32编码器4倍频） */
    float    gear_ratio;        /* 减速箱减速比                          */
} Motor_HwMap_t;

static const Motor_HwMap_t MOTOR_HW_MAP[MOTOR_NUM] = {

    { .motor_id = MOTOR_LF,
        .pwm_fwd = 9,  .pwm_rev = 10,
        .encoder_id = 0,
        .encoder_cpr = 2000.0f,
        .gear_ratio = 30.0f },

    { .motor_id = MOTOR_RF,
        .pwm_fwd = 11, .pwm_rev = 12,
        .encoder_id = 1,
        .encoder_cpr = 2000.0f,
        .gear_ratio = 30.0f },

    { .motor_id = MOTOR_LB,
        .pwm_fwd = 13, .pwm_rev = 14,
        .encoder_id = 2,
        .encoder_cpr = 2000.0f,
        .gear_ratio = 30.0f },

    { .motor_id = MOTOR_RB,
        .pwm_fwd = 15, .pwm_rev = 16,
        .encoder_id = 3,
        .encoder_cpr = 2000.0f,
        .gear_ratio = 30.0f },
};

/*==============================================================================
 * 电机运行时实例结构体
 *============================================================================*/
typedef struct {
    PID_Handle_t   pid;                    /* PID 控制器实例               */
    float          target_speed;           /* 目标速度（缓存，与pid同步）  */
    uint8_t        enable;                 /* 启用标志：1=参与调度         */
} Motor_Instance_t;

/* 电机实例池 */
static Motor_Instance_t motor_pool[MOTOR_NUM];

/*==============================================================================
 * 内部工具函数
 *============================================================================*/
/**
 * @brief 根据 motor_id 查找硬件映射
 * @param motor_id 电机编号
 * @retval 非 NULL：找到
 * @retval NULL：未找到
 */
static const Motor_HwMap_t *Motor_FindMap(uint8_t motor_id)
{
    for (uint8_t i = 0; i < MOTOR_NUM; i++)
    {
        if (MOTOR_HW_MAP[i].motor_id == motor_id)
            return &MOTOR_HW_MAP[i];
    }
    return NULL;
}

/**
 * @brief 设置 AT8236 双PWM输出
 * @param map  电机硬件映射
 * @param duty 占空比（-100.0 ~ 100.0，负值=反转，0=停止）
 *
 * @note  正转：正向PWM = duty，反向PWM = 0
 *        反转：正向PWM = 0，反向PWM = |duty|
 *        停止：两个PWM均为0
 */
static void Motor_SetPWM(const Motor_HwMap_t *map, float duty)
{
    if (map == NULL) return;

    if (duty > 0.0f)
    {
        /* 正转 */
        BSP_PWM_SetDuty(map->pwm_fwd,  duty);
        BSP_PWM_SetDuty(map->pwm_rev,  0.0f);
    }
    else if (duty < 0.0f)
    {
        /* 反转 */
        BSP_PWM_SetDuty(map->pwm_fwd,  0.0f);
        BSP_PWM_SetDuty(map->pwm_rev, -duty);
    }
    else
    {
        /* 停止 */
        BSP_PWM_SetDuty(map->pwm_fwd,  0.0f);
        BSP_PWM_SetDuty(map->pwm_rev,  0.0f);
    }
}

/**
 * @brief 将编码器计数值换算为速度百分比
 * @param map   电机硬件映射（含编码器线数、减速比）
 * @param count 编码器计数值（每 PID 周期增量）
 * @return 换算后的速度（-100.0 ~ 100.0）
 *
 * @note 公式：speed = count × 100 / (gear_ratio × encoder_cpr × dt)
 *       负值表示反转，正值表示正转。
 */
static float Motor_CountToSpeed(const Motor_HwMap_t *map, int16_t count)
{
    float denominator = map->gear_ratio * map->encoder_cpr * MOTOR_PID_TICK_S;
    if (denominator < 0.001f) return 0.0f;

    return (float)count * 100.0f / denominator;
}

/*==============================================================================
 * 初始化函数
 *============================================================================*/
/**
 * @brief 电机 APP 层初始化
 * @note 清空电机池，初始化所有电机 PID 句柄，启动 PWM 输出。
 *       需在 RTOS 任务启动前调用一次。
 */
void APP_Motor_Init(void)
{
    BSP_PWM_Init();
    /* 清空电机池 */
    memset(motor_pool, 0, sizeof(motor_pool));

    for (uint8_t i = 0; i < MOTOR_NUM; i++)
    {
        const Motor_HwMap_t *map = &MOTOR_HW_MAP[i];
        Motor_Instance_t *inst = &motor_pool[i];

        /* 初始化 PID 句柄（缺省参数，后续可通过 APP_Motor_SetPID 调整） */
        PID_Init(&inst->pid, 0.5f, 0.4f, 0.0f, MOTOR_PID_TICK_S);
        inst->target_speed = 0.0f;
        inst->enable = 1;

        /* 启动正/反向 PWM 通道 */
        BSP_PWM_Start(map->pwm_fwd);
        BSP_PWM_Start(map->pwm_rev);

        /* 初始输出 0（停止） */
        Motor_SetPWM(map, 0.0f);
    }
}

/*==============================================================================
 * 控制函数
 *============================================================================*/
/**
 * @brief 设置电机目标速度
 * @param motor_id 电机编号（Motor_ID_t）
 * @param speed    目标速度（-MOTOR_SPEED_MAX ~ +MOTOR_SPEED_MAX）
 */
void APP_Motor_SetSpeed(uint8_t motor_id, float speed)
{
    if (motor_id >= MOTOR_NUM) return;

    /* 限幅 */
    if (speed >  MOTOR_SPEED_MAX) speed =  MOTOR_SPEED_MAX;
    if (speed < -MOTOR_SPEED_MAX) speed = -MOTOR_SPEED_MAX;

    Motor_Instance_t *inst = &motor_pool[motor_id];
    inst->target_speed = speed;

    /* 同步到 PID 目标值 */
    PID_SetTarget(&inst->pid, speed);
}

/**
 * @brief 获取电机当前实际速度
 * @param motor_id 电机编号
 * @param speed    输出速度值的指针
 * @retval 1: 成功
 * @retval 0: 失败
 */
uint8_t APP_Motor_GetSpeed(uint8_t motor_id, float *speed)
{
    if (motor_id >= MOTOR_NUM || speed == NULL)
        return 0;

    *speed = motor_pool[motor_id].pid.actual;
    return 1;
}

/**
 * @brief 设置电机 PID 参数
 * @param motor_id 电机编号
 * @param Kp       比例系数
 * @param Ki       积分系数
 * @param Kd       微分系数
 * @retval 1: 成功
 * @retval 0: 失败
 */
uint8_t APP_Motor_SetPID(uint8_t motor_id, float Kp, float Ki, float Kd)
{
    if (motor_id >= MOTOR_NUM) return 0;

    PID_SetParam(&motor_pool[motor_id].pid, Kp, Ki, Kd);
    return 1;
}

/**
 * @brief 紧急停止所有电机
 */
void APP_Motor_StopAll(void)
{
    for (uint8_t i = 0; i < MOTOR_NUM; i++)
    {
        const Motor_HwMap_t *map = &MOTOR_HW_MAP[i];
        Motor_Instance_t *inst = &motor_pool[i];

        /* 重置 PID 状态 */
        PID_Reset(&inst->pid);
        inst->target_speed = 0.0f;

        /* PWM 输出 0 */
        Motor_SetPWM(map, 0.0f);
    }
}

/*==============================================================================
 * PID 调度函数（由 TASK 层周期调用）
 *============================================================================*/
/**
 * @brief 电机 PID 闭环调度
 * @note 每 MOTOR_PID_TICK_MS 调用一次，完成一次完整的闭环控制周期：
 *
 *       1. 编码器读数 → 2. 速度换算 → 3. PID增量计算 →
 *       4. 输出累加 + 限幅 → 5. PWM输出
 *
 *       停止检测：当目标速度为 0 且实际速度为 0 且输出 < 5 时，
 *       强制输出为 0 并重置误差历史，防止静摩擦导致积分累积。
 */
void APP_Motor_RunPID(void)
{
    for (uint8_t i = 0; i < MOTOR_NUM; i++)
    {
        Motor_Instance_t *inst = &motor_pool[i];
        if (inst->enable == 0) continue;

        const Motor_HwMap_t *map = &MOTOR_HW_MAP[i];

        /* 1. 读取编码器计数值 */
        int16_t count = 0;
        if (BSP_Encoder_GetCount(map->encoder_id, &count) == 0)
            continue;

        /* 2. 换算为实际速度 */
        float actual = Motor_CountToSpeed(map, count);

        /* 3. 执行增量式 PID */
        PID_IncrementControl(&inst->pid, actual);

        /* 4. 输出限幅到 ±100 */
        float out = inst->pid.out;
        if (out >  MOTOR_SPEED_MAX) out =  MOTOR_SPEED_MAX;
        if (out < -MOTOR_SPEED_MAX) out = -MOTOR_SPEED_MAX;

        /* 5. 停止检测：目标=0 且 实际≈0 且 输出较小 → 强制归零 */
        if (inst->target_speed == 0.0f &&
            fabsf(actual) < 0.5f &&
            fabsf(out) < 5.0f)
        {
            out = 0.0f;
            PID_Reset(&inst->pid);
        }

        /* 6. 更新 PID 输出值（限幅后的值） */
        inst->pid.out = out;
        inst->pid.actual = actual;

        /* 7. 写入 PWM 硬件 */
        Motor_SetPWM(map, out);
    }
}
