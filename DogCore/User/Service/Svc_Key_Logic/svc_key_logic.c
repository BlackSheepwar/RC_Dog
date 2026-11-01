/**
 * @file svc_key_logic.c
 * @brief 按键服务实现：两段式状态机（先消抖，再判长短按）
 * @author Ricardo
 * @date 2026-11-01
 *
 * @note 分两步走，是为了让"毛刺"和"业务判定"互不污染：
 *       第一步只看时间，把抖动的电平滤成一根干净的方波；
 *       第二步只认这根方波，判定按下沿、长按、松开。
 *
 *       本文件没有 static 变量：一个按键一份上下文，多个按键互不干扰。
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include <stddef.h>
#include "svc_key_logic.h"

/*==============================================================================
 * 宏定义与常量
 *============================================================================*/
/** @brief 内部状态：松开 */
#define SVC_KEY_ST_IDLE     0u
/** @brief 内部状态：已按下（未到长按阈值） */
#define SVC_KEY_ST_DOWN     1u
/** @brief 内部状态：长按已触发 */
#define SVC_KEY_ST_LONG     2u

/*==============================================================================
 * 接口实现
 *============================================================================*/
/**
 * @brief 初始化一个按键的逻辑上下文
 * @param key           上下文，可为 NULL（安全返回）
 * @param raw           初始电平
 * @param long_press_ms 长按阈值；传 0 使用 SVC_KEY_LONG_PRESS_MS
 * @retval 无
 * @note 初始 stable 直接取当前电平：上电时按键正好被按住，
 *       不会凭空报一个 DOWN 后再报 CLICK。
 */
void Svc_Key_Logic_Init(svc_key_logic_t *key, uint8_t raw, uint32_t long_press_ms)
{
    if (key == NULL) {
        return;
    }

    key->state         = (raw != 0u) ? SVC_KEY_ST_DOWN : SVC_KEY_ST_IDLE;
    key->stable        = (raw != 0u) ? 1u : 0u;
    key->last_raw      = key->stable;
    key->debounce_ms   = 0u;
    key->press_ms      = 0u;
    key->long_press_ms = (long_press_ms != 0u) ? long_press_ms
                                               : SVC_KEY_LONG_PRESS_MS;
}

/**
 * @brief 喂一次采样，得到本周期的事件
 * @param key        上下文，不可为 NULL
 * @param raw        本次原始电平
 * @param elapsed_ms 距上次调用的毫秒数
 * @retval KEY_EVENT_NONE: 无事件
 * @retval KEY_EVENT_DOWN: 消抖后的按下沿
 * @retval KEY_EVENT_LONG: 长按触发
 * @retval KEY_EVENT_CLICK: 短按松开
 * @retval KEY_EVENT_LONG_UP: 长按后松开
 */
key_event_t Svc_Key_Logic_Tick(svc_key_logic_t *key, uint8_t raw, uint32_t elapsed_ms)
{
    uint8_t level;

    if (key == NULL) {
        return KEY_EVENT_NONE;
    }

    level = (raw != 0u) ? 1u : 0u;

    /* ---------- 1. 消抖 ---------- */
    if (level != key->last_raw) {
        /* 电平刚变，计时重新开始；这一次不产生任何事件 */
        key->last_raw    = level;
        key->debounce_ms = 0u;
        return KEY_EVENT_NONE;
    }

    if (key->debounce_ms < SVC_KEY_DEBOUNCE_MS) {
        key->debounce_ms += elapsed_ms;
        if (key->debounce_ms < SVC_KEY_DEBOUNCE_MS) {
            return KEY_EVENT_NONE;
        }
    }

    /* 到这里电平已经稳定 */
    key->stable = level;

    /* ---------- 2. 状态机 ---------- */
    switch (key->state) {
        case SVC_KEY_ST_IDLE:
            if (key->stable != 0u) {
                key->state    = SVC_KEY_ST_DOWN;
                key->press_ms = 0u;
                return KEY_EVENT_DOWN;
            }
            break;

        case SVC_KEY_ST_DOWN:
            if (key->stable != 0u) {
                key->press_ms += elapsed_ms;
                if (key->press_ms >= key->long_press_ms) {
                    key->state = SVC_KEY_ST_LONG;
                    return KEY_EVENT_LONG;
                }
            } else {
                key->state = SVC_KEY_ST_IDLE;
                return KEY_EVENT_CLICK;
            }
            break;

        case SVC_KEY_ST_LONG:
            if (key->stable == 0u) {
                key->state = SVC_KEY_ST_IDLE;
                return KEY_EVENT_LONG_UP;
            }
            break;

        default:
            /* 状态被写坏时退回空闲，避免卡死在中间态 */
            key->state = SVC_KEY_ST_IDLE;
            break;
    }

    return KEY_EVENT_NONE;
}
