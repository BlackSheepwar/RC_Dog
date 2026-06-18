/**
 * @file app_gait.c
 * @brief 步态控制层：足端轨迹 → IK → 肢体调度 实现
 * @author 李嘉图
 * @date 2026-06-11
 *
 * @note 本模块是步态控制的"上层"：
 *       1) 维护 IK 步态相位推进（时间基，非 tick 计数）
 *       2) 每个相位通过 IK_TwoLink 将足端坐标转为关节角度
 *       3) 将关节角度写入 Limb_SetTarget，由 Limb 层做平滑插值
 *
 *       舵机→肢体的编译期映射表在 app_limb.c 中定义，
 *       本模块不再持有关节映射。
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
#include "app_gait.h"
#include "app_servo.h"      /* 显式声明依赖：Gait 层运行在 Servo 之上 */

/*==============================================================================
 * 静态池
 *============================================================================*/
static ik_runner_t s_ik;

/*==============================================================================
 * API 实现
 *============================================================================*/
/**
 * @brief 初始化 IK 步态控制器
 * @note 内部调用 Limb_Init()（含舵机映射配置与高速模式设定）。
 *       必须在 APP_Servo_Add 全部完成后调用。
 */
void Gait_IK_Init(void)
{
    /* ── 初始化 Limb 层（含舵机映射与高速设定）── */
    Limb_Init();

    /* ── 清空 IK 运行状态 ── */
    s_ik.sequence         = NULL;
    s_ik.phase_index      = 0;
    s_ik.phase_start_tick = 0;
    s_ik.active           = 0;
}

/**
 * @brief 辅助函数：对单个相位做 IK 解算并设肢体目标
 * @param phase       IK 相位指针
 * @note 先对所有腿做 IK 解算，全部可达才一起设置目标。
 *       任一腿 IK 失败时整组跳过，避免部分腿到位、部分停留在旧位置导致不平衡。
 */
static void Gait_IK_ApplyPhase(const ik_gait_phase_t *phase)
{
    int16_t targets[LEG_COUNT][LIMB_JOINT_COUNT];
    uint8_t all_valid = 1;

    /* 第一遍：遍历所有腿做 IK，确保全部可达 */
    for (uint8_t leg = 0; leg < LEG_COUNT; leg++)
    {
        float hip_rad, knee_rad;

        if (IK_TwoLink(LEG_L1, LEG_L2,
                       phase->legs[leg].x,
                       phase->legs[leg].z,
                       &hip_rad, &knee_rad))
        {
            targets[leg][0] = (int16_t)(hip_rad  * RAD_TO_DEG + 0.5f);
            targets[leg][1] = (int16_t)(knee_rad * RAD_TO_DEG + 0.5f);
        }
        else
        {
            all_valid = 0;
            break;      /* 任一腿不可达 → 整组跳过 */
        }
    }

    /* 第二遍：全部可达才设置肢体目标 */
    if (all_valid)
    {
        for (uint8_t leg = 0; leg < LEG_COUNT; leg++)
            Limb_SetTarget(leg, targets[leg], phase->duration_ms);
    }
}

/**
 * @brief 开始执行 IK 步态
 * @param gait 步态序列指针（指向静态常量，不拷贝）
 * @note 初始化相位索引，记录开始时间，
 *       立即通过 IK 解算第一个相位并设置肢体目标。
 */
void Gait_IK_Start(const ik_gait_sequence_t *gait)
{
    if (gait == NULL || gait->phase_count == 0)
        return;

    s_ik.sequence         = gait;
    s_ik.phase_index      = 0;
    s_ik.phase_start_tick = HAL_GetTick();
    s_ik.active           = 1;

    /* ── 立即应用第一个相位 ── */
    Gait_IK_ApplyPhase(&gait->phases[0]);
}

/**
 * @brief 停止 IK 步态
 * @note 同时停止所有肢体运动（Limb_StopAll），
 *       肢体停留在当前插值位置。
 */
void Gait_IK_Stop(void)
{
    s_ik.active = 0;
    Limb_StopAll();
}

/**
 * @brief 查询 IK 步态是否正在运行
 * @retval 1 运行中
 * @retval 0 空闲
 */
uint8_t Gait_IK_IsRunning(void)
{
    return s_ik.active;
}

/**
 * @brief IK 步态调度器（每 SERVO_TICK_MS 调用）
 * @note 每 tick 检查当前相位持续时间是否已到：
 *       - 未到：直接返回，由 Limb_Scheduler 继续插值
 *       - 已到：推进相位 → IK 解算 → Limb_SetTarget
 *
 *       相位推进逻辑：
 *       - 非循环序列到末尾后 active=0 自动停止
 *       - 循环序列回绕到 phase_index=0
 */
void Gait_IK_Scheduler(void)
{
    if (!s_ik.active || s_ik.sequence == NULL)
        return;

    /* ── 检查当前相位是否结束 ── */
    uint32_t now           = HAL_GetTick();
    uint32_t elapsed       = now - s_ik.phase_start_tick;
    uint32_t phase_duration = s_ik.sequence->phases[s_ik.phase_index].duration_ms;

    if (elapsed < phase_duration)
        return;     /* 当前相位未完成 */

    /* ── 推进到下一相位 ── */
    s_ik.phase_index++;

    if (s_ik.phase_index >= s_ik.sequence->phase_count)
    {
        if (s_ik.sequence->loop)
        {
            s_ik.phase_index = 0;   /* 循环 */
        }
        else
        {
            s_ik.active = 0;        /* 单次结束 */
            return;
        }
    }

    /* ── 计算新相位目标并设置 ── */
    s_ik.phase_start_tick = now;
    Gait_IK_ApplyPhase(&s_ik.sequence->phases[s_ik.phase_index]);
}

/*==============================================================================
 * 内置 IK 步态序列
 *
 * 注意：以下足端位置是参考值，基于 LEG_L1=140mm, LEG_L2=180mm 推算。
 * 实际使用时必须根据机械结构微调。
 *
 * 足端坐标系原点在髋关节：
 *   x = 前后（+前）
 *   z = 竖直（+下）
 *
 * 可达范围：L1+L2=320mm（全伸） ~ |L1-L2|≈40mm（全折）
 * 合理工作区约 z=60~300mm
 *============================================================================*/

/* ---------- 站立姿态 ----------
 * 双腿直立，膝关节微屈以保持稳定 */
static const ik_gait_phase_t PHASE_IK_STAND[] = {
    {
        .legs = {
            { .x =  0.0f,  .z = 85.0f },   /* 左腿 */
            { .x =  0.0f,  .z = 85.0f },   /* 右腿 */
        },
        .duration_ms = 100,     /* 100ms 过渡到站立 */
    },
};

const ik_gait_sequence_t GAIT_IK_STAND = {
    .phases      = PHASE_IK_STAND,
    .phase_count = 1,
    .loop        = 1,
};

/* ---------- 蹲下姿态 ----------
 * 双腿弯曲，足端靠近髋关节 */
static const ik_gait_phase_t PHASE_IK_SIT[] = {
    {
        .legs = {
            { .x =  0.0f,  .z = 45.0f },   /* 左腿弯曲 */
            { .x =  0.0f,  .z = 45.0f },   /* 右腿弯曲 */
        },
        .duration_ms = 500,     /* 500ms 过渡到蹲下 */
    },
};

const ik_gait_sequence_t GAIT_IK_SIT = {
    .phases      = PHASE_IK_SIT,
    .phase_count = 1,
    .loop        = 0,
};

/* ---------- 行走（4 相位 trot）----------
 *
 * 相位 0: 左腿抬起（收膝）→ 右腿支撑
 * 相位 1: 左腿前摆         → 右腿支撑
 * 相位 2: 右腿抬起（收膝）→ 左腿支撑
 * 相位 3: 右腿前摆         → 左腿支撑
 *
 * 支撑腿足端保持在髋关节稍前方，保证稳定。
 * 摆动腿先收膝（z 减小），再前伸（x 增大）。
 *============================================================================*/
static const ik_gait_phase_t PHASE_IK_WALK[] = {
    {
        .legs = {
            { .x =  0.0f,  .z = 55.0f },   /* 左腿：抬膝 */
            { .x = 15.0f,  .z = 80.0f },   /* 右腿：支撑 */
        },
        .duration_ms = 150,
    },
    {
        .legs = {
            { .x = 30.0f,  .z = 55.0f },   /* 左腿：前摆 */
            { .x =  0.0f,  .z = 80.0f },   /* 右腿：支撑后移 */
        },
        .duration_ms = 200,
    },
    {
        .legs = {
            { .x =  0.0f,  .z = 80.0f },   /* 左腿：落地支撑 */
            { .x =  0.0f,  .z = 55.0f },   /* 右腿：抬膝 */
        },
        .duration_ms = 150,
    },
    {
        .legs = {
            { .x = -10.0f, .z = 80.0f },   /* 左腿：支撑后移 */
            { .x =  30.0f, .z = 55.0f },   /* 右腿：前摆 */
        },
        .duration_ms = 200,
    },
};

const ik_gait_sequence_t GAIT_IK_WALK = {
    .phases      = PHASE_IK_WALK,
    .phase_count = 4,
    .loop        = 1,
};
