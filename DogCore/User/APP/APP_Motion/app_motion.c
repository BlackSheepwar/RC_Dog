/**
 * @file app_motion.c
 * @brief 单腿运动控制实现：角度映射 + 百分比速度同步 + 到位检测
 * @author Ricardo
 * @date 2026-11-01
 *
 * @note 核心功能：
 *       - 每条腿独立的目标角度 + 百分比速度 [0.1-100]
 *       - 两个关节自动同步到达（最慢的全速，快的配速）
 *       - 每条腿维护状态池：current_angle / target_angle / speed / reached
 *       - APP_Motion_Scheduler() 读舵机实际位置做角度到达检测
 *       - 速度算出来后直接交给舵机层平滑到位，本层不逐 tick 插值
 *
 *       关节映射注册表（Joint Map）：
 *       每条腿每个关节独立配置校准偏移、方向反转、输入限位。
 *       APP_Motion_Set_Target() 入口为用户/IK 坐标系，
 *       内部自动映射为舵机坐标系再下发。
 *
 *       架构关系：
 *       IK/用户 → APP_Motion_Set_Target(用户角度, speed)
 *               → APP_Motion_MapToServo()  [校准+反转+限幅]
 *               → Svc_Servo_Motion_Set_Target(舵机角度)
 *       APP_Motion_Scheduler() → 读舵机位置 → 角度到达判定
 *       Svc_Servo_Motion_Tick() → 后台平滑到位
 *
 * @warning 使用前必须先调用 APP_Motion_Init() 初始化
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include <stddef.h>
#include <string.h>
#include "app_motion.h"
// 功能包含
#include "bsp_sys.h"       /* BSP_SYS_Get_Tick_Ms */
#include "svc_servo_motion.h"     /* 运动句柄 */
#include "dev_servo.h"           /* 安装表里存设备句柄 */

/*==============================================================================
 * 腿配置注册表（与 Dev_Servo 的安装表同构：一条腿一条记录）
 *
 * 每条腿独立配置：腿ID + 两个关节的全部映射参数（舵机绑定 + 速度上限 + 校准 + 反转 + 限位）。
 * 映射顺序（正向）：
 *   1. 限幅到 [limit_neg, limit_pos]
 *   2. 预方向反转 pre_reverse（pre_rev=1 时取反）
 *   3. 加校准偏移 calibration（后偏）
 *   4. 后方向反转 post_reverse（post_rev=1 时再取反）
 *
 * 公式： servo = post_rev ? -((pre_rev ? -input : input) + cal)
 *                      :   ((pre_rev ? -input : input) + cal)
 * 反向： input = pre_rev  ? -((post_rev ? -servo : servo) - cal)
 *                      :    ((post_rev ? -servo : servo) - cal)
 *
 * 典型用途：
 *   calibration = -90 表示舵机 0° 在用户坐标系中是 90°
 *   post_rev    = 1   表示舵机安装方向相反（输出侧反转）
 *============================================================================*/
/** @brief 腿配置结构体（腿ID + 两个关节的全部映射参数） */
typedef struct {
    uint8_t leg_id;            /**< 腿编号 */
    const dev_servo_t *servo[APP_MOTION_JOINTS_PER_LEG]; /**< [髋, 膝] 舵机设备句柄 */
    float   speed_max[APP_MOTION_JOINTS_PER_LEG];        /**< [髋, 膝] 最快运动速度(°/s) */
    int16_t calibration[APP_MOTION_JOINTS_PER_LEG];      /**< [髋, 膝] 校准偏移(°)：用户→舵机坐标差值 */
    uint8_t pre_reverse[APP_MOTION_JOINTS_PER_LEG];      /**< [髋, 膝] 预反转（输入侧）: 0=正转, 1=反转 */
    uint8_t post_reverse[APP_MOTION_JOINTS_PER_LEG];     /**< [髋, 膝] 后反转（输出侧）: 0=正转, 1=反转 */
    int16_t limit_pos[APP_MOTION_JOINTS_PER_LEG];        /**< [髋, 膝] 输入正限位(°) */
    int16_t limit_neg[APP_MOTION_JOINTS_PER_LEG];        /**< [髋, 膝] 输入负限位(°) */
    common_ik_link_t link;                              /**< 杆长（mm），逆解用 */
} leg_cfg_t;

/** @brief 腿配置初始化表（编译期填充到运行时池） */
static const leg_cfg_t s_leg_cfg_init[APP_MOTION_MAX_LEGS] = {
    {
        .leg_id = 1,
        .servo     = {&dev_servo_1, &dev_servo_2},
        .speed_max   = {200.0f, 200.0f},
        .calibration = {-90, -90},
        .pre_reverse  = {0, 1},
        .post_reverse = {0, 1},
        .limit_pos   = {180, 0},
        .limit_neg   = {0, -180},
        .link        = { .l1 = 140.0f, .l2 = 140.0f },
    },
    {
        .leg_id = 2,
        .servo     = {&dev_servo_3, &dev_servo_4},
        .speed_max   = {200.0f, 200.0f},
        .calibration = {-90, -90},
        .pre_reverse  = {0, 1},
        .post_reverse = {0, 1},
        .limit_pos   = {180, 0},
        .limit_neg   = {0, -180},
        .link        = { .l1 = 140.0f, .l2 = 140.0f },
    },
        {
        .leg_id = 3,
        .servo     = {&dev_servo_5, &dev_servo_6},
        .speed_max   = {200.0f, 200.0f},
        .calibration = {-90, -90},
        .pre_reverse  = {0, 1},
        .post_reverse = {0, 1},
        .limit_pos   = {180, 0},
        .limit_neg   = {0, -180},
        .link        = { .l1 = 140.0f, .l2 = 140.0f },
    },
    {
        .leg_id = 4,
        .servo     = {&dev_servo_7, &dev_servo_8},
        .speed_max   = {200.0f, 200.0f},
        .calibration = {-90, -90},
        .pre_reverse  = {0, 1},
        .post_reverse = {0, 1},
        .limit_pos   = {180, 0},
        .limit_neg   = {0, -180},
        .link        = { .l1 = 140.0f, .l2 = 140.0f },
    },

};

/**
 * @brief 根据腿编号获取配置（直接查 s_leg_cfg_init 常量表）
 * @param leg_id 腿编号（1～APP_MOTION_MAX_LEGS）
 * @retval 非NULL：找到常量指针
 * @retval NULL：越界
 */
static inline const leg_cfg_t *APP_Motion_GetLegCfg(uint8_t leg_id)
{
    if (leg_id < 1 || leg_id > APP_MOTION_MAX_LEGS) return NULL;
    return &s_leg_cfg_init[leg_id - 1];
}

/**
 * @brief 腿关节 → 舵机设备句柄
 * @param leg_id  腿编号
 * @param joint   关节索引 [0=hip, 1=knee]
 * @retval 非 NULL: 该关节的舵机设备句柄
 * @retval NULL: 腿号或关节号非法
 */
static inline const dev_servo_t *APP_Motion_GetServo(uint8_t leg_id, uint8_t joint)
{
    const leg_cfg_t *cfg = APP_Motion_GetLegCfg(leg_id);

    if (cfg == NULL) {
        return NULL;
    }

    return cfg->servo[joint];
}

/**
 * @brief 取关节最高运动速度
 * @param leg_id  腿编号
 * @param joint   关节索引
 * @retval 大于 0: 该关节的最高角速度（°/s）
 * @retval 0.0f: 腿号或关节号非法
 */
static inline float APP_Motion_GetSpeedMax(uint8_t leg_id, uint8_t joint)
{
    const leg_cfg_t *cfg = APP_Motion_GetLegCfg(leg_id);
    if (cfg == NULL) return 0.0f;
    return cfg->speed_max[joint];
}

/**
 * @brief 取某条腿的杆长（逆解用）
 * @param leg_id 腿编号，1 ~ APP_MOTION_MAX_LEGS
 * @retval 非 NULL: 该腿的杆长
 * @retval NULL: 腿号非法
 */
const common_ik_link_t *APP_Motion_Get_Link(uint8_t leg_id)
{
    const leg_cfg_t *cfg = APP_Motion_GetLegCfg(leg_id);

    if (cfg == NULL) {
        return NULL;
    }

    return &cfg->link;
}

/*==============================================================================
 * 角度映射辅助函数
 *============================================================================*/
/**
 * @brief 用户/IK 角度 → 舵机角度（正向映射）
 * @param leg_id  腿编号
 * @param joint   关节索引
 * @param input   用户坐标系角度(°)（浮点，亚度精度）
 * @retval 映射后的舵机空间角度（°）
 * @note 映射顺序：限幅 → pre_rev → +cal → post_rev
 *       servo = post_rev ? -((pre_rev ? -input : input) + cal)
 *                       :   ((pre_rev ? -input : input) + cal)
 */
static inline float APP_Motion_MapToServo(uint8_t leg_id, uint8_t joint, float input)
{
    const leg_cfg_t *cfg = APP_Motion_GetLegCfg(leg_id);
    if (cfg == NULL) return input;

    float out = input;

    /* 1. 输入限幅 */
    if (out > (float)cfg->limit_pos[joint]) out = (float)cfg->limit_pos[joint];
    if (out < (float)cfg->limit_neg[joint]) out = (float)cfg->limit_neg[joint];

    /* 2. 预方向反转（输入侧） */
    if (cfg->pre_reverse[joint]) out = -out;

    /* 3. 校准偏移 */
    out += (float)cfg->calibration[joint];

    /* 4. 后方向反转（输出侧） */
    if (cfg->post_reverse[joint]) out = -out;

    return out;
}

/**
 * @brief 舵机角度 → 用户/IK 角度（反向映射）
 * @param leg_id  腿编号
 * @param joint   关节索引
 * @param servo   舵机空间角度(°)（浮点）
 * @retval 用户坐标系角度（°）
 * @note 与正向映射对称：undo post_rev → -cal → undo pre_rev
 *       input = pre_rev ? -((post_rev ? -servo : servo) - cal)
 *                      :   ((post_rev ? -servo : servo) - cal)
 *       用于从舵机读数恢复 IK 坐标系的值；不做限幅
 */
static inline float APP_Motion_MapToUser(uint8_t leg_id, uint8_t joint, float servo)
{
    const leg_cfg_t *cfg = APP_Motion_GetLegCfg(leg_id);
    if (cfg == NULL) return servo;

    float in = servo;

    /* 1. 撤销后方向反转 */
    if (cfg->post_reverse[joint]) in = -in;

    /* 2. 减校准偏移 */
    in -= (float)cfg->calibration[joint];

    /* 3. 撤销预方向反转 */
    if (cfg->pre_reverse[joint]) in = -in;

    return in;
}

/*==============================================================================
 * 类型定义
 *============================================================================*/
/**
 * @brief 单条腿的运行时状态（布局只在本文件，头文件只有名字）
 * @note 关节数组按 [髋, 膝] 顺序；reached 是每个关节的到位标志，
 *       moving 是腿级汇总，调度层只看 moving。
 */
struct app_motion_leg {
    svc_servo_motion_t *motion[APP_MOTION_JOINTS_PER_LEG]; /* 舵机运动句柄（Init 时挂上） */
    float    current_angle[APP_MOTION_JOINTS_PER_LEG]; /* 当前角度（用户坐标系，度） */
    float    target_angle[APP_MOTION_JOINTS_PER_LEG];  /* 目标角度（用户坐标系，度） */
    float    speed[APP_MOTION_JOINTS_PER_LEG];         /* 当前角速度（度/秒） */
    uint8_t  reached[APP_MOTION_JOINTS_PER_LEG];       /* 1: 该关节已到位 */
    uint32_t finish_tick;                              /* 预计完成时刻（ms） */
    uint8_t  registered;                               /* 1: 已注册 */
    uint8_t  moving;                                   /* 1: 至少一个关节未到位 */
};

/*==============================================================================
 * 静态数据
 *============================================================================*/
/** @brief 四条腿的运动状态池（数组下标 = leg_id - 1） */
static app_motion_leg_t s_legs[APP_MOTION_MAX_LEGS];

/**
 * @brief 取某个关节的舵机运动句柄
 * @param leg_id 腿编号，1 ~ APP_MOTION_MAX_LEGS
 * @param joint  关节索引 [0=hip, 1=knee]
 * @retval 非 NULL: 运动句柄
 * @retval NULL: 腿号/关节号非法，或该腿尚未 Init
 */
static svc_servo_motion_t *APP_Motion_Motion(uint8_t leg_id, uint8_t joint)
{
    if ((leg_id < 1u) || (leg_id > APP_MOTION_MAX_LEGS)
        || (joint >= APP_MOTION_JOINTS_PER_LEG)) {
        return NULL;
    }

    return s_legs[leg_id - 1u].motion[joint];
}

/**
 * @brief 根据腿编号获取运动状态
 * @param leg_id 腿编号（1～APP_MOTION_MAX_LEGS）
 * @retval 非NULL：找到，返回指针
 * @retval NULL：未找到或未注册
 */
static inline app_motion_leg_t *APP_Motion_GetLeg(uint8_t leg_id)
{
    if (leg_id < 1 || leg_id > APP_MOTION_MAX_LEGS) return NULL;
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
 *       - 把安装表里的舵机设备句柄换成运动句柄（编号从此不进本模块）
 *       - 从运动服务读取当前角度填入状态池（current_angle = target_angle）
 * @note 必须在 Svc_Servo_Motion_Init() 之后、APP_Motion_Scheduler 之前调用。
 *       设备句柄查不到运动句柄说明该舵机没被服务接管，直接跳过该关节。
 */
void APP_Motion_Init(void)
{
    memset(s_legs, 0, sizeof(s_legs));

    for (uint8_t i = 0; i < APP_MOTION_MAX_LEGS; i++)
    {
        uint8_t leg_id = i + 1;

        s_legs[i].registered = 1;

        for (uint8_t j = 0; j < APP_MOTION_JOINTS_PER_LEG; j++)
        {
            /* 设备句柄 → 运动句柄：这是"编号"消失的地方 */
            s_legs[i].motion[j] = Svc_Servo_Motion_Of(APP_Motion_GetServo(leg_id, j));

            s_legs[i].current_angle[j] =
                APP_Motion_MapToUser(leg_id, j, Svc_Servo_Motion_Get_Current(s_legs[i].motion[j]));
            s_legs[i].target_angle[j] = s_legs[i].current_angle[j];
            s_legs[i].reached[j]      = 1;
        }
    }
}

/*==============================================================================
 * 运动控制函数
 *============================================================================*/
/**
 * @brief 设置腿的目标关节角度（百分比速度同步）
 * @param leg_id        腿编号 [1, APP_MOTION_MAX_LEGS]
 * @param target_angle  目标角度数组 [hip_angle, knee_angle]，单位°
 * @param speed         速度 [0.1 ~ 100.0]，浮点精度 0.1
 *
 * @details
 *       1. 从舵机层读取当前实际角度作为起点
 *       2. 将用户坐标系角度 → 舵机坐标系
 *          a. 限幅到 [limit_neg, limit_pos]
 *          b. 预方向反转（pre_reverse）
 *          c. 加校准偏移 calibration
 *          d. 后方向反转（post_reverse）
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
void APP_Motion_Set_Target(uint8_t leg_id, const float target_angle[APP_MOTION_JOINTS_PER_LEG],
                      float speed)
{
    if (target_angle == NULL) return;
    app_motion_leg_t *leg = APP_Motion_GetLeg(leg_id);
    if (leg == NULL) return;

    /* ── 钳位速度 ── */
    if (speed <= 0.0f)   speed = 0.1f;
    if (speed > 100.0f)  speed = 100.0f;
    float speed_ratio = speed / 100.0f;

    /* ── 扇区1：读当前舵机位置，算映射后 delta ── */
    float target_servo[APP_MOTION_JOINTS_PER_LEG];
    float delta[APP_MOTION_JOINTS_PER_LEG];
    uint32_t max_need = 0;

    for (uint8_t j = 0; j < APP_MOTION_JOINTS_PER_LEG; j++)
    {
        svc_servo_motion_t *sm = APP_Motion_Motion(leg_id, j);
        float    start = Svc_Servo_Motion_Get_Current(sm);           /* 当前舵机空间位置 */
        target_servo[j] = APP_Motion_MapToServo(leg_id, j, target_angle[j]); /* 用户→舵机 */

        /* ── 更新状态池（用户空间值） ── */
        leg->current_angle[j] = APP_Motion_MapToUser(leg_id, j, start);
        leg->target_angle[j]  = target_angle[j];
        leg->reached[j] = 0;

        float diff = target_servo[j] - start;
        delta[j] = (diff > 0.0f) ? diff : -diff;

        if (delta[j] != 0.0f)
        {
            float j_speed = APP_Motion_GetSpeedMax(leg_id, j) * speed_ratio;
            uint32_t need_t = (uint32_t)(delta[j] / j_speed * 1000.0f + 0.5f);
            if (need_t > max_need) max_need = need_t;
        }
    }

    /* ── 实际时间 = max(1ms, 最慢关节按速度计算的时间) ── */
    uint32_t actual_duration = (max_need > 0) ? max_need : 1;

    /* ── 扇区2：计算速度，设舵机目标（舵机空间值） ── */
    for (uint8_t j = 0; j < APP_MOTION_JOINTS_PER_LEG; j++)
    {
        svc_servo_motion_t *sm = APP_Motion_Motion(leg_id, j);

        float j_speed;
        float j_speed_max = APP_Motion_GetSpeedMax(leg_id, j);
        if (delta[j] == 0.0f)
            j_speed = j_speed_max;                   /* 已到位，维持快速 */
        else
            j_speed = (float)delta[j] / (float)actual_duration * 1000.0f;

        if (j_speed > j_speed_max)
            j_speed = j_speed_max;

        leg->speed[j] = j_speed;

        Svc_Servo_Motion_Set_Speed(sm, j_speed);
        Svc_Servo_Motion_Set_Target(sm, target_servo[j]); /* ← 舵机空间终值（已映射） */
    }

    /* ── 扇区3：更新腿级状态 ── */
    leg->finish_tick = BSP_SYS_Get_Tick_Ms() + actual_duration;
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
void APP_Motion_Set_Immediate(uint8_t leg_id, const float target_angle[APP_MOTION_JOINTS_PER_LEG])
{
    if (target_angle == NULL) return;
    app_motion_leg_t *leg = APP_Motion_GetLeg(leg_id);
    if (leg == NULL) return;

    for (uint8_t j = 0; j < APP_MOTION_JOINTS_PER_LEG; j++)
    {
        svc_servo_motion_t *sm = APP_Motion_Motion(leg_id, j);
        float    curr   = Svc_Servo_Motion_Get_Current(sm);
        float    mapped = APP_Motion_MapToServo(leg_id, j, target_angle[j]);

        /* ── 更新状态池 ── */
        leg->current_angle[j] = APP_Motion_MapToUser(leg_id, j, curr);
        leg->target_angle[j] = target_angle[j];
        leg->speed[j] = APP_Motion_GetSpeedMax(leg_id, j);
        leg->reached[j] = 1;

        Svc_Servo_Motion_Set_Speed(sm, APP_Motion_GetSpeedMax(leg_id, j));
        Svc_Servo_Motion_Set_Target(sm, mapped);
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
 * @retval 1: 运动中
 * @retval 0: 已到位或腿号非法
 */
uint8_t APP_Motion_Is_Moving(uint8_t leg_id)
{
    app_motion_leg_t *leg = APP_Motion_GetLeg(leg_id);
    if (leg == NULL) return 0;
    return leg->moving;
}

/**
 * @brief 查询是否有任何腿在运动
 * @retval 1: 至少一条腿在运动
 * @retval 0: 全部静止
 */
uint8_t APP_Motion_Is_Any_Moving(void)
{
    for (uint8_t i = 0; i < APP_MOTION_MAX_LEGS; i++)
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
 *       - 下次 APP_Motion_Set_Target 会从该位置重新出发
 */
void APP_Motion_Stop(uint8_t leg_id)
{
    app_motion_leg_t *leg = APP_Motion_GetLeg(leg_id);
    if (leg == NULL) return;
    leg->moving = 0;

    /* 冻结舵机到当前实际位置 */
    for (uint8_t j = 0; j < APP_MOTION_JOINTS_PER_LEG; j++)
    {
        svc_servo_motion_t *sm = APP_Motion_Motion(leg_id, j);
        float curr = Svc_Servo_Motion_Get_Current(sm);

        /* 同步状态池 */
        leg->current_angle[j] = APP_Motion_MapToUser(leg_id, j, curr);
        leg->target_angle[j]  = leg->current_angle[j];
        leg->reached[j] = 1;

        Svc_Servo_Motion_Set_Target(sm, curr);
    }
}

/**
 * @brief 停止所有腿的运动
 * @note 常用于步态切换或紧急停止
 */
void APP_Motion_Stop_All(void)
{
    for (uint8_t i = 0; i < APP_MOTION_MAX_LEGS; i++)
        if (s_legs[i].registered)
            APP_Motion_Stop(i + 1);  /* s_legs[i] ↔ leg_id = i + 1 */
}

/*==============================================================================
 * 调度器函数
 *============================================================================*/
/**
 * @brief 运动调度器（每10ms调用一次）
 * @details
 *       遍历所有腿，对正在运动的腿做逐关节角度到达检测：
 *       1. 读取舵机实际位置并转换到用户空间 → current_angle
 *       2. 与 target_angle 比较，误差 < APP_MOTION_REACHED_DEG 则标记 reached
 *       3. 两关节全部 reached → leg->moving = 0（调度器可推进相位）
 *
 *       ═══════════════════════════════════════════════════
 *       角度过渡由舵机层 Svc_Servo_Motion_Tick() 后台平滑完成，
 *       APP_Motion_Scheduler 仅做状态监测。
 *       ═══════════════════════════════════════════════════
 *
 * @note
 *       - 必须与 APP_Servo_Scheduler 同步在 10ms 任务中调用
 *       - APP_Motion_Sched_Scheduler 会在本函数之后检查相位推进
 */
void APP_Motion_Scheduler(void)
{
    for (uint8_t i = 0; i < APP_MOTION_MAX_LEGS; i++)
    {
        app_motion_leg_t *leg = &s_legs[i];
        if (!leg->registered || !leg->moving)
            continue;

        uint8_t leg_id = i + 1;
        uint8_t all_reached = 1;

        for (uint8_t j = 0; j < APP_MOTION_JOINTS_PER_LEG; j++)
        {
            svc_servo_motion_t *sm = APP_Motion_Motion(leg_id, j);
            float    servo_pos = Svc_Servo_Motion_Get_Current(sm);
            leg->current_angle[j] = APP_Motion_MapToUser(leg_id, j, servo_pos);

            /* 计算到达误差 */
            float diff = leg->target_angle[j] - leg->current_angle[j];
            if (diff < 0.0f) diff = -diff;

            if (diff <= (float)APP_MOTION_REACHED_DEG)
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
void APP_Motion_Get_Current_Angles(uint8_t leg_id, float *out_angle)
{
    if (out_angle == NULL) return;
    if (APP_Motion_GetLeg(leg_id) == NULL) return;

    for (uint8_t j = 0; j < APP_MOTION_JOINTS_PER_LEG; j++)
    {
        svc_servo_motion_t *sm = APP_Motion_Motion(leg_id, j);
        float    servo = Svc_Servo_Motion_Get_Current(sm);
        out_angle[j]   = APP_Motion_MapToUser(leg_id, j, servo); /* 舵机→用户空间 */
    }
}

