/**
 * @file app_servo_gait.c
 * @brief 步态控制器实现
 * @author 李嘉图
 * @date 2026-06-05
 *
 * @note 实现了步态相位推进 + 预定义步态（站立/蹲下/行走）。
 *       每个 tick 调用 Gait_Scheduler() 设置舵机目标角度，
 *       由 APP_Servo_Scheduler() 负责平滑逼近。
 *       步态参数可通过修改下方的常量表调整，不需要改调度逻辑。
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
#include "app_servo_gait.h"
#include "app_servo.h"
#include "common.h"

/*==============================================================================
 * 舵机ID → 腿关节映射
 *============================================================================*/
static const uint8_t LEG_SERVO_MAP[LEG_COUNT][SERVO_PER_LEG] = {
    {1, 2},   /* 左前腿: 髋, 膝 */
    {3, 4},   /* 右前腿: 髋, 膝 */
};

/*==============================================================================
 * 步态执行器状态
 *============================================================================*/
static gait_runner_t s_runner;

/*==============================================================================
 * API 实现
 *============================================================================*/
/**
 * @brief 初始化步态控制器
 */
void Gait_Init(void)
{
    s_runner.current_gait = NULL;
    s_runner.phase_index  = 0;
    s_runner.phase_tick   = 0;
    s_runner.active       = 0;
}

/**
 * @brief 开始执行步态
 * @param gait 步态序列指针（指向静态常量，不拷贝）
 */
void Gait_Start(const gait_sequence_t *gait)
{
    if (gait == NULL) return;
    s_runner.current_gait = gait;
    s_runner.phase_index  = 0;
    s_runner.phase_tick   = 0;
    s_runner.active       = 1;
}

/**
 * @brief 停止步态
 */
void Gait_Stop(void)
{
    s_runner.active = 0;
}

/**
 * @brief 查询是否正在执行步态
 * @retval 1 运行中
 * @retval 0 空闲
 */
uint8_t Gait_IsRunning(void)
{
    return s_runner.active;
}

/**
 * @brief 步态调度器（每 tick 调用）
 * @note 设置舵机目标角度，推进相位，处理循环/结束。
 *       在 task_servo 中先调 APP_Servo_Scheduler() 再调此函数。
 */
void Gait_Scheduler(void)
{
    if (!s_runner.active) return;

    const gait_sequence_t *gait = s_runner.current_gait;
    const gait_phase_t *phase = &gait->phases[s_runner.phase_index];

    /* 设置每条腿每个关节的目标角度 */
    for (uint8_t leg = 0; leg < LEG_COUNT; leg++)
    {
        for (uint8_t joint = 0; joint < SERVO_PER_LEG; joint++)
        {
            APP_Servo_SetTarget(LEG_SERVO_MAP[leg][joint],
                             phase->legs[leg].joint[joint]);
        }
    }

    /* 推进相位 */
    s_runner.phase_tick++;
    if (s_runner.phase_tick >= phase->duration_tick)
    {
        s_runner.phase_tick = 0;
        s_runner.phase_index++;

        if (s_runner.phase_index >= gait->phase_count)
        {
            if (gait->loop)
            {
                s_runner.phase_index = 0;   /* 自动循环 */
            }
            else
            {
                s_runner.active = 0;         /* 单次结束 */
            }
        }
    }
}

/*==============================================================================
 * 内置步态序列
 *
 * 注意：以下角度值是参考值，实际需要根据机械结构微调。
 *       角度范围 -125~+125，正值/负值的含义取决于舵机安装方向。
 *       每条腿2个关节: [0]=髋, [1]=膝
 *============================================================================*/

/* ---------- 站立姿态 ---------- */
static const gait_phase_t PHASE_STAND[] = {
    {
        .legs = {
            { .joint = {0, 0} },     /* 左腿：中立 */
            { .joint = {0, 0} },     /* 右腿：中立 */
        },
        .duration_tick = 10,             /* 100ms 过渡到站立 */
    },
};

const gait_sequence_t GAIT_STAND = {
    .phases       = PHASE_STAND,
    .phase_count  = 1,
    .loop         = 1,                  /* 持续保持 */
};

/* ---------- 蹲下姿态 ---------- */
static const gait_phase_t PHASE_SIT[] = {
    {
        .legs = {
            { .joint = {0, -60} },   /* 左腿：膝盖弯曲 */
            { .joint = {0, -60} },   /* 右腿：膝盖弯曲 */
        },
        .duration_tick = 50,             /* 500ms 过渡到蹲下 */
    },
};

const gait_sequence_t GAIT_SIT = {
    .phases       = PHASE_SIT,
    .phase_count  = 1,
    .loop         = 0,                  /* 单次过渡 */
};

/* ---------- 行走循环（4相位 trot） ----------
 *
 * 相位0: 左腿抬 → 右腿站立支撑
 * 相位1: 左腿前摆 → 右腿站立
 * 相位2: 右腿抬 → 左腿站立支撑
 * 相位3: 右腿前摆 → 左腿站立
 *============================================================================*/
static const gait_phase_t PHASE_WALK[] = {
    {
        .legs = {
            { .joint = {-20, 10} },  /* 左腿：抬（膝弯+踝摆） */
            { .joint = {0, 0} },     /* 右腿：中立支撑 */
        },
        .duration_tick = 15,         /* 150ms */
    },
    {
        .legs = {
            { .joint = {0, 5} },     /* 左腿：前摆（踝前伸） */
            { .joint = {0, 0} },     /* 右腿：中立支撑 */
        },
        .duration_tick = 20,         /* 200ms */
    },
    {
        .legs = {
            { .joint = {0, 0} },     /* 左腿：落地支撑 */
            { .joint = {-20, 10} },  /* 右腿：抬 */
        },
        .duration_tick = 15,         /* 150ms */
    },
    {
        .legs = {
            { .joint = {0, 0} },     /* 左腿：支撑 */
            { .joint = {0, 5} },     /* 右腿：前摆 */
        },
        .duration_tick = 20,         /* 200ms */
    },
};

const gait_sequence_t GAIT_WALK = {
    .phases       = PHASE_WALK,
    .phase_count  = 4,
    .loop         = 1,
};
