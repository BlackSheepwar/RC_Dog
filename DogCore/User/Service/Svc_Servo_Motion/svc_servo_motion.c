/**
 * @file svc_servo_motion.c
 * @brief 舵机运动服务实现：定速逼近 + 到位判定
 * @author Ricardo
 * @date 2026-11-01
 *
 * @note 句柄布局只在本文件：设备句柄 + 当前角 + 目标角 + 角速度 + 到位标志。
 *       句柄池是 static 的，Svc_Servo_Motion_Init 一次建好，
 *       上层拿到的是指向池内的 const 指针——只读，改状态必须走函数。
 *
 *       步长 = 角速度 × elapsed_ms / 1000，再夹到剩余差值：
 *       一次不会越过目标，因此不会出现"到了再往回修"的抖动。
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include <stddef.h>
#include "common.h"             /* ARRAY_SIZE */
#include "svc_servo_motion.h"

/*==============================================================================
 * 类型定义
 *============================================================================*/
/**
 * @brief 舵机运动句柄（一台一份）
 * @note 布局不外泄：头文件只有 typedef，上层拿不到成员。
 */
struct svc_servo_motion {
    const dev_servo_t *dev;        /* 设备句柄 */
    float              current;    /* 当前角（度） */
    float              target;     /* 目标角（度） */
    float              speed_dps;  /* 角速度（度/秒） */
    uint8_t            moving;     /* 1: 未到位 */
};

/*==============================================================================
 * 内部变量
 *============================================================================*/
/** @brief 句柄池 */
static svc_servo_motion_t s_pool[SVC_SERVO_MOTION_MAX];
/** @brief 已建立的槽位数 */
static uint8_t s_count = 0u;
/** @brief 是否已建立过（幂等标记：统一初始化在调度前执行一次，守卫防将来重复调用） */
static uint8_t s_inited = 0u;

/*==============================================================================
 * 内部函数
 *============================================================================*/
/**
 * @brief 把句柄的当前角输出到设备
 * @param motion 运动句柄，不可为 NULL
 * @retval 无
 */
static void Svc_Servo_Motion_Output(const svc_servo_motion_t *motion)
{
    if (motion->dev == NULL) {
        return;
    }

    (void)Dev_Servo_Set_Angle(motion->dev, motion->current);
}

/*==============================================================================
 * 建立与查找
 *============================================================================*/
/**
 * @brief 为每台已初始化的舵机建立运动槽位
 * @retval 0: 成功（含"已经建立过"的情况）
 * @retval -1: 设备数量为 0 或超过上限
 * @note 幂等：重复调用只执行第一次。本函数在 freertos.c 的调度前统一调用一次；
 *       守卫给将来的重复调用兜底——建立槽位时会把设备的当前角抄进
 *       current/target，重复执行会把这期间已下发的目标角抹掉。
 */
int Svc_Servo_Motion_Init(void)
{
    uint8_t dev_count = Dev_Servo_Count();

    if (s_inited != 0u) {
        return 0;
    }

    if ((dev_count == 0u) || (dev_count > SVC_SERVO_MOTION_MAX)) {
        return -1;
    }

    s_count = dev_count;

    for (uint8_t i = 0u; i < s_count; i++) {
        svc_servo_motion_t *motion = &s_pool[i];

        motion->dev       = Dev_Servo_Get(i);
        motion->current   = Dev_Servo_Get_Angle(motion->dev);
        motion->target    = motion->current;
        motion->speed_dps = SVC_SERVO_MOTION_DEFAULT_DPS;
        motion->moving    = 0u;
    }

    s_inited = 1u;

    return 0;
}

/**
 * @brief 取已建立的槽位数
 * @retval 槽位数
 */
uint8_t Svc_Servo_Motion_Count(void)
{
    return s_count;
}

/**
 * @brief 按顺序取句柄
 * @param index 0 ~ Count-1
 * @retval 非 NULL: 句柄
 * @retval NULL: 序号越界
 */
svc_servo_motion_t *Svc_Servo_Motion_Get(uint8_t index)
{
    if (index >= s_count) {
        return NULL;
    }

    return &s_pool[index];
}

/**
 * @brief 由设备句柄查出对应的运动句柄
 * @param dev 舵机设备句柄，可为 NULL
 * @retval 非 NULL: 运动句柄
 * @retval NULL: 设备为空或未被接管
 */
svc_servo_motion_t *Svc_Servo_Motion_Of(const dev_servo_t *dev)
{
    if (dev == NULL) {
        return NULL;
    }

    for (uint8_t i = 0u; i < s_count; i++) {
        if (s_pool[i].dev == dev) {
            return &s_pool[i];
        }
    }

    return NULL;
}

/*==============================================================================
 * 目标与参数
 *============================================================================*/
/**
 * @brief 设置目标角度
 * @param motion 运动句柄，可为 NULL
 * @param deg    目标角（度）
 * @retval 无
 */
void Svc_Servo_Motion_Set_Target(svc_servo_motion_t *motion, float deg)
{
    if (motion == NULL) {
        return;
    }

    /* 用设备的夹紧规则：限位只有一份，避免上层再算一遍算出不同的值 */
    motion->target = Dev_Servo_Clamp_Angle(motion->dev, deg);
}

/**
 * @brief 在当前目标角上叠加增量
 * @param motion 运动句柄，可为 NULL
 * @param delta  角度增量（度）
 * @retval 无
 */
void Svc_Servo_Motion_Step_Target(svc_servo_motion_t *motion, float delta)
{
    if (motion == NULL) {
        return;
    }

    motion->target =
        Dev_Servo_Clamp_Angle(motion->dev, motion->target + delta);
}

/**
 * @brief 设置角速度
 * @param motion 运动句柄，可为 NULL
 * @param dps    角速度（度/秒）
 * @retval 无
 */
void Svc_Servo_Motion_Set_Speed(svc_servo_motion_t *motion, float dps)
{
    if (motion == NULL) {
        return;
    }

    motion->speed_dps = (dps > 0.0f) ? dps : 0.0f;
}

/*==============================================================================
 * 查询
 *============================================================================*/
/**
 * @brief 取当前角
 * @param motion 运动句柄，可为 NULL
 * @retval 当前角（度）
 */
float Svc_Servo_Motion_Get_Current(const svc_servo_motion_t *motion)
{
    return (motion != NULL) ? motion->current : 0.0f;
}

/**
 * @brief 取目标角
 * @param motion 运动句柄，可为 NULL
 * @retval 目标角（度）
 */
float Svc_Servo_Motion_Get_Target(const svc_servo_motion_t *motion)
{
    return (motion != NULL) ? motion->target : 0.0f;
}

/**
 * @brief 查询是否还在动
 * @param motion 运动句柄，可为 NULL
 * @retval 1: 未到位
 * @retval 0: 已到位或句柄为空
 */
uint8_t Svc_Servo_Motion_Is_Moving(const svc_servo_motion_t *motion)
{
    return (motion != NULL) ? motion->moving : 0u;
}

/*==============================================================================
 * 推进
 *============================================================================*/
/**
 * @brief 推进一帧：所有已接管舵机朝各自目标走一步并输出
 * @param elapsed_ms 距上次调用的毫秒数
 * @retval 1: 至少有一台还没到位
 * @retval 0: 全部到位
 */
uint8_t Svc_Servo_Motion_Tick_All(uint32_t elapsed_ms)
{
    uint8_t any_moving = 0u;

    for (uint8_t i = 0u; i < s_count; i++) {
        svc_servo_motion_t *motion = &s_pool[i];
        float               diff;
        float               abs_diff;
        float               step;

        if (motion->dev == NULL) {
            continue;
        }

        /* ---------- 1. 算差值 ---------- */
        diff     = motion->target - motion->current;
        abs_diff = (diff >= 0.0f) ? diff : -diff;

        if (abs_diff <= SVC_SERVO_MOTION_REACHED_DEG) {
            /* ---------- 2a. 已到位：贴值并重发一次输出 ---------- */
            motion->current = motion->target;
            motion->moving  = 0u;
            Svc_Servo_Motion_Output(motion);
            continue;
        }

        /* ---------- 2b. 未到位：按角速度走一步，不越过目标 ---------- */
        step = motion->speed_dps * (float)elapsed_ms / 1000.0f;
        if (step > abs_diff) {
            step = abs_diff;
        }

        motion->current += (diff > 0.0f) ? step : -step;
        motion->moving   = 1u;
        any_moving       = 1u;

        /* ---------- 3. 输出 ---------- */
        Svc_Servo_Motion_Output(motion);
    }

    return any_moving;
}

/**
 * @brief 把所有舵机冻结在当前角度（目标 := 当前）
 * @retval 无
 */
void Svc_Servo_Motion_Hold_All(void)
{
    for (uint8_t i = 0u; i < s_count; i++) {
        s_pool[i].target = s_pool[i].current;
        s_pool[i].moving = 0u;
    }
}
