/**
 * @file app_servo.h
 * @brief 舵机控制（浮点角度平滑 + 角速度驱动）
 * @author 李嘉图
 * @date 2026-06-05
 *
 * @note 调度器周期固定为10ms，current_angle 为浮点数实现连续平滑。
 *       每 tick 移动量 = speed_dps × 0.01s，直接浮点累加无整数步进。
 *
 *       Servo_HwConfig_t 包含舵机安装后不会变的物理参数，
 *       在 app_servo_cfg.h 中定义，注册时传给 APP_Servo_Add。
 *       运行时只调 SetTarget/SetSpeed，不改硬件参数。
 */

#ifndef __APP_SERVO_H__
#define __APP_SERVO_H__

/*==============================================================================
 * 头文件包含
 *============================================================================*/
 // 固定包含
#include <stdint.h>

/*==============================================================================
 * 宏定义与常量
 *============================================================================*/
/* 调度周期：秒版（调度器用） + 毫秒版（osDelay 用） */
#define SERVO_TICK_S        0.01f
#define SERVO_TICK_MS       10

/*==============================================================================
 * 舵机硬件配置结构体
 *
 * 这些参数由 app_servo_cfg.h 定义，安装后固定，运行时不动。
 * 每个舵机一份，注册时整体传入 APP_Servo_Add。
 *============================================================================*/
typedef struct {
    uint16_t  phys_range;        // 舵机物理总范围（±phys_range/2），如 300=±150°
    int16_t   offset;            // 中位偏移校准值
    uint8_t   offset_max;        // 偏移量上限（防止误填，注册时校验）
    int16_t   limit_min;         // 软件限位下限（受 phys_range 和 offset_max 约束）
    int16_t   limit_max;         // 软件限位上限（受 phys_range 和 offset_max 约束）
    int16_t   init_angle;        // 上电初始角度
    uint8_t   reverse;           // 方向反转: 0=正转, 1=反转（安装时确定）
    uint16_t  pulse_min;         // 最小脉宽（μs），典型值 500
    uint16_t  pulse_max;         // 最大脉宽（μs），典型值 2500
} Servo_HwConfig_t;

/*==============================================================================
 * 初始化函数
 *============================================================================*/
/**
 * @brief 初始化APP舵机控制系统
 * @note 清空舵机池，同时调用 BSP_PWM_Init 重置底层
 */
void APP_Servo_Init(void);

/**
 * @brief 注册一个舵机实例到控制池
 * @param servo_id  逻辑舵机编号（分发表匹配用）
 * @param pwm_id    PWM 映射 ID（BSP_PWM 寻址用，须与 PWM_HW_MAP 一致）
 * @param hw        舵机硬件配置（安装后固定参数，注册时拷贝入结构体）
 * @param speed_dps 角速度 (°/s)，例如 300.0f = 每秒转 300°
 * @retval 1: 注册成功
 * @retval 0: 注册失败（池满、ID 重复、参数无效）
 * @note servo_id 和 pwm_id 分离设计，逻辑标识与硬件映射解耦。
 *       例如：腿 #1 的舵机可以映射到任意 PWM 通道。
 *       注册时校验 offset ≤ offset_max、limit 截断到物理可用范围，
 *       校验通过后启动 PWM 并转到 init_angle 位置。
 */
uint8_t APP_Servo_Add(uint8_t servo_id, uint8_t pwm_id, const Servo_HwConfig_t *hw, float speed_dps);

/*==============================================================================
 * 舵机控制函数
 *
 * 这些函数被 CAN 命令、UART 命令、步态调度器调用。
 * 它们只修改目标值，不直接控制硬件——
 * 真正的硬件输出在 APP_Servo_Scheduler() 里每 SERVO_TICK_MS毫秒 执行一次。
 *============================================================================*/
/**
 * @brief 设置舵机目标角度
 * @param id    舵机编号
 * @param angle 目标角度（浮点，亚度精度）
 * @note 先限幅到物理可用范围 +/-(phys_range/2 - offset_max)，
 *       再限幅到软件机械限制 hw.limit_min / hw.limit_max
 */
void APP_Servo_SetTarget(uint8_t id, float angle);

/**
 * @brief 设置舵机目标角度（增量方式）
 * @param id    舵机编号
 * @param angle 角度增量（浮点，相对当前目标值）
 * @note 用于 CAN 或 UART 的增量式控制。
 *       先限幅到物理可用范围 +/-(phys_range/2 - offset_max)，
 *       再限幅到软件机械限制 hw.limit_min / hw.limit_max
 */
void APP_Servo_SetincreaseTarget(uint8_t id, float angle);

/**
 * @brief 获取舵机当前角度
 * @param id 舵机编号
 * @return 当前角度（浮点，亚度精度）
 * @note 内部 current_angle 是浮点数，直接返回无需取整
 */
float APP_Servo_GetCurrent(uint8_t id);

/**
 * @brief 设置舵机运动角速度
 * @param id        舵机编号
 * @param speed_dps 角速度 (°/s)
 * @note 不设上限，测试后自行决定合理值
 *       常见值：低速 30°/s，中速 300°/s，高速 600°/s+
 */
void APP_Servo_SetSpeed(uint8_t id, float speed_dps);

/*==============================================================================
 * 舵机平滑调度器
 *
 * 每 SERVO_TICK_MS毫秒 调用一次，每次只走一小步（浮点）。
 * 脉宽输出 = 500 + (current + offset + phys_range/2) × (2000 / phys_range)
 *          = 500μs（最低）~ 2500μs（最高）
 *
 * 反向模式时将 out_angle 镜像：out_angle' = phys_range - out_angle
 *============================================================================*/
/**
 * @brief 舵机平滑调度器（SERVO_TICK_MS毫秒）
 * @note 每 tick 计算 speed_dps × SERVO_TICK_S 的浮点步进，
 *       current_angle 直接浮点累加实现亚度数连续平滑，
 *       最终将角度换算为脉宽（μs）调用 BSP_PWM_SetPulseUs 输出
 */
void APP_Servo_Scheduler(void);

#endif /* __APP_SERVO_H__ */
