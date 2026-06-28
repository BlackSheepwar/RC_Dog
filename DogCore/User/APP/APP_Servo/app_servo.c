/**
 * @file app_servo.c
 * @brief 舵机控制（浮点角度平滑 + 角速度驱动）
 * @author 李嘉图
 * @date 2026-06-05
 *
 * @note 调度器每 10ms 跑一次（由 task_servo.c 调用）。
 *       每次调度：
 *         1. 计算目标角度和当前角度的差值
 *         2. 按 speed_dps × 0.01s 朝目标逼近一小步（浮点）
 *         3. 把当前角度换算成 PWM 脉宽（μs）
 *         4. 调用 BSP_PWM_SetPulseUs 输出到硬件
 *
 *       硬件参数（phys_range、offset、limit、reverse）在注册时
 *       从 Servo_HwConfig_t 一次性拷入，运行时不改。
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include <stdint.h>
#include <math.h>
#include <stdlib.h> 
#include "app_servo.h"
// 功能包含
#include "bsp_pwm.h"       /* BSP_PWM_SetPulseUs */

/*==============================================================================
 * 舵机运行时实例结构体
 *============================================================================*/
typedef struct
{
    uint8_t             servo_id;        /**< 逻辑舵机 ID（分发表匹配用） */
    uint8_t             pwm_id;          /**< PWM 映射 ID（BSP_PWM 寻址用） */
    Servo_HwConfig_t    hw;              // 硬件配置（安装后固定）
    float               current_angle;   // 当前角度（浮点，连续平滑）
    int16_t             target_angle;    // 目标角度（受 phys_range 和 offset_max 约束）
    float               speed_dps;       // 角速度 (°/s)
    uint8_t             enable;          // 是否启用: 1=参与调度, 0=跳过
} APP_Servo_t;

/*==============================================================================
 * 内部辅助函数（static inline，零调用开销）
 *============================================================================*/
/**
 * @brief 计算物理角度可用范围（扣除offset_max余量）
 * @param hw 舵机硬件配置
 * @return 正负对称的物理限位值，如 125（phys_range=300, offset_max=25）
 */
static inline int16_t Servo_GetAbsLimit(const Servo_HwConfig_t *hw)
{
    return (int16_t)(hw->phys_range / 2) - (int16_t)hw->offset_max;
}

/**
 * @brief 角度限幅：先物理范围（扣除offset余量），再软件限位
 * @param hw    舵机硬件配置
 * @param angle 待限幅的角度
 * @return 限幅后的角度
 */
static inline int16_t Servo_ClampAngle(const Servo_HwConfig_t *hw, int16_t angle)
{
    int16_t abs_limit = Servo_GetAbsLimit(hw);
    if (angle < -abs_limit) angle = -abs_limit;
    else if (angle > abs_limit) angle = abs_limit;
    if (angle < hw->limit_min) angle = hw->limit_min;
    if (angle > hw->limit_max) angle = hw->limit_max;
    return angle;
}

/**
 * @brief 角度转PWM脉宽（μs）
 * @param hw            舵机硬件配置（含 pulse_min/pulse_max）
 * @param current_angle 当前角度（浮点，含亚度数精度）
 * @return 脉宽值（μs），范围 [pulse_min, pulse_max]
 * @note  换算公式: out = current + offset + half_range
 *         reverse=1 时镜像：out' = phys_range - out
 *         脉宽 = pulse_min + out × (pulse_max - pulse_min) / phys_range
 */
static inline uint16_t Servo_AngleToPulse(const Servo_HwConfig_t *hw, float current_angle)
{
    float half_range    = (float)hw->phys_range * 0.5f;
    float pulse_per_deg = (float)(hw->pulse_max - hw->pulse_min) / (float)hw->phys_range;

    float out_angle = current_angle + (float)hw->offset + half_range;
    if (hw->reverse)
        out_angle = (float)hw->phys_range - out_angle;
    if (out_angle < 0.0f)
        out_angle = 0.0f;
    else if (out_angle > (float)hw->phys_range)
        out_angle = (float)hw->phys_range;

    return (uint16_t)((float)hw->pulse_min + out_angle * pulse_per_deg + 0.5f);
}

/*==============================================================================
 * 舵机静态池
 *
 * 所有舵机实例存在这个数组里，最多 TIM_MAX_SIZE 个。
 * 注册时（APP_Servo_Add）按顺序填入空闲槽位。
 * 查找时（APP_Servo_GetById）遍历匹配 id。
 *============================================================================*/
static APP_Servo_t app_servo_pool[TIM_MAX_SIZE];
static uint8_t app_servo_count = 0;  /* 当前已注册的舵机数量 */

/**
 * @brief 根据逻辑舵机编号查找实例
 * @param servo_id 逻辑舵机编号
 * @retval 非NULL：找到，返回指针
 * @retval NULL：未找到
 */
static APP_Servo_t *APP_Servo_GetById(uint8_t servo_id)
{
    for (uint8_t i = 0; i < app_servo_count; i++)
    {
        if (app_servo_pool[i].servo_id == servo_id)
            return &app_servo_pool[i];
    }
    return NULL;
}

/*==============================================================================
 * 初始化函数
 *============================================================================*/
/**
 * @brief 初始化APP舵机控制系统
 * @note 清空舵机池，同时调用 BSP_PWM_Init 重置底层
 */
void APP_Servo_Init(void)
{
    BSP_PWM_Init();
    app_servo_count = 0;
}

/**
 * @brief 注册一个舵机实例到控制池
 * @param servo_id  逻辑舵机编号（分发表匹配用）
 * @param pwm_id    PWM 映射 ID（BSP_PWM 寻址用，须与 PWM_HW_MAP 一致）
 * @param hw        舵机硬件配置（安装后固定参数，注册时拷贝入结构体）
 * @param speed_dps 角速度 (°/s)，例如 300.0f = 每秒转 300°
 * @retval 1: 注册成功
 * @retval 0: 注册失败（池满、ID 重复、参数无效）
 * @note servo_id 和 pwm_id 分离设计，逻辑标识与硬件映射解耦。
 *       注册时校验 offset ≤ offset_max、limit 截断到物理可用范围，
 *       校验通过后启动 PWM 并转到 init_angle 位置。
 *       hw 中的 offset 若 reverse=1 则自动取反。
 */
uint8_t APP_Servo_Add(uint8_t servo_id, uint8_t pwm_id, const Servo_HwConfig_t *hw, float speed_dps)
{
    /* ---------- 容量检查 ---------- */
    if (app_servo_count >= TIM_MAX_SIZE) return 0;
    /* ---------- 重复检查 ---------- */
    if (APP_Servo_GetById(servo_id) != NULL) return 0;
    /* ---------- 参数校验 ---------- */
    if (hw == NULL) return 0;
    if (hw->phys_range == 0) return 0;
    if (abs(hw->offset) > hw->offset_max) return 0;
    /* ---------- 整体拷贝硬件配置 ---------- */
    APP_Servo_t *se = &app_servo_pool[app_servo_count];
    se->servo_id = servo_id;
    se->pwm_id   = pwm_id;
    se->hw = *hw;
    /* 如果装了反转，offset 也要取反，否则方向会错 */
    if (se->hw.reverse)
        se->hw.offset = -se->hw.offset;
    /* ── 限位截断到物理可用范围 ──
     * 输出脉宽公式: out = current + offset + phys_range/2
     * offset 最大 ±offset_max，为使 out 不超出 [0, phys_range]:
     *   current 最大 = phys_range/2 - offset_max = (phys_range - 2*offset_max)/2
     *   current 最小 = -(phys_range/2 - offset_max)
     * 用户配置的 limit_min/limit_max 必须在此范围内，否则截断 */
    int16_t abs_limit = Servo_GetAbsLimit(hw);
    if (se->hw.limit_min < -abs_limit) se->hw.limit_min = -abs_limit;
    if (se->hw.limit_max > abs_limit)  se->hw.limit_max = abs_limit;
    if (se->hw.limit_min > se->hw.limit_max)
        se->hw.limit_min = se->hw.limit_max;   /* 保底：交叉时以下限为准 */
    /* ---------- 初始化运行时状态 ---------- */
    se->current_angle = (float)hw->init_angle;
    se->target_angle  = hw->init_angle;
    se->speed_dps     = speed_dps;
    se->enable        = 1;
    /* ── 先设脉宽，再启动 PWM，避免启动瞬间的异常脉冲 ── */
    BSP_PWM_SetPulseUs(se->pwm_id, Servo_AngleToPulse(&se->hw, se->current_angle));
    BSP_PWM_Start(se->pwm_id);
    BSP_PWM_Reset(se->pwm_id);

    app_servo_count++;
    return 1;
}

/*==============================================================================
 * 舵机控制函数
 *
 * 这些函数被 CAN 命令、UART 命令、步态调度器调用。
 * 它们只修改目标值，不直接控制硬件——
 * 真正的硬件输出在 APP_Servo_Scheduler() 里每 10ms 执行一次。
 *============================================================================*/
/**
 * @brief 设置舵机目标角度
 * @param id    舵机编号
 * @param angle 目标角度
 * @note 调度器会以当前 speed_dps 平滑逼近此角度。
 *       先限幅到物理可用范围 +/-(phys_range/2 - offset_max)，
 *       再限幅到软件机械限制 hw.limit_min / hw.limit_max。
 */
void APP_Servo_SetTarget(uint8_t id, int16_t angle)
{
    APP_Servo_t *se = APP_Servo_GetById(id);
    if (se == NULL) return;

    se->target_angle = Servo_ClampAngle(&se->hw, angle);
}

/**
 * @brief 设置舵机目标角度（增量方式）
 * @param id    舵机编号
 * @param angle 角度增量（相对当前目标值）
 * @note 用于 CAN 或 UART 的增量式控制。
 *       例如当前目标 0°，调 +10 就变成 10°。
 *       先限幅到物理可用范围 +/-(phys_range/2 - offset_max)，
 *       再限幅到软件机械限制 hw.limit_min / hw.limit_max。
 */
void APP_Servo_SetincreaseTarget(uint8_t id, int16_t angle)
{
    APP_Servo_t *se = APP_Servo_GetById(id);
    if (se == NULL) return;

    int16_t target = se->target_angle + angle;
    se->target_angle = Servo_ClampAngle(&se->hw, target);
}

/**
 * @brief 获取舵机当前角度
 * @param id 舵机编号
 * @return 当前角度，四舍五入到整数
 * @note 内部 current_angle 是浮点数（精度高），
 *       此函数取整后返回，供外部查询或协议回传
 */
int16_t APP_Servo_GetCurrent(uint8_t id)
{
    APP_Servo_t *se = APP_Servo_GetById(id);
    if (se == NULL) return 0xFF;
    /* +0.5f 实现四舍五入 */
    return (int16_t)(se->current_angle + 0.5f);
}

/**
 * @brief 设置舵机运动角速度
 * @param id        舵机编号
 * @param speed_dps 角速度 (°/s)
 * @note 不设上限，测试后自行决定合理值。
 *       常见值：低速 30°/s，中速 300°/s，高速 600°/s+
 */
void APP_Servo_SetSpeed(uint8_t id, float speed_dps)
{
    APP_Servo_t *se = APP_Servo_GetById(id);
    if (se == NULL) return;
    se->speed_dps = speed_dps;
}

/*==============================================================================
 * 舵机平滑调度器
 *
 * 每 SERVO_TICK_MS毫秒 调用一次，每次只走一小步（浮点）。
 * 脉宽输出 = pulse_min + out × (pulse_max - pulse_min) / phys_range
 * 反向模式时将 out_angle 镜像：out_angle' = phys_range - out_angle
 *============================================================================*/
void APP_Servo_Scheduler(void)
{
    for (uint8_t i = 0; i < app_servo_count; i++)
    {
        APP_Servo_t *se = &app_servo_pool[i];
        if (se->enable == 0) continue;

        /* ── 算差值 + 到位检查 ── */
        float diff = (float)se->target_angle - se->current_angle;
        if (fabsf(diff) < 0.001f)
        {
            se->current_angle = (float)se->target_angle;
            /* 即使已到位也输出 PWM，确保 Limb 层更新的角度即时反映到硬件 */
            BSP_PWM_SetPulseUs(se->pwm_id, Servo_AngleToPulse(&se->hw, se->current_angle));
            continue;
        }

        /* ── 算步长 + 逼近 ── */
        float move = se->speed_dps * SERVO_TICK_S;
        if (move > fabsf(diff)) move = fabsf(diff);
        se->current_angle += (diff > 0.0f) ? move : -move;

        /* ── 角度 → 脉宽 ── */
        BSP_PWM_SetPulseUs(se->pwm_id, Servo_AngleToPulse(&se->hw, se->current_angle));
    }
}
