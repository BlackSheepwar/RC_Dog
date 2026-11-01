/**
 * @file dev_servo.h
 * @brief 舵机设备层：一台舵机一个句柄，管安装参数与"角度→脉宽"
 * @author Ricardo
 * @date 2026-11-01
 *
 * @note 一台舵机的全部物理属性都在这里：物理行程、中位偏移、是否反装、
 *       软件限位、初始角、脉宽范围。上面几层因此不需要再各存一份配置。
 *
 *       本层只做"给一个角度，立刻输出对应脉宽"，**不含任何时间轴**：
 *       平滑、限速、多关节同步到位属于 Service/Svc_Servo_Motion。
 *
 *       句柄只放配置（const，进 .rodata）；最近一次写入的角度是运行时状态，
 *       放在本模块的 static 数组里，按句柄的槽位索引取。
 */

#ifndef __Dev_Servo_H__
#define __Dev_Servo_H__

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include <stdint.h>

/*==============================================================================
 * 类型定义
 *============================================================================*/
/**
 * @brief 舵机句柄（不透明类型，布局在 dev_servo.c）
 */
typedef struct dev_servo dev_servo_t;

/*==============================================================================
 * 外部声明：一台舵机一份句柄
 *============================================================================*/
/** @brief 1 号舵机（左前髋） */
extern const dev_servo_t dev_servo_1;
/** @brief 2 号舵机（左前膝） */
extern const dev_servo_t dev_servo_2;
/** @brief 3 号舵机（右前髋） */
extern const dev_servo_t dev_servo_3;
/** @brief 4 号舵机（右前膝） */
extern const dev_servo_t dev_servo_4;
/** @brief 5 号舵机（左后髋） */
extern const dev_servo_t dev_servo_5;
/** @brief 6 号舵机（左后膝） */
extern const dev_servo_t dev_servo_6;
/** @brief 7 号舵机（右后髋） */
extern const dev_servo_t dev_servo_7;
/** @brief 8 号舵机（右后膝） */
extern const dev_servo_t dev_servo_8;

/*==============================================================================
 * 初始化与枚举
 *============================================================================*/
/**
 * @brief 逐台输出初始脉宽并启动 PWM 通道
 * @retval 0: 全部成功
 * @retval -1: 句柄表里有空指针或参数非法（phys_range 为 0）
 * @note 必须在 BSP_PWM_Init()（定时器频率）之后调用：
 *       改 PSC/ARR 会让已写入的比较值失效。
 *       先写比较值再启动通道，避免上电第一个周期出现不定宽度脉冲。
 */
int Dev_Servo_Init(void);

/**
 * @brief 取舵机台数
 * @retval 已实例化的舵机数量（当前 8）
 */
uint8_t Dev_Servo_Count(void);

/**
 * @brief 按槽位取舵机句柄
 * @param index 槽位，0 ~ Dev_Servo_Count()-1
 * @retval 非 NULL: 舵机句柄
 * @retval NULL: 槽位越界
 */
const dev_servo_t *Dev_Servo_Get(uint8_t index);

/*==============================================================================
 * 控制
 *============================================================================*/
/**
 * @brief 把角度夹到这台舵机机械允许的范围内
 * @param servo 舵机句柄，可为 NULL
 * @param deg   目标角（度），用户坐标系
 * @retval 夹紧后的角度（度）；句柄为 NULL 时原值返回
 * @note 先夹到物理行程（已扣掉偏移余量），再夹到安装时配的软件限位。
 *       上层要判断"到没到位"，得先知道自己给的目标有没有被夹，
 *       所以夹紧规则集中在这一个函数里，不在上层重写一遍。
 */
float Dev_Servo_Clamp_Angle(const dev_servo_t *servo, float deg);

/**
 * @brief 设置角度并立即输出脉宽
 * @param servo 舵机句柄，可为 NULL
 * @param deg   目标角（度），内部会先夹紧
 * @retval 0: 已输出
 * @retval -1: 句柄为 NULL
 * @note 立即生效、无平滑：要"慢慢转过去"请用 Svc_Servo_Motion。
 */
int Dev_Servo_Set_Angle(const dev_servo_t *servo, float deg);

/**
 * @brief 直接给脉宽（标定中位/量行程时用）
 * @param servo 舵机句柄，可为 NULL
 * @param us    高电平脉宽（μs），内部按 PWM 通道的计数频率换算
 * @retval 无
 * @note 绕开了角度换算与限位，只在标定时用，正常运行请走 Set_Angle。
 */
void Dev_Servo_Set_Pulse_Us(const dev_servo_t *servo, uint16_t us);

/**
 * @brief 取最近一次写给这台舵机的角度
 * @param servo 舵机句柄，可为 NULL
 * @retval 最近一次的角度（度，已夹紧）；句柄为 NULL 时返回 0
 * @note 读的是"我们让它去的位置"，不是编码器反馈的回读值——
 *       模拟舵机没有位置反馈，这个值就是系统对它的最佳认知。
 */
float Dev_Servo_Get_Angle(const dev_servo_t *servo);

/**
 * @brief 停止这台舵机的 PWM 输出
 * @param servo 舵机句柄，可为 NULL
 * @retval 无
 * @note 停止后舵机失去保持力矩（模拟舵机会松掉），只在断电流程里用。
 */
void Dev_Servo_Stop(const dev_servo_t *servo);

#endif /* __Dev_Servo_H__ */
