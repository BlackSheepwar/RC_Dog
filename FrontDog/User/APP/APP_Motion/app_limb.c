/**
 * @file app_limb.c
 * @brief 肢体控制层：时间基插值调度实现
 * @author 李嘉图
 * @date 2026-06-11
 *
 * @note 核心逻辑：每 tick 计算归一化进度 t = elapsed / duration，
 *       对每个关节做线性插值 angle = start + (target - start) × t。
 *
 *       舵机→肢体的映射为编译期 const 表 LIMB_SERVO_MAP，
 *       Limb_Init 预置所有肢体 registered=1 并设定舵机高速跟随。
 *
 *       使用 HAL_GetTick() 作为时间基准，
 *       无符号减法自动处理 49 天回绕（unsigned wrap-around）。
 */

#include "app_limb.h"
#include "app_limb_cfg.h"  /* LIMB_SERVO_MAP, LIMB_SERVO_SPEED_DPS */
#include "app_servo.h"
#include <string.h>

/*==============================================================================
 * 静态池
 *============================================================================*/
static limb_t s_limbs[LIMB_MAX];

/*==============================================================================
 * 初始化
 *============================================================================*/
/**
 * @brief 初始化肢体控制模块
 * @note 清零运行状态，预置所有肢体的 registered 标记，
 *       并设定各关节舵机为高速跟随模式。
 *       必须在 APP_Servo_Add 全部完成后调用。
 */
void Limb_Init(void)
{
    memset(s_limbs, 0, sizeof(s_limbs));

    /* ── 预置所有肢体为已注册 ── */
    for (uint8_t i = 0; i < LIMB_MAX; i++)
        s_limbs[i].registered = 1;

    /* ── 设定舵机高速跟随 ── */
    for (uint8_t i = 0; i < LIMB_MAX; i++)
        for (uint8_t j = 0; j < LIMB_JOINT_COUNT; j++)
            if (LIMB_SERVO_MAP[i][j] != 0)
                APP_Servo_SetSpeed(LIMB_SERVO_MAP[i][j], LIMB_SERVO_SPEED_DPS);
}

/*==============================================================================
 * 运动控制
 *============================================================================*/
/**
 * @brief 设置肢体目标角度（时间基插值）
 * @param limb_id     肢体编号
 * @param target      目标关节角度数组（长度 LIMB_JOINT_COUNT）
 * @param duration_ms 过渡时间（ms），最小 1ms
 * @note 自动快照当前角度为起始值，开始时间基插值。
 *       若 duration_ms 传 0 会被修正为 1ms。
 *       若目标与当前角度一致，立即标记到位（moving=0）。
 */
void Limb_SetTarget(uint8_t limb_id,
                    const int16_t target[LIMB_JOINT_COUNT],
                    uint32_t duration_ms)
{
    if (limb_id >= LIMB_MAX || !s_limbs[limb_id].registered || target == NULL)
        return;

    limb_t *limb = &s_limbs[limb_id];

    /* 快照当前角度为起始值 */
    for (uint8_t j = 0; j < LIMB_JOINT_COUNT; j++)
        limb->start_joint[j] = limb->current_joint[j];

    /* 设置目标 */
    for (uint8_t j = 0; j < LIMB_JOINT_COUNT; j++)
        limb->target_joint[j] = target[j];

    limb->duration_ms = (duration_ms > 0) ? duration_ms : 1;
    limb->start_tick  = HAL_GetTick();
    limb->moving      = 1;

    /* ── 若目标与当前恰好一致，立即标记到位 ── */
    uint8_t same = 1;
    for (uint8_t j = 0; j < LIMB_JOINT_COUNT; j++)
    {
        if (target[j] != limb->current_joint[j])
        {
            same = 0;
            break;
        }
    }
    if (same) limb->moving = 0;
}

/**
 * @brief 强制跳转到指定角度（瞬间到位，无插值）
 * @param limb_id 肢体编号
 * @param target  目标关节角度数组
 * @note 同时更新 current/start/target 三个快照，
 *       并立即通过 APP_Servo_SetTarget 输出到舵机。
 */
void Limb_SetImmediate(uint8_t limb_id,
                       const int16_t target[LIMB_JOINT_COUNT])
{
    if (limb_id >= LIMB_MAX || !s_limbs[limb_id].registered || target == NULL)
        return;

    limb_t *limb = &s_limbs[limb_id];

    for (uint8_t j = 0; j < LIMB_JOINT_COUNT; j++)
    {
        limb->current_joint[j] = target[j];
        limb->start_joint[j]   = target[j];
        limb->target_joint[j]  = target[j];
        APP_Servo_SetTarget(LIMB_SERVO_MAP[limb_id][j], target[j]);
    }

    limb->moving = 0;
}

/*==============================================================================
 * 状态查询
 *============================================================================*/
/**
 * @brief 查询指定肢体是否正在运动
 * @param limb_id 肢体编号
 * @retval 1 运动中
 * @retval 0 已到位或未注册
 */
uint8_t Limb_IsMoving(uint8_t limb_id)
{
    if (limb_id >= LIMB_MAX || !s_limbs[limb_id].registered)
        return 0;
    return s_limbs[limb_id].moving;
}

/**
 * @brief 查询是否有任何肢体在运动
 * @retval 1 至少一个肢体在运动中
 * @retval 0 全部静止
 */
uint8_t Limb_IsAnyMoving(void)
{
    for (uint8_t i = 0; i < LIMB_MAX; i++)
    {
        if (s_limbs[i].registered && s_limbs[i].moving)
            return 1;
    }
    return 0;
}

/**
 * @brief 获取肢体当前插值角度
 * @param limb_id 肢体编号
 * @param out     [out] 当前角度数组（长度 LIMB_JOINT_COUNT）
 * @note 若 limb_id 无效或未注册，out 不做写入（调用者需确保 out 有效）
 */
void Limb_GetCurrent(uint8_t limb_id, int16_t out[LIMB_JOINT_COUNT])
{
    if (limb_id >= LIMB_MAX || !s_limbs[limb_id].registered || out == NULL)
        return;

    for (uint8_t j = 0; j < LIMB_JOINT_COUNT; j++)
        out[j] = s_limbs[limb_id].current_joint[j];
}

/**
 * @brief 停止指定肢体的插值运动
 * @param limb_id 肢体编号
 * @note 保持在当前角度不动，下次 SetTarget 从当前位置开始
 */
void Limb_Stop(uint8_t limb_id)
{
    if (limb_id >= LIMB_MAX || !s_limbs[limb_id].registered)
        return;
    s_limbs[limb_id].moving = 0;
}

/**
 * @brief 停止所有肢体的插值运动
 * @note 常用于紧急停止或步态切换
 */
void Limb_StopAll(void)
{
    for (uint8_t i = 0; i < LIMB_MAX; i++)
        if (s_limbs[i].registered)
            s_limbs[i].moving = 0;
}

/*==============================================================================
 * 调度器（每 SERVO_TICK_MS 调用一次）
 *============================================================================*/
/**
 * @brief 肢体时间基插值调度器
 * @note 遍历所有注册的肢体：
 *       1. 计算 elapsed = now - start_tick
 *       2. t = min(elapsed / duration_ms, 1.0)
 *       3. 每个关节: angle = start + (target - start) × t
 *       4. 将 angle 写入 APP_Servo_SetTarget（查 LIMB_SERVO_MAP 转舵机 ID）
 *       5. t=1.0 时标记 moving=0
 *
 *       舵机层的 speed_dps 由 Limb_Init 设为高速（如 5000 dps），
 *       使单个 tick 内即可跟上 Limb 的目标值。
 */
void Limb_Scheduler(void)
{
    uint32_t now = HAL_GetTick();

    for (uint8_t i = 0; i < LIMB_MAX; i++)
    {
        limb_t *limb = &s_limbs[i];
        if (!limb->registered || !limb->moving)
            continue;

        /* ── 计算归一化进度 t ∈ [0, 1] ── */
        uint32_t elapsed = now - limb->start_tick;
        float t;
        if (elapsed >= limb->duration_ms)
            t = 1.0f;
        else
            t = (float)elapsed / (float)limb->duration_ms;

        /* ── 线性插值每个关节 ── */
        for (uint8_t j = 0; j < LIMB_JOINT_COUNT; j++)
        {
            float range = (float)(limb->target_joint[j] - limb->start_joint[j]);
            float angle = (float)limb->start_joint[j] + range * t;
            limb->current_joint[j] = (int16_t)(angle + 0.5f);

            APP_Servo_SetTarget(LIMB_SERVO_MAP[i][j], limb->current_joint[j]);
        }

        /* ── 到位标记 ── */
        if (t >= 1.0f)
            limb->moving = 0;
    }
}
