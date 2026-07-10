/**
 * @file app_led.h
 * @brief LED 应用层（BSP_GPIO 上层封装）
 * @author 李嘉图
 * @date 2026-07-10
 *
 * @note LED 采用静态注册表模式（同 app_key_cfg.h 的 KEY_REG_TABLE），
 *       通过 led_id 查找对应的 gpio_id，再调用 BSP_GPIO 接口操作硬件。
 *       硬件引脚映射由 BSP 层的 GPIO_HW_MAP 决定，本模块不感知具体引脚。
 *
 *       新增 LED 只需在 app_led.c 的 LED_REG_TABLE 表中添加一行，
 *       并确保对应的 gpio_id 已在 bsp_gpio.c 的 GPIO_HW_MAP 中注册。
 */

#ifndef __APP_LED_H__
#define __APP_LED_H__

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include <stdint.h>

/*==============================================================================
 * 常量定义
 *============================================================================*/
#define LED_MAX_NUM     10    /**< 最大支持的 LED 数量 */

/** @brief LED 亮灭控制宏 */
#define LED_ON  1    /**< 点亮 */
#define LED_OFF 0    /**< 熄灭 */

/*==============================================================================
 * 类型定义
 *============================================================================*/
/** @brief LED 注册参数结构体 */
typedef struct {
    uint8_t  led_id;           /**< 逻辑 LED ID（应用层寻址用） */
    uint8_t  gpio_id;          /**< GPIO 映射 ID（BSP_GPIO 寻址用） */
    uint8_t  active_level;     /**< 点亮电平（GPIO_LEVEL_HIGH / GPIO_LEVEL_LOW） */
} led_register_t;

/*==============================================================================
 * 初始化函数
 *============================================================================*/
/**
 * @brief LED 模块初始化
 * @note 根据 LED_REG_TABLE 遍历注册所有 LED，将所有灯置为熄灭状态
 */
void App_LED_Init(void);

/*==============================================================================
 * 控制函数
 *============================================================================*/
/**
 * @brief LED 亮灭控制
 * @param led_id 逻辑 LED ID
 * @param cmd    控制命令：LED_ON 点亮 / LED_OFF 熄灭
 *               （内部根据注册表中的 active_level 自动映射硬件极性）
 * @retval 1: 成功
 * @retval 0: 失败（led_id 无效）
 */
uint8_t App_LED_Control(uint8_t led_id, uint8_t cmd);

/**
 * @brief LED 批量亮灭控制
 * @param led_ids  LED ID 数组指针
 * @param count   数组长度
 * @param cmd     控制命令：LED_ON 点亮 / LED_OFF 熄灭
 * @retval 1: 全部成功
 * @retval 0: 存在失败（某个 led_id 无效）
 */
uint8_t App_LED_ControlBatch(const uint8_t *led_ids, uint8_t count, uint8_t cmd);

/**
 * @brief LED 批量翻转
 * @param led_ids  LED ID 数组指针
 * @param count   数组长度
 * @retval 1: 全部成功
 * @retval 0: 存在失败（某个 led_id 无效）
 */
uint8_t App_LED_ToggleBatch(const uint8_t *led_ids, uint8_t count);

/**
 * @brief 翻转指定 LED 状态
 * @param led_id 逻辑 LED ID
 * @retval 1: 成功
 * @retval 0: 失败（led_id 无效）
 */
uint8_t App_LED_Toggle(uint8_t led_id);

#endif /* __APP_LED_H__ */
