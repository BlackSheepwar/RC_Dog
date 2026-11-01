/**
 * @file svc_key_logic.h
 * @brief 按键服务：消抖 + 短按/长按判定，纯逻辑、不碰硬件与 OS
 * @author Ricardo
 * @date 2026-11-01
 *
 * @note 为什么不是 APP：消抖与长按是"与产品无关的策略"，跟按键接的是
 *       哪个引脚、按下去要做什么都无关，因此属于 Service。
 *       本模块只收"当前电平 + 距上次调用过了多少毫秒"，
 *       不看时钟、不读 GPIO、不碰队列，可以直接在 PC 上单测。
 *
 *       时间由 TASK 层用 BSP_SYS_Get_Tick_Ms 差分后以 elapsed_ms 传进来，
 *       所以换 OS / 换节拍本模块一行不用改。
 *
 *       事件语义（一次按下只产生一条"沿"事件，不在按下期间反复上报）：
 *         DOWN    ── 消抖后的按下沿
 *         LONG    ── 按住超过 long_press_ms（只报一次）
 *         CLICK   ── 未到长按阈值就松开
 *         LONG_UP ── 长按之后松开
 */

#ifndef __Svc_Key_Logic_H__
#define __Svc_Key_Logic_H__

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include <stdint.h>

/*==============================================================================
 * 宏定义与常量
 *============================================================================*/
/** @brief 默认消抖时间：连续稳定这么久才认电平变化 */
#define SVC_KEY_DEBOUNCE_MS     20u
/** @brief 默认长按阈值：按住超过这么久算长按 */
#define SVC_KEY_LONG_PRESS_MS   500u

/*==============================================================================
 * 类型定义
 *============================================================================*/
/**
 * @brief 按键事件
 */
typedef enum {
    KEY_EVENT_NONE = 0,     /* 无事件 */
    KEY_EVENT_DOWN,         /* 消抖后的按下沿 */
    KEY_EVENT_LONG,         /* 长按触发（每次按下只报一次） */
    KEY_EVENT_CLICK,        /* 短按：未到长按阈值就松开 */
    KEY_EVENT_LONG_UP,      /* 长按后松开 */
} key_event_t;

/**
 * @brief 单个按键的逻辑上下文（值类型：调用方按值持有，通常一个数组）
 * @note 成员是内部实现，上层只负责 Init 与喂 Tick，不要直接改字段。
 */
typedef struct {
    uint8_t  state;         /* 内部状态机，见 .c 里的 s_ 定义 */
    uint8_t  stable;        /* 消抖后的稳定电平 */
    uint8_t  last_raw;      /* 上次采样到的原始电平 */
    uint32_t debounce_ms;   /* 电平稳定已持续的毫秒数 */
    uint32_t press_ms;      /* 已按住的毫秒数 */
    uint32_t long_press_ms; /* 本按键的长按阈值（可单独调） */
} svc_key_logic_t;

/**
 * @brief 按键事件消息（值类型：消息队列元素）
 * @note key_id 由扫描线程填，event 来自 Svc_Key_Logic_Tick。
 */
typedef struct {
    uint8_t     key_id;     /* 业务按键编号 */
    key_event_t event;      /* 事件类型 */
} key_event_packet_t;

/**
 * @brief 兼容别名：CubeMX 生成的 freertos.c 用旧名字建 KEY_CMD_Q
 * @note 生成代码在 USER CODE 之外，改不动；用别名让它继续编译。
 *       新代码一律用 key_event_packet_t。
 */
typedef key_event_packet_t App_Key_EventPacket_t;

/*==============================================================================
 * 接口
 *============================================================================*/
/**
 * @brief 初始化一个按键的逻辑上下文
 * @param key           上下文，可为 NULL（安全返回）
 * @param raw           初始电平（上电时读一次，避免误判出按下沿）
 * @param long_press_ms 长按阈值；传 0 使用 SVC_KEY_LONG_PRESS_MS
 * @retval 无
 */
void Svc_Key_Logic_Init(svc_key_logic_t *key, uint8_t raw, uint32_t long_press_ms);

/**
 * @brief 喂一次采样，得到本周期的事件
 * @param key        上下文，不可为 NULL
 * @param raw        本次原始电平（1 按下 / 0 松开，极性已由 Dev_Key 换算）
 * @param elapsed_ms 距上次调用的毫秒数
 * @retval KEY_EVENT_NONE: 无事件
 * @retval KEY_EVENT_DOWN: 消抖后的按下沿
 * @retval KEY_EVENT_LONG: 长按触发
 * @retval KEY_EVENT_CLICK: 短按松开
 * @retval KEY_EVENT_LONG_UP: 长按后松开
 * @note 每周期最多返回一个事件；按键为 NULL 时返回 KEY_EVENT_NONE。
 *       调用周期建议 10 ms：elapsed_ms 越大，长按阈值的时间误差越大。
 */
key_event_t Svc_Key_Logic_Tick(svc_key_logic_t *key, uint8_t raw, uint32_t elapsed_ms);

#endif /* __Svc_Key_Logic_H__ */
