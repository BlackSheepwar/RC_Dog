/**
 * @file app_key.h
 * @brief 按键应用层（内置消抖状态机）
 * @author 李嘉图
 * @date 2026-06-18
 *
 * @note GPIO 引脚映射已下沉到 BSP 层（bsp_gpio.c 的 GPIO_HW_MAP），
 *       消抖状态机从 Middlewares/Debounce 合并至此，不再依赖外部模块。
 *       应用层注册按键时只需传逻辑参数，不再传 GPIOx/Pin。
 *
 *       内部状态机流转：
 *         IDLE → (按下) → DOWN 事件 → PRESSED
 *         PRESSED → (短按释放) → CLICK 事件 → IDLE
 *         PRESSED → (长按超时) → LONG 事件 → LONG_HELD
 *         LONG_HELD → (释放) → LONG_UP 事件 → IDLE
 */

#ifndef __APP_KEY_H__
#define __APP_KEY_H__

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include <stdint.h>

/*==============================================================================
 * 常量定义
 *============================================================================*/
#define KEY_MAX_NUM     8   /**< 最大支持的按键数（独立于 GPIO_MAX_NUM） */

/*==============================================================================
 * 事件枚举
 *============================================================================*/
/** @brief 按键事件类型 */
typedef enum {
    APP_KEY_EVENT_NONE      = 0,   /**< 无事件 */
    APP_KEY_EVENT_DOWN      = 1,   /**< 刚按下 */
    APP_KEY_EVENT_CLICK     = 2,   /**< 短按释放 */
    APP_KEY_EVENT_LONG      = 3,   /**< 长按触发 */
    APP_KEY_EVENT_LONG_UP   = 4,   /**< 长按释放 */
} App_Key_Event_t;

/** @brief 按键事件消息结构（用于消息队列） */
typedef struct {
    uint8_t         key_id;  /**< 逻辑按键 ID */
    App_Key_Event_t event;   /**< 按键事件类型 */
} App_Key_EventPacket_t;

/*==============================================================================
 * 初始化函数
 *============================================================================*/
/**
 * @brief 按键模块初始化
 * @note 内部调用 BSP_GPIO_Init()，清空按键资源池
 */
void App_Key_Init(void);

/**
 * @brief 注册一个按键到静态资源池
 * @param key_id         逻辑按键 ID（分发表匹配用，独立于硬件引脚）
 * @param gpio_id        GPIO 映射 ID（BSP_GPIO 寻址用，须与 GPIO_HW_MAP 一致）
 * @param long_press     长按判定阈值（单位:任务周期）
 * @param active_level   按键激活电平（0:低电平，1:高电平）
 * @param debounce_ticks 消抖稳定计数（单位:任务周期，推荐 2~3）
 * @retval 0: 参数错误或 gpio_id 无效
 * @retval 1: 成功
 * @retval 2: 资源池已满
 * @note key_id 和 gpio_id 分离设计，逻辑标识与硬件映射解耦。
 *       GPIO 引脚映射由 BSP 层内置的 GPIO_HW_MAP 决定，无需传入。
 */
uint8_t App_Key_Register(uint8_t key_id,
                         uint8_t gpio_id,
                         uint8_t long_press,
                         uint8_t active_level,
                         uint8_t debounce_ticks);

/*==============================================================================
 * 更新按键状态
 *============================================================================*/
/**
 * @brief 更新所有已注册按键的状态（周期调用）
 * @note 建议调用周期 10ms（与 KEY_SCAN_PERIOD_MS 一致）
 *       内部遍历按键池 → 读 GPIO → 消抖状态机 → 事件入队
 */
void App_Key_Update(void);

#endif /* __APP_KEY_H__ */
