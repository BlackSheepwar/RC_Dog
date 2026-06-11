/**
 * @file debounce.h
 * @brief 防抖库处理接口
 * @author 李嘉图
 * @date 2026-05-08
 */

#ifndef __DEBOUNCE_H__
#define __DEBOUNCE_H__

/*==============================================================================
 * 宏定义与常量
 *============================================================================*/
#define DEBOUNCE_TICKS     2        //防抖稳定计数阈值,单位:任务周期

/*==============================================================================
 * 按键状态枚举定义
 *============================================================================*/
// 按键状态枚举定义
typedef enum {
    KEY_NO = 0,                 // 未按下
    KEY_YES = 1,               // 已按下
    KEY_LONG = 2,               // 长按状态
} Debounce_State_t;

// 按键事件枚举定义
typedef enum {
    KEY_EVENT_NONE = 0,         // 无事件
    KEY_EVENT_DOWN = 1,         // 刚按下
    KEY_EVENT_CLICK = 2,        // 短按释放
    KEY_EVENT_LONG = 3,         // 长按触发
    KEY_EVENT_LONG_UP = 4       // 长按释放
} Debounce_Event_t;

/*==============================================================================
 * 按键结构体
 *============================================================================*/
 typedef struct {
    uint8_t  id;                // 按键ID
    uint8_t  stable       : 1;  // 存储稳定电平
    uint8_t  last_raw     : 1;  // 存储上一次原始电平
    uint8_t  active_level : 1;  // 有效电平：1-高电平按下，0-低电平按下
    uint8_t  state        : 2;  // 可以存 0/1/2/3
    uint8_t  reserved     : 3;  // 未使用，留给以后扩展
    uint8_t  counter;           // 防抖计数器
    uint8_t long_counter;       // 长按计数器
    uint8_t long_press;         // 长按判定阈值，单位：任务周期
} Debounce_Key_t;

// 按键事件结构体定义
typedef struct
{
    uint8_t id;
    Debounce_Event_t event;
} Debounce_Event_packet_t;

/*==============================================================================
 * 功能函数
 *============================================================================*/
/**
 * @brief 按键轮询（防抖 + 状态机）
 * @param key 按键结构体指针
 * @param raw 原始按键状态（0: 未按下, 1: 已按下）
 * @retval 按键事件枚举值
 * @note 需要周期调用（建议 5~10ms）
 */
Debounce_Event_t Debounce_Update(Debounce_Key_t *key, uint8_t raw);

#endif
