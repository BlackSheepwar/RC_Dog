/**
 * @file task_key_debounce.c
 * @brief 按键扫描线程（入口名由 CubeMX .ioc 决定，不能改）：读设备 → 喂消抖服务 → 事件入队
 * @author Ricardo
 * @date 2026-11-01
 *
 * @note 本文件只回答三件事：多久跑一次、跑哪个函数、事件往哪个队列丢。
 *       电平极性在 Dev_Key，消抖与长短按在 Svc_Key_Logic，
 *       业务动作在 APP_Key_Cmd——线程体本身不超过 10 行逻辑。
 *
 *       elapsed_ms 由本层用 OS 节拍算（TASK 是唯一允许看 OS 时钟的层），
 *       服务层只收参数，因此换 OS 时只改这里。
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include <stdint.h>
#include "common.h"             /* ARRAY_SIZE */
#include "cmsis_os2.h"
#include "main.h"               /* KEY_CMD_QHandle（CubeMX 生成并定义） */
// 功能包含
#include "dev_key.h"
#include "svc_key_logic.h"

/*==============================================================================
 * 宏定义与常量
 *============================================================================*/
/** @brief 扫描周期（ms） */
#define TASK_KEY_SCAN_PERIOD_MS     10u

/*==============================================================================
 * 类型定义
 *============================================================================*/
/**
 * @brief 一路按键的绑定：设备句柄 + 逻辑上下文
 */
typedef struct {
    const dev_key_t *dev;       /* 设备句柄 */
    uint8_t          key_id;    /* 业务编号，与 APP_Key_Cmd 表一致 */
    svc_key_logic_t  logic;     /* 消抖/长短按上下文 */
} task_key_slot_t;

/*==============================================================================
 * 内部变量
 *============================================================================*/
/**
 * @brief 扫描表
 * @note 加一个按键 = 加一行（设备句柄已在 dev_key.c 里实例化）。
 *       长按阈值传 0 表示用服务层默认值。
 */
static task_key_slot_t s_keys[] = {
    { .dev = &dev_key_1, .key_id = 1u, .logic = { 0 } },
};

/*==============================================================================
 * 内部函数
 *============================================================================*/
/**
 * @brief 把所有按键的上下文对齐到当前电平
 * @retval 无
 * @note 上电时若按键正好被按住，我们不想报一个"假按下"，
 *       所以先读一次电平再 Init。
 * @note 按键设备的自检已在 freertos.c 的调度前统一完成，这里只对齐上下文。
 */
static void TASK_Key_Init(void)
{
    for (uint8_t i = 0u; i < ARRAY_SIZE(s_keys); i++) {
        uint8_t raw = Dev_Key_Is_Pressed(s_keys[i].dev);
        Svc_Key_Logic_Init(&s_keys[i].logic, raw, 0u);
    }
}

/*==============================================================================
 * 线程函数
 *============================================================================*/
/**
 * @brief 按键扫描线程（入口名由 CubeMX .ioc 决定，不能改）
 * @param argument 线程参数（未使用）
 * @retval 无
 * @note 每 TASK_KEY_SCAN_PERIOD_MS 采一次；有事件就投进 KEY_CMD_Q，
 *       队列满则丢弃（按键事件丢一条不影响功能，但绝不阻塞扫描）。
 */
void Task_KEY_Debounce(void *argument)
{
    uint32_t last_tick;

    (void)argument;

    TASK_Key_Init();
    last_tick = osKernelGetTickCount();

    for (;;) {
        uint32_t     now = osKernelGetTickCount();
        uint32_t     elapsed = now - last_tick;
        last_tick = now;

        /* ---------- 1. 逐路采样并推进状态机 ---------- */
        for (uint8_t i = 0u; i < ARRAY_SIZE(s_keys); i++) {
            task_key_slot_t *slot = &s_keys[i];
            uint8_t          raw  = Dev_Key_Is_Pressed(slot->dev);
            key_event_t      event;

            event = Svc_Key_Logic_Tick(&slot->logic, raw, elapsed);
            if (event == KEY_EVENT_NONE) {
                continue;
            }

            /* ---------- 2. 事件入队 ---------- */
            key_event_packet_t msg;
            msg.key_id = slot->key_id;
            msg.event  = event;
            (void)osMessageQueuePut(KEY_CMD_QHandle, &msg, 0u, 0u);
        }

        osDelay(TASK_KEY_SCAN_PERIOD_MS);
    }
}
