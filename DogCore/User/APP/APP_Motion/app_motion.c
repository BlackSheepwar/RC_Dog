/**
 * @file app_motion.c
 * @brief 单腿运动控制层实现 — 百分比速度 + 角度到达检测
 * @author 李嘉图
 * @date 2026-06-30
 *
 * @note 核心功能：
 *       - 每条腿独立的目标角度 + 百分比速度 [1-100]
 *       - 两个关节自动同步到达（最慢全速，快的配速）
 *       - 每关节维护状态池：current_angle / target_angle / speed / reached
 *       - Motion_Scheduler() 通过读取舵机实际位置做角度到达检测
 *       - 速度计算后直接交给舵机层平滑到位，不再逐 tick 插值
 *
 *       关节映射注册表（Joint Map）：
 *       每条腿每个关节独立配置校准偏移、方向反转、输入限位。
 *       Motion_SetTarget() 入口为用户/IK 坐标系，
 *       内部自动映射为舵机坐标系再下发。
 *
 *       架构关系：
 *       IK/用户 → Motion_SetTarget(用户角度, speed)
 *               → Motion_MapToServo()  [校准+反转+限幅]
 *               → APP_Servo_SetTarget(舵机角度)
 *       Motion_Scheduler() → 读舵机位置 → 角度到达判定
 *       APP_Servo_Scheduler() → 后台平滑到位
 *
 * @warning 使用前必须先调用 Motion_Init() 初始化
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include <string.h>
#include "app_motion.h"
// 功能包含
#include "bsp_sys.h"       /* BSP_GetTickMs */
#include "app_servo.h"

/*==============================================================================
 * 腿配置注册表（仿 APP_Servo_GetById 模式）
 *
 * 每条腿独立配置：腿ID + 两个关节的全部映射参数（舵机绑定 + 速度上限 + 校准 + 反转 + 限位）。
 * 映射顺序（正向）：
 *   1. 限幅到 [limit_neg, limit_pos]
 *   2. 加校准偏移 calibration（先偏）
 *   3. 方向反转（reverse=1 时取反）（后翻）
 *
 * 公式： servo = reverse ? -(input + calibration) : (input + calibration)
 * 反向： input = reverse ? (-servo - calibration) : (servo - calibration)
 *
 * 典型用途：
 *   calibration = -90 表示舵机 0° 在用户坐标系中是 90°
 *   reverse    = 1   表示舵机安装方向相反
 *============================================================================*/
/** @brief 腿配置结构体（腿ID + 两个关节的全部映射参数） */
typedef struct {
    uint8_t leg_id;            /**< 腿编号 */
    uint8_t servo_id[MOTION_JOINTS_PER_LEG];       /**< [髋, 膝] 舵机逻辑 ID */
    float   speed_max[MOTION_JOINTS_PER_LEG];        /**< [髋, 膝] 最快运动速度(°/s) */
    int16_t calibration[MOTION_JOINTS_PER_LEG];      /**< [髋, 膝] 校准偏移(°)：用户→舵机坐标差值 */
    uint8_t reverse[MOTION_JOINTS_PER_LEG];          /**< [髋, 膝] 方向反转: 0=正转, 1=反转 */
    int16_t limit_pos[MOTION_JOINTS_PER_LEG];        /**< [髋, 膝] 输入正限位(°) */
    int16_t limit_neg[MOTION_JOINTS_PER_LEG];        /**< [髋, 膝] 输入负限位(°) */
} leg_cfg_t;

/** @brief 腿配置初始化表（编译期填充到运行时池） */
static const leg_cfg_t s_leg_cfg_init[MOTION_MAX_LEGS] = {
    {
        .leg_id = 1,
        .servo_id    = {1, 2},
        .speed_max   = {200.0f, 200.0f},
        .calibration = {-90, -90},
        .reverse     = {1, 1},
        .limit_pos   = {180, 180},
        .limit_neg   = {0, 0},
    },
    {
        .leg_id = 2,
        .servo_id    = {3, 4},
        .speed_max   = {200.0f, 200.0f},
        .calibration = {0, 0},
        .reverse     = {0, 0},
        .limit_pos   = {130, 130},
        .limit_neg   = {-130, -130},
    },
    {
        .leg_id = 3,
        .servo_id    = {5, 6},
        .speed_max   = {200.0f, 200.0f},
        .calibration = {0, 0},
        .reverse     = {0, 0},
        .limit_pos   = {130, 130},
        .limit_neg   = {-130, -130},
    },
    {
        .leg_id = 4,
        .servo_id    = {7, 8},
        .speed_max   = {200.0f, 200.0f},
        .calibration = {0, 0},
        .reverse     = {0, 0},
        .limit_pos   = {130, 130},
        .limit_neg   = {-130, -130},
    },
};

/**
 * @brief 根据腿编号获取配置（直接查 s_leg_cfg_init 常量表）
 * @param leg_id 腿编号（1～MOTION_MAX_LEGS）
 * @retval 非NULL：找到常量指针
 * @retval NULL：越界
 */
static inline const leg_cfg_t *Motion_GetLegCfg(uint8_t leg_id)
{
    if (leg_id < 1 || leg_id > MOTION_MAX_LEGS) return NULL;
    return &s_leg_cfg_init[leg_id - 1];
}

/**
 * @brief 腿关节 → 舵机 ID
 * @param leg_id  腿编号
 * @param joint   关节索引 [0=hip, 1=knee]
 * @return 舵机逻辑编号，查不到时返回 0
 */
static inline uint8_t Motion_GetServoId(uint8_t leg_id, uint8_t joint)
{
    const leg_cfg_t *cfg = Motion_GetLegCfg(leg_id);
    if (cfg == NULL) return 0;
    return cfg->servo_id[joint];
}

/**
 * @brief 取关节最高运动速度
 * @param leg_id  腿编号
 * @param joint   关节索引
 * @return 速度值(°/s)，为 0 时返回保底值
 */
static inline float Motion_GetSpeedMax(uint8_t leg_id, uint8_t joint)
{
    const leg_cfg_t *cfg = Motion_GetLegCfg(leg_id);
    if (cfg == NULL) return 0.0f;
    return cfg->speed_max[joint];
}

/*==============================================================================
 * 角度映射辅助函数
 *============================================================================*/
/**
 * @brief 用户/IK 角度 → 舵机角度（正向映射）
 * @param leg_id  腿编号
 * @param joint   关节索引
 * @param input   用户坐标系角度(°)
 * @return 映射后的舵机空间角度(°)
 * @note 映射顺序：限幅 → 校准偏移 → 方向反转
 *       先偏后翻：reverse ? -(input + cal) : (input + cal)
 *       这样校准用于修正零位，反转用于修正安装朝向
 */
static inline int16_t Motion_MapToServo(uint8_t leg_id, uint8_t joint, int16_t input)
{
    const leg_cfg_t *cfg = Motion_GetLegCfg(leg_id);
    if (cfg == NULL) return input;

    int16_t out = input;

    /* 1. 输入限幅 */
    if (out > cfg->limit_pos[joint]) out = cfg->limit_pos[joint];
    if (out < cfg->limit_neg[joint]) out = cfg->limit_neg[joint];

    /* 2. 校准偏移（先偏） */
    out += cfg->calibration[joint];

    /* 3. 方向反转（后翻） */
    if (cfg->reverse[joint]) out = -out;

    return out;
}

/**
 * @brief 舵机角度 → 用户/IK 角度（反向映射）
 * @param leg_id  腿编号
 * @param joint   关节索引
 * @param servo   舵机空间角度(°)
 * @return 用户坐标系角度(°)
 * @note 与正向映射对称：先反转归位 → 再减校准
 *       用于从舵机读数恢复 IK 坐标系的值；不做限幅
 */
static inline int16_t Motion_MapToUser(uint8_t leg_id, uint8_t joint, int16_t servo)
{
    const leg_cfg_t *cfg = Motion_GetLegCfg(leg_id);
    if (cfg == NULL) return servo;

    int16_t in = servo;

    /* 1. 方向反转（先翻回来） */
    if (cfg->reverse[joint]) in = -in;

    /* 2. 减校准偏移 */
    in -= cfg->calibration[joint];

    return in;
}

/*==============================================================================
 * 静态数据
 *============================================================================*/
/** @brief 四条腿的运动状态池（数组下标 = leg_id - 1） */
static motion_leg_t s_legs[MOTION_MAX_LEGS];

/**
 * @brief 根据腿编号获取运动状态
 * @param leg_id 腿编号（1～MOTION_MAX_LEGS）
 * @retval 非NULL：找到，返回指针
 * @retval NULL：未找到或未注册
 */
static inline motion_leg_t *Motion_GetLeg(uint8_t leg_id)
{
    if (leg_id < 1 || leg_id > MOTION_MAX_LEGS) return NULL;
    uint8_t idx = leg_id - 1;
    if (!s_legs[idx].registered) return NULL;
    return &s_legs[idx];
}

/*==============================================================================
 * 初始化函数
 *============================================================================*/
/**
 * @brief 初始化运动控制模块
 * @details
 *       - 清零所有腿的运动状态
 *       - 标记所有腿为已注册
 *       - 从舵机层读取当前角度填入状态池（current_angle = target_angle）
 * @note 必须在 Motion_Scheduler 之前调用，舵机层初始化之后
 */
void Motion_Init(void)
{
    memset(s_legs, 0, sizeof(s_legs));

    for (uint8_t i = 0; i < MOTION_MAX_LEGS; i++)
    {
        s_legs[i].registered = 1;
        uint8_t leg_id = i + 1;
        for (uint8_t j = 0; j < MOTION_JOINTS_PER_LEG; j++)
        {
            uint8_t sid = Motion_GetServoId(leg_id, j);
            int16_t pos = APP_Servo_GetCurrent(sid);
            s_legs[i].current_angle[j] = Motion_MapToUser(leg_id, j, pos);
            s_legs[i].target_angle[j]  = s_legs[i].current_angle[j];
            s_legs[i].reached[j] = 1;
        }
    }
}

/*==============================================================================
 * 运动控制函数
 *============================================================================*/
/**
 * @brief 设置腿的目标关节角度（百分比速度同步）
 * @param leg_id        腿编号 [1, MOTION_MAX_LEGS]
 * @param target_angle  目标角度数组 [hip_angle, knee_angle]，单位°
 * @param speed         速度 [0.1 ~ 100.0]，浮点精度 0.1
 *
 * @details
 *       1. 从舵机层读取当前实际角度作为起点
 *       2. 将用户坐标系角度 → 舵机坐标系
 *          a. 限幅到 [limit_neg, limit_pos]
 *          b. 方向反转（reverse=1 时取反）
 *          c. 加校准偏移 calibration
 *       3. 在舵机空间计算各关节 delta
 *       4. 由 speed 计算实际运动时间：
 *          max_need = max(delta / (speed_max * speed / 100))
 *       5. 快的关节降速配平，保证两关节同步到达
 *       6. 更新腿状态池（target_angle / speed / reached）
 *
 * @note
 *       - speed=100.0 时各关节按 speed_max 全速运动
 *       - speed 会被钳位到 [0.1, 100.0]
 *       - 浮点精度 0.1，舵机层支持浮点速度，控速更精准
 *       - 映射参数在 s_leg_cfg_init（腿配置注册表）中配置
 */
void Motion_SetTarget(uint8_t leg_id, const int16_t target_angle[MOTION_JOINTS_PER_LEG],
                      float speed)
{
    if (target_angle == NULL) return;
    motion_leg_t *leg = Motion_GetLeg(leg_id);
    if (leg == NULL) return;

    /* ── 钳位速度 ── */
    if (speed <= 0.0f)   speed = 0.1f;
    if (speed > 100.0f)  speed = 100.0f;
    float speed_ratio = speed / 100.0f;

    /* ── 扇区1：读当前舵机位置，算映射后 delta ── */
    int16_t target_servo[MOTION_JOINTS_PER_LEG];
    int16_t delta[MOTION_JOINTS_PER_LEG];
    uint32_t max_need = 0;

    for (uint8_t j = 0; j < MOTION_JOINTS_PER_LEG; j++)
    {
        uint8_t  sid   = Motion_GetServoId(leg_id, j);
        int16_t  start = APP_Servo_GetCurrent(sid);           /* 当前舵机空间位置 */
        target_servo[j] = Motion_MapToServo(leg_id, j, target_angle[j]); /* 用户→舵机 */

        /* ── 更新状态池（用户空间值） ── */
        leg->current_angle[j] = Motion_MapToUser(leg_id, j, start);
        leg->target_angle[j]  = target_angle[j];
        leg->reached[j] = 0;

        int16_t diff = target_servo[j] - start;
        delta[j] = (diff > 0) ? diff : -diff;

        if (delta[j] != 0)
        {
            float j_speed = Motion_GetSpeedMax(leg_id, j) * speed_ratio;
            uint32_t need_t = (uint32_t)((float)delta[j] / j_speed * 1000.0f + 0.5f);
            if (need_t > max_need) max_need = need_t;
        }
    }

    /* ── 实际时间 = max(1ms, 最慢关节按速度计算的时间) ── */
    uint32_t actual_duration = (max_need > 0) ? max_need : 1;

    /* ── 扇区2：计算速度，设舵机目标（舵机空间值） ── */
    for (uint8_t j = 0; j < MOTION_JOINTS_PER_LEG; j++)
    {
        uint8_t sid = Motion_GetServoId(leg_id, j);

        float j_speed;
        float j_speed_max = Motion_GetSpeedMax(leg_id, j);
        if (delta[j] == 0)
            j_speed = j_speed_max;                   /* 已到位，维持快速 */
        else
            j_speed = (float)delta[j] / (float)actual_duration * 1000.0f;

        if (j_speed > j_speed_max)
            j_speed = j_speed_max;

        leg->speed[j] = j_speed;

        APP_Servo_SetSpeed(sid, j_speed);
        APP_Servo_SetTarget(sid, target_servo[j]); /* ← 舵机空间终值（已映射） */
    }

    /* ── 扇区3：更新腿级状态 ── */
    leg->finish_tick = BSP_GetTickMs() + actual_duration;
    leg->moving = 1;

    if (max_need == 0) leg->moving = 0;           /* δ 全零 → 立即完成 */
}

/**
 * @brief 强制腿瞬间跳转到角度（无插值）
 * @param leg_id        腿编号
 * @param target_angle  目标角度数组 [hip_angle, knee_angle]
 *
 * @details
 *       - 一次性设舵机最终目标 + 高速
 *       - 更新状态池全部标记为已到达
 *       - moving 标记立即清零（不阻塞调度器）
 *
 * @warning 舵机内部仍有自己的平滑（最快速度逼近），并不是"瞬移"
 *          但本函数标记为已到位，不会阻塞调度器
 */
void Motion_SetImmediate(uint8_t leg_id, const int16_t target_angle[MOTION_JOINTS_PER_LEG])
{
    if (target_angle == NULL) return;
    motion_leg_t *leg = Motion_GetLeg(leg_id);
    if (leg == NULL) return;

    for (uint8_t j = 0; j < MOTION_JOINTS_PER_LEG; j++)
    {
        uint8_t  sid    = Motion_GetServoId(leg_id, j);
        int16_t  curr   = APP_Servo_GetCurrent(sid);
        int16_t  mapped = Motion_MapToServo(leg_id, j, target_angle[j]);

        /* ── 更新状态池 ── */
        leg->current_angle[j] = Motion_MapToUser(leg_id, j, curr);
        leg->target_angle[j] = target_angle[j];
        leg->speed[j] = Motion_GetSpeedMax(leg_id, j);
        leg->reached[j] = 1;

        APP_Servo_SetSpeed(sid, Motion_GetSpeedMax(leg_id, j));
        APP_Servo_SetTarget(sid, mapped);
    }

    leg->finish_tick = 0;
    leg->moving = 0;
}

/*==============================================================================
 * 状态查询函数
 *============================================================================*/
/**
 * @brief 查询指定腿是否正在运动
 * @param leg_id 腿编号
 * @return 1=运动中, 0=已到位或未注册
 */
uint8_t Motion_IsMoving(uint8_t leg_id)
{
    motion_leg_t *leg = Motion_GetLeg(leg_id);
    if (leg == NULL) return 0;
    return leg->moving;
}

/**
 * @brief 查询是否有任何腿在运动
 * @return 1=至少一条腿运动中, 0=全部静止
 */
uint8_t Motion_IsAnyMoving(void)
{
    for (uint8_t i = 0; i < MOTION_MAX_LEGS; i++)
    {
        if (s_legs[i].registered && s_legs[i].moving)
            return 1;
    }
    return 0;
}

/**
 * @brief 停止指定腿的运动（冻结在当前舵机位置）
 * @param leg_id 腿编号
 *
 * @details
 *       - 清零 moving 标记
 *       - 读舵机当前角度 → 同步更新状态池
 *       - 下次 Motion_SetTarget 会从该位置重新出发
 */
void Motion_Stop(uint8_t leg_id)
{
    motion_leg_t *leg = Motion_GetLeg(leg_id);
    if (leg == NULL) return;
    leg->moving = 0;

    /* 冻结舵机到当前实际位置 */
    for (uint8_t j = 0; j < MOTION_JOINTS_PER_LEG; j++)
    {
        uint8_t sid = Motion_GetServoId(leg_id, j);
        int16_t curr = APP_Servo_GetCurrent(sid);

        /* 同步状态池 */
        leg->current_angle[j] = Motion_MapToUser(leg_id, j, curr);
        leg->target_angle[j]  = leg->current_angle[j];
        leg->reached[j] = 1;

        APP_Servo_SetTarget(sid, curr);
    }
}

/**
 * @brief 停止所有腿的运动
 * @note 常用于步态切换或紧急停止
 */
void Motion_StopAll(void)
{
    for (uint8_t i = 0; i < MOTION_MAX_LEGS; i++)
        if (s_legs[i].registered)
            Motion_Stop(i + 1);  /* s_legs[i] ↔ leg_id = i + 1 */
}

/*==============================================================================
 * 调度器函数
 *============================================================================*/
/**
 * @brief 运动调度器（每10ms调用一次）
 * @details
 *       遍历所有腿，对正在运动的腿做逐关节角度到达检测：
 *       1. 读取舵机实际位置并转换到用户空间 → current_angle
 *       2. 与 target_angle 比较，误差 < ANGLE_REACHED_THRESHOLD 则标记 reached
 *       3. 两关节全部 reached → leg->moving = 0（调度器可推进相位）
 *
 *       ═══════════════════════════════════════════════════
 *       角度过渡由舵机层 APP_Servo_Scheduler() 后台平滑完成，
 *       Motion_Scheduler 仅做状态监测。
 *       ═══════════════════════════════════════════════════
 *
 * @note
 *       - 必须与 APP_Servo_Scheduler 同步在 10ms 任务中调用
 *       - MotionSched_Scheduler 会在本函数之后检查相位推进
 */
void Motion_Scheduler(void)
{
    for (uint8_t i = 0; i < MOTION_MAX_LEGS; i++)
    {
        motion_leg_t *leg = &s_legs[i];
        if (!leg->registered || !leg->moving)
            continue;

        uint8_t leg_id = i + 1;
        uint8_t all_reached = 1;

        for (uint8_t j = 0; j < MOTION_JOINTS_PER_LEG; j++)
        {
            uint8_t  sid       = Motion_GetServoId(leg_id, j);
            int16_t  servo_pos = APP_Servo_GetCurrent(sid);
            leg->current_angle[j] = Motion_MapToUser(leg_id, j, servo_pos);

            /* 计算到达误差 */
            int16_t diff = leg->target_angle[j] - leg->current_angle[j];
            if (diff < 0) diff = -diff;

            if (diff <= ANGLE_REACHED_THRESHOLD)
                leg->reached[j] = 1;
            else
                all_reached = 0;
        }

        if (all_reached)
            leg->moving = 0;
    }
}

/**
 * @brief 读取腿的当前实际关节角度（从舵机层）
 * @param leg_id    腿编号
 * @param out_angle 输出数组 [hip_angle, knee_angle]
 * @note 用于 IK 层或外部协议获取当前姿态
 */
void Motion_GetCurrentAngles(uint8_t leg_id, int16_t *out_angle)
{
    if (out_angle == NULL) return;
    if (Motion_GetLeg(leg_id) == NULL) return;

    for (uint8_t j = 0; j < MOTION_JOINTS_PER_LEG; j++)
    {
        uint8_t  sid   = Motion_GetServoId(leg_id, j);
        int16_t  servo = APP_Servo_GetCurrent(sid);
        out_angle[j]   = Motion_MapToUser(leg_id, j, servo); /* 舵机→用户空间 */
    }
}

/**
 * @brief 获取腿级状态池指针（只读）
 * @param leg_id 腿编号
 * @return const 指针，可用以读取 current_angle/target_angle/speed/reached
 */
const motion_leg_t *Motion_GetLegState(uint8_t leg_id)
{
    return Motion_GetLeg(leg_id);
}

