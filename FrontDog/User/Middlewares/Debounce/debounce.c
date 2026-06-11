/**
 * @file debounce.c
 * @brief 防抖库状态机处理实现
 * @author 李嘉图
 * @date 2026-05-08
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
#include "main.h"
#include "debounce.h"

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
Debounce_Event_t Debounce_Update(Debounce_Key_t *key, uint8_t raw)
{
    if(raw == 0xFF) return KEY_EVENT_NONE;

    /* ==================================================
     * 1. 消抖：生成 stable 信号
     * ================================================== */
    if(raw == key->last_raw)
    {
        if(key->counter < DEBOUNCE_TICKS)
        {
            key->counter++;
            return KEY_EVENT_NONE;
        }

        key->stable = raw;   // 只有这里才更新稳定值
    }
    else
    {
        key->counter = 0;
        key->last_raw = raw;
        return KEY_EVENT_NONE;
    }

    /* ==================================================
     * 2. 状态机（只用 stable，不用 raw）
     * ================================================== */
    switch(key->state)
    {
        case KEY_NO:
            if(key->stable == key->active_level)
            {
                key->state = KEY_YES;
                key->long_counter = 0;
                return KEY_EVENT_DOWN;
            }
            break;

        case KEY_YES:
            if(key->stable == key->active_level)
            {
                if(key->long_counter < key->long_press)
                {
                    key->long_counter++;
                }
                else
                {
                    key->state = KEY_LONG;
                    return KEY_EVENT_LONG;
                }
            }
            else
            {
                key->state = KEY_NO;

                if(key->long_counter < key->long_press)
                    return KEY_EVENT_CLICK;
                else
                    return KEY_EVENT_LONG_UP;
            }
            break;

        case KEY_LONG:
            if(key->stable != key->active_level)
            {
                key->state = KEY_NO;
                return KEY_EVENT_LONG_UP;
            }
            break;
    }

    return KEY_EVENT_NONE;
}