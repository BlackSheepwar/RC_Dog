/**
 * @file svc_motor_speed.c
 * @brief 电机速度环实现：增量式 PID + 双 PWM 占空比输出
 * @author Ricardo
 * @date 2026-11-01
 *
 * @note 一台电机一份句柄：设备句柄 + PID 上下文 + 目标值。
 *       句柄池是 static 的，Init 一次建好；上层拿到非 const 指针，
 *       但类型不完整，只能传回给本层函数，不会绕过接口改状态。
 *
 *       用增量式 PID（Common_Pid_Update_Increment）：
 *       现场参数按"Ki 项不带 dt"整定，换位置式会因 dt=0.01 让积分项缩 100 倍。
 *
 *       停止逻辑：目标为 0 且实测已进入停稳带，就把输出与积分一起清掉，
 *       否则残留下来的积分会一直顶着电机（现象是停不住、慢慢爬）。
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include <stddef.h>
#include "common.h"             /* ARRAY_SIZE */
#include "svc_motor_speed.h"
// 功能包含
#include "common_pid.h"

/*==============================================================================
 * 内部变量
 *============================================================================*/
/** @brief PID 输出限幅：±100 对应满占空比 */
static const common_pid_cfg_t s_pid_cfg = {
    .kp = SVC_MOTOR_SPEED_KP,
    .ki = SVC_MOTOR_SPEED_KI,
    .kd = SVC_MOTOR_SPEED_KD,
    .out_min = -100.0f,
    .out_max = 100.0f,
    .integral_min = -100.0f,
    .integral_max = 100.0f,
};

/*==============================================================================
 * 类型定义
 *============================================================================*/
/**
 * @brief 电机速度环句柄（一台一份）
 */
struct svc_motor_speed {
    const dev_motor_t *dev;           /* 设备句柄 */
    common_pid_t       pid;           /* PID 上下文 */
    float              target_speed;  /* 目标速度（停止判定用） */
};

/** @brief 句柄池 */
static svc_motor_speed_t s_pool[SVC_MOTOR_SPEED_MAX];
/** @brief 已建立的槽位数 */
static uint8_t s_count = 0u;
/** @brief 是否已建立过（幂等标记：初始化下放到各任务头部，可能被多处调用） */
static uint8_t s_inited = 0u;

/*==============================================================================
 * 内部函数
 *============================================================================*/
/**
 * @brief 编码器增量 → 速度刻度
 * @param speed      句柄，不可为 NULL
 * @param count      本周期编码器增量
 * @param elapsed_ms 本周期毫秒数，必须大于 0
 * @retval 速度刻度（1.0 ≈ 0.01 转/秒），符号跟随转向
 */
static float Svc_Motor_Speed_Count_To_Speed(const svc_motor_speed_t *speed,
                                            int16_t count, uint32_t elapsed_ms)
{
    float dt_s        = (float)elapsed_ms / 1000.0f;
    float denominator = Dev_Motor_Cpr(speed->dev) * dt_s;

    if (denominator < 0.001f) {
        return 0.0f;
    }

    return (float)count * 100.0f / denominator;
}

/*==============================================================================
 * 建立与查找
 *============================================================================*/
/**
 * @brief 为每台已初始化的电机建立速度环槽位并初始化 PID
 * @retval 0: 成功（含"已经建立过"的情况）
 * @retval -1: 设备数量为 0 或超过上限
 * @note 幂等：重复调用只执行第一次。本函数在 freertos.c 的调度前统一调用一次；
 *       守卫给将来的重复调用兜底——重复执行会把 PID 积分清掉，
 *       正在跑的闭环会突然丢输出。
 */
int Svc_Motor_Speed_Init(void)
{
    uint8_t dev_count = Dev_Motor_Count();

    if (s_inited != 0u) {
        return 0;
    }

    if ((dev_count == 0u) || (dev_count > SVC_MOTOR_SPEED_MAX)) {
        return -1;
    }

    s_count = dev_count;

    for (uint8_t i = 0u; i < s_count; i++) {
        svc_motor_speed_t *speed = &s_pool[i];

        speed->dev          = Dev_Motor_Get(i);
        speed->target_speed = 0.0f;
        (void)Common_Pid_Init(&speed->pid, &s_pid_cfg);
    }

    s_inited = 1u;

    return 0;
}

/**
 * @brief 取已建立的槽位数
 * @retval 槽位数
 */
uint8_t Svc_Motor_Speed_Count(void)
{
    return s_count;
}

/**
 * @brief 按顺序取句柄
 * @param index 0 ~ Count-1
 * @retval 非 NULL: 句柄
 * @retval NULL: 序号越界
 */
svc_motor_speed_t *Svc_Motor_Speed_Get(uint8_t index)
{
    if (index >= s_count) {
        return NULL;
    }

    return &s_pool[index];
}

/**
 * @brief 由设备句柄查出对应的速度环句柄
 * @param dev 电机设备句柄，可为 NULL
 * @retval 非 NULL: 速度环句柄
 * @retval NULL: 设备为空或未被接管
 */
svc_motor_speed_t *Svc_Motor_Speed_Of(const dev_motor_t *dev)
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
 * @brief 设置目标速度
 * @param speed 速度环句柄，可为 NULL
 * @param value 目标速度（刻度值）
 * @retval 无
 */
void Svc_Motor_Speed_Set_Target(svc_motor_speed_t *speed, float value)
{
    if (speed == NULL) {
        return;
    }

    if (value > SVC_MOTOR_SPEED_LIMIT) {
        value = SVC_MOTOR_SPEED_LIMIT;
    } else if (value < -SVC_MOTOR_SPEED_LIMIT) {
        value = -SVC_MOTOR_SPEED_LIMIT;
    }

    speed->target_speed = value;
    Common_Pid_Set_Target(&speed->pid, value);
}

/**
 * @brief 在线改 PID 增益
 * @param speed 速度环句柄，可为 NULL
 * @param kp    比例系数
 * @param ki    积分系数
 * @param kd    微分系数
 * @retval 0: 成功
 * @retval -1: 句柄为 NULL
 */
int Svc_Motor_Speed_Set_Gain(svc_motor_speed_t *speed, float kp, float ki, float kd)
{
    if (speed == NULL) {
        return -1;
    }

    Common_Pid_Set_Gain(&speed->pid, kp, ki, kd);
    return 0;
}

/*==============================================================================
 * 查询
 *============================================================================*/
/**
 * @brief 取当前实测速度
 * @param speed 速度环句柄，可为 NULL
 * @retval 实测速度（刻度值）
 */
float Svc_Motor_Speed_Get_Actual(const svc_motor_speed_t *speed)
{
    return (speed != NULL) ? Common_Pid_Get_Actual(&speed->pid) : 0.0f;
}

/**
 * @brief 取目标/实测/输出三件套
 * @param speed  速度环句柄，不可为 NULL
 * @param target 输出目标值，可为 NULL
 * @param actual 输出实测值，可为 NULL
 * @param out    输出 PID 输出值，可为 NULL
 * @retval 0: 成功
 * @retval -1: 句柄为 NULL
 */
int Svc_Motor_Speed_Get_State(const svc_motor_speed_t *speed,
                              float *target, float *actual, float *out)
{
    if (speed == NULL) {
        return -1;
    }

    if (target != NULL) {
        *target = Common_Pid_Get_Target(&speed->pid);
    }
    if (actual != NULL) {
        *actual = Common_Pid_Get_Actual(&speed->pid);
    }
    if (out != NULL) {
        *out = Common_Pid_Get_Out(&speed->pid);
    }

    return 0;
}

/*==============================================================================
 * 推进
 *============================================================================*/
/**
 * @brief 跑一轮闭环
 * @param elapsed_ms 距上次调用的毫秒数
 * @retval 无
 */
void Svc_Motor_Speed_Tick_All(uint32_t elapsed_ms)
{
    if (elapsed_ms == 0u) {
        return;
    }

    for (uint8_t i = 0u; i < s_count; i++) {
        svc_motor_speed_t *speed = &s_pool[i];
        int16_t            count = 0;
        float              actual;
        float              out;

        if (speed->dev == NULL) {
            continue;
        }

        /* ---------- 1. 读编码器增量；读不到就保持上一拍输出 ---------- */
        if (Dev_Motor_Read_Delta(speed->dev, &count) != 0) {
            continue;
        }

        /* ---------- 2. 换算实测速度 ---------- */
        actual = Svc_Motor_Speed_Count_To_Speed(speed, count, elapsed_ms);

        /* ---------- 3. 增量式 PID（内部累加并限幅） ---------- */
        (void)Common_Pid_Update_Increment(&speed->pid, actual);
        out = Common_Pid_Get_Out(&speed->pid);

        /* ---------- 4. 停稳处理：目标 0 且实测进带 → 清输出与积分 ---------- */
        if ((speed->target_speed == 0.0f)
            && (actual < SVC_MOTOR_SPEED_STOP_BAND)
            && (actual > -SVC_MOTOR_SPEED_STOP_BAND)) {
            out = 0.0f;
            Common_Pid_Reset(&speed->pid);
        }

        /* ---------- 5. 写硬件 ---------- */
        (void)Dev_Motor_Set_Duty(speed->dev, out);
    }
}

/**
 * @brief 全部停机：清零目标、复位 PID、占空比归零
 * @retval 无
 */
void Svc_Motor_Speed_Stop_All(void)
{
    for (uint8_t i = 0u; i < s_count; i++) {
        Common_Pid_Reset(&s_pool[i].pid);
        s_pool[i].target_speed = 0.0f;

        if (s_pool[i].dev != NULL) {
            Dev_Motor_Stop(s_pool[i].dev);
        }
    }
}
