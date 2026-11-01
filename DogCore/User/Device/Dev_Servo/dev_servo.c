/**
 * @file dev_servo.c
 * @brief 舵机设备层实现：安装参数表 + 角度/脉宽换算
 * @author Ricardo
 * @date 2026-11-01
 *
 * @note 8 台舵机的安装参数集中写在本文件的实例表里——这是"这台机器装成什么样"
 *       的唯一事实来源。换舵机、换中位、换行程只改这张表，上面三层一行不动。
 *
 *       换算链（与标定步骤一一对应）：
 *         角度 → 加中位偏移 → 加半行程 → 反装镜像 → 换算成脉宽
 *       反装时偏移要反向参与，所以偏移在表里写原始符号，
 *       取负的动作放在换算里做，句柄保持 const。
 *
 *       运行时状态只有"最近一次写入的角度"，按槽位放进 static 数组；
 *       句柄只放配置，编译期进 .rodata，不占 RAM。
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include <stddef.h>
#include "common.h"             /* ARRAY_SIZE */
#include "dev_servo.h"
// 功能包含
#include "bsp_pwm.h"            /* BSP_PWM_Set_Pulse_Us / Start / Stop */

/*==============================================================================
 * 类型定义
 *============================================================================*/
/**
 * @brief 舵机句柄（一台一份）
 * @note 只放配置，全部字段在编译期确定，因此可以 const 进 .rodata。
 */
struct dev_servo {
    const bsp_pwm_t *pwm;        /* PWM 通道句柄，引脚号在 BSP 里 */

    float    phys_range;         /* 物理总行程（度），如 300 表示 ±150 */
    float    offset;             /* 中位偏移（度），标定值，保留原始符号 */
    float    offset_max;         /* 偏移允许的上限（度） */
    float    limit_min;          /* 软件下限（度） */
    float    limit_max;          /* 软件上限（度） */
    float    init_angle;         /* 上电初始角（度） */
    uint8_t  reverse;            /* 1: 安装方向相反 */
    uint16_t pulse_min;          /* 最小脉宽（μs） */
    uint16_t pulse_max;          /* 最大脉宽（μs） */
};

/*==============================================================================
 * 句柄实例：安装参数只出现在这里
 *============================================================================*/
// 左前腿
const dev_servo_t dev_servo_1 = {
    .pwm = &bsp_pwm_servo_1, .phys_range = 300.0f, .offset = -3.0f, .offset_max = 20.0f,
    .limit_min = -130.0f, .limit_max = 130.0f, .init_angle = 73.0f, .reverse = 1u,
    .pulse_min = 500u, .pulse_max = 2500u,
};
const dev_servo_t dev_servo_2 = {
    .pwm = &bsp_pwm_servo_2, .phys_range = 300.0f, .offset = 5.0f, .offset_max = 20.0f,
    .limit_min = -130.0f, .limit_max = 130.0f, .init_angle = -57.0f, .reverse = 1u,
    .pulse_min = 500u, .pulse_max = 2500u,
};
// 右前腿
const dev_servo_t dev_servo_3 = {
    .pwm = &bsp_pwm_servo_3, .phys_range = 300.0f, .offset = 5.0f, .offset_max = 20.0f,
    .limit_min = -130.0f, .limit_max = 130.0f, .init_angle = 73.0f, .reverse = 0u,
    .pulse_min = 500u, .pulse_max = 2500u,
};
const dev_servo_t dev_servo_4 = {
    .pwm = &bsp_pwm_servo_4, .phys_range = 300.0f, .offset = 2.0f, .offset_max = 20.0f,
    .limit_min = -130.0f, .limit_max = 130.0f, .init_angle = -57.0f, .reverse = 0u,
    .pulse_min = 500u, .pulse_max = 2500u,
};
// 左后腿
const dev_servo_t dev_servo_5 = {
    .pwm = &bsp_pwm_servo_5, .phys_range = 300.0f, .offset = -15.0f, .offset_max = 20.0f,
    .limit_min = -130.0f, .limit_max = 130.0f, .init_angle = 73.0f, .reverse = 0u,
    .pulse_min = 500u, .pulse_max = 2500u,
};
const dev_servo_t dev_servo_6 = {
    .pwm = &bsp_pwm_servo_6, .phys_range = 300.0f, .offset = 8.0f, .offset_max = 20.0f,
    .limit_min = -130.0f, .limit_max = 130.0f, .init_angle = -57.0f, .reverse = 0u,
    .pulse_min = 500u, .pulse_max = 2500u,
};
// 右后腿
const dev_servo_t dev_servo_7 = {
    .pwm = &bsp_pwm_servo_7, .phys_range = 300.0f, .offset = 0.0f, .offset_max = 20.0f,
    .limit_min = -130.0f, .limit_max = 130.0f, .init_angle = 73.0f, .reverse = 1u,
    .pulse_min = 500u, .pulse_max = 2500u,
};
const dev_servo_t dev_servo_8 = {
    .pwm = &bsp_pwm_servo_8, .phys_range = 300.0f, .offset = -7.0f, .offset_max = 20.0f,
    .limit_min = -130.0f, .limit_max = 130.0f, .init_angle = -57.0f, .reverse = 1u,
    .pulse_min = 500u, .pulse_max = 2500u,
};

/*==============================================================================
 * 内部变量
 *============================================================================*/
/** @brief 舵机句柄表（顺序即槽位） */
static const dev_servo_t *const s_servo_list[] = {
    &dev_servo_1, &dev_servo_2, &dev_servo_3, &dev_servo_4,
    &dev_servo_5, &dev_servo_6, &dev_servo_7, &dev_servo_8,
};

/** @brief 最近一次写入的角度（运行时状态，按槽位索引） */
static float s_angle[ARRAY_SIZE(s_servo_list)];

/** @brief 是否已初始化过（幂等标记：统一初始化在调度前执行一次，守卫防将来重复调用） */
static uint8_t s_inited = 0u;

/*==============================================================================
 * 内部函数
 *============================================================================*/
/**
 * @brief 按句柄查槽位
 * @param servo 舵机句柄，可为 NULL
 * @retval 0 ~ Count-1: 槽位
 * @retval 0xFF: 句柄为空或不在表里
 */
static uint8_t Dev_Servo_Find_Slot(const dev_servo_t *servo)
{
    if (servo == NULL) {
        return 0xFFu;
    }

    for (uint8_t i = 0u; i < ARRAY_SIZE(s_servo_list); i++) {
        if (s_servo_list[i] == servo) {
            return i;
        }
    }

    return 0xFFu;
}

/**
 * @brief 角度 → 脉宽
 * @param servo 舵机句柄，不可为 NULL
 * @param deg   角度（度），已夹紧
 * @retval 脉宽（μs），范围 [pulse_min, pulse_max]
 * @note out = deg + (reverse ? −offset : offset) + range/2
 *       reverse 时镜像：out' = range − out
 *       脉宽 = pulse_min + out × (pulse_max − pulse_min) / range
 */
static uint16_t Dev_Servo_Angle_To_Pulse(const dev_servo_t *servo, float deg)
{
    float half   = servo->phys_range * 0.5f;
    float offset = (servo->reverse != 0u) ? -servo->offset : servo->offset;
    float span   = (float)(servo->pulse_max - servo->pulse_min) / servo->phys_range;
    float pos;

    pos = deg + offset + half;

    if (servo->reverse != 0u) {
        pos = servo->phys_range - pos;
    }

    if (pos < 0.0f) {
        pos = 0.0f;
    } else if (pos > servo->phys_range) {
        pos = servo->phys_range;
    }

    return (uint16_t)((float)servo->pulse_min + (pos * span) + 0.5f);
}

/*==============================================================================
 * 初始化与枚举
 *============================================================================*/
/**
 * @brief 逐台输出初始脉宽并启动 PWM 通道
 * @retval 0: 全部成功（含"已经初始化过"的情况）
 * @retval -1: 句柄表里有空指针或参数非法
 * @note 幂等：重复调用只执行第一次。本函数在 freertos.c 的调度前统一调用一次；
 *       守卫给将来的重复调用兜底——否则会把舵机拽回初始角、
 *       把已下发的目标角冲掉。
 */
int Dev_Servo_Init(void)
{
    if (s_inited != 0u) {
        return 0;
    }

    for (uint8_t i = 0u; i < ARRAY_SIZE(s_servo_list); i++) {
        const dev_servo_t *servo = s_servo_list[i];

        if ((servo == NULL) || (servo->pwm == NULL)) {
            return -1;
        }
        if (servo->phys_range <= 0.0f) {
            return -1;
        }

        /* 先写比较值再启动通道：避免上电第一个周期宽度不定 */
        s_angle[i] = servo->init_angle;
        (void)Dev_Servo_Set_Angle(servo, servo->init_angle);
        BSP_PWM_Start(servo->pwm);
    }

    s_inited = 1u;

    return 0;
}

/**
 * @brief 取舵机台数
 * @retval 已实例化的舵机数量
 */
uint8_t Dev_Servo_Count(void)
{
    return (uint8_t)ARRAY_SIZE(s_servo_list);
}

/**
 * @brief 按槽位取舵机句柄
 * @param index 槽位，0 ~ Dev_Servo_Count()-1
 * @retval 非 NULL: 舵机句柄
 * @retval NULL: 槽位越界
 */
const dev_servo_t *Dev_Servo_Get(uint8_t index)
{
    if (index >= ARRAY_SIZE(s_servo_list)) {
        return NULL;
    }

    return s_servo_list[index];
}

/*==============================================================================
 * 控制
 *============================================================================*/
/**
 * @brief 把角度夹到这台舵机机械允许的范围内
 * @param servo 舵机句柄，可为 NULL
 * @param deg   目标角（度）
 * @retval 夹紧后的角度（度）
 */
float Dev_Servo_Clamp_Angle(const dev_servo_t *servo, float deg)
{
    float abs_limit;
    float limit_min;
    float limit_max;

    if (servo == NULL) {
        return deg;
    }

    /* ---------- 1. 物理行程（扣掉偏移余量，保证换算不越界） ---------- */
    abs_limit = (servo->phys_range * 0.5f) - servo->offset_max;
    if (deg < -abs_limit) {
        deg = -abs_limit;
    } else if (deg > abs_limit) {
        deg = abs_limit;
    }

    /* ---------- 2. 软件限位（安装表里配的机构极限） ---------- */
    limit_min = (servo->limit_min > -abs_limit) ? servo->limit_min : -abs_limit;
    limit_max = (servo->limit_max < abs_limit) ? servo->limit_max : abs_limit;
    if (limit_min > limit_max) {
        limit_min = limit_max;
    }

    if (deg < limit_min) {
        deg = limit_min;
    } else if (deg > limit_max) {
        deg = limit_max;
    }

    return deg;
}

/**
 * @brief 设置角度并立即输出脉宽
 * @param servo 舵机句柄，可为 NULL
 * @param deg   目标角（度）
 * @retval 0: 已输出
 * @retval -1: 句柄为 NULL
 */
int Dev_Servo_Set_Angle(const dev_servo_t *servo, float deg)
{
    uint8_t  slot;
    uint16_t pulse;

    slot = Dev_Servo_Find_Slot(servo);
    if (slot == 0xFFu) {
        return -1;
    }

    deg   = Dev_Servo_Clamp_Angle(servo, deg);
    pulse = Dev_Servo_Angle_To_Pulse(servo, deg);

    BSP_PWM_Set_Pulse_Us(servo->pwm, pulse);
    s_angle[slot] = deg;

    return 0;
}

/**
 * @brief 直接给脉宽（标定中位/量行程时用）
 * @param servo 舵机句柄，可为 NULL
 * @param us    高电平脉宽（μs）
 * @retval 无
 */
void Dev_Servo_Set_Pulse_Us(const dev_servo_t *servo, uint16_t us)
{
    if ((servo == NULL) || (servo->pwm == NULL)) {
        return;
    }

    BSP_PWM_Set_Pulse_Us(servo->pwm, us);
}

/**
 * @brief 取最近一次写给这台舵机的角度
 * @param servo 舵机句柄，可为 NULL
 * @retval 最近一次的角度（度）；句柄为 NULL 时返回 0
 */
float Dev_Servo_Get_Angle(const dev_servo_t *servo)
{
    uint8_t slot = Dev_Servo_Find_Slot(servo);

    if (slot == 0xFFu) {
        return 0.0f;
    }

    return s_angle[slot];
}

/**
 * @brief 停止这台舵机的 PWM 输出
 * @param servo 舵机句柄，可为 NULL
 * @retval 无
 */
void Dev_Servo_Stop(const dev_servo_t *servo)
{
    if ((servo == NULL) || (servo->pwm == NULL)) {
        return;
    }

    BSP_PWM_Stop(servo->pwm);
}
