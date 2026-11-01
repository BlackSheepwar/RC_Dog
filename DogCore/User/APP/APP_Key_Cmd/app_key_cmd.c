/**
 * @file app_key_cmd.c
 * @brief 按键业务实现：事件配置表 + 处理函数
 * @author Ricardo
 * @date 2026-11-01
 *
 * @note 原来的 switch(event) 套 switch(id) 换成了配置表：
 *       新增按键或新增事件只需加一行，分发函数不跟着改。
 *
 *       处理函数里只写"产品动作"，不写消抖、不写队列、不写延时。
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include <stddef.h>
#include "common.h"             /* ARRAY_SIZE */
#include "app_key_cmd.h"
// 功能包含
#include "dev_led.h"            /* Dev_LED_Toggle */
#include "app_uart.h"           /* APP_UART_Send */

/*==============================================================================
 * 宏定义与常量
 *============================================================================*/
/** @brief 测试命令字：按键按下时向链路 2 发一帧 0xAA */
#define APP_KEY_CMD_TEST_CMD    0xAAu

/*==============================================================================
 * 类型定义
 *============================================================================*/
/**
 * @brief 命令表项（值类型：表元素）
 */
typedef struct {
    uint8_t      key_id;    /* 业务按键编号 */
    key_event_t  event;     /* 触发事件 */
    void       (*handler)(void); /* 处理函数 */
} app_key_cmd_entry_t;

/*==============================================================================
 * 处理函数
 *============================================================================*/
/**
 * @brief 按键 1 按下：翻转指示灯并发一帧测试命令
 * @retval 无
 */
static void APP_Key_Cmd_Key1_Down(void)
{
    Dev_LED_Toggle(&dev_led_0);
    (void)APP_UART_Send(APP_UART_PORT_2, APP_KEY_CMD_TEST_CMD, NULL, 0u);
}

/**
 * @brief 按键 1 短按：暂未定义动作
 * @retval 无
 */
static void APP_Key_Cmd_Key1_Click(void)
{
}

/**
 * @brief 按键 1 长按：暂未定义动作
 * @retval 无
 */
static void APP_Key_Cmd_Key1_Long(void)
{
}

/**
 * @brief 按键 1 长按松开：暂未定义动作
 * @retval 无
 */
static void APP_Key_Cmd_Key1_Long_Up(void)
{
}

/*==============================================================================
 * 内部变量
 *============================================================================*/
/** @brief 按键命令表 */
static const app_key_cmd_entry_t APP_KEY_CMD_TABLE[] = {
    { .key_id = 1u, .event = KEY_EVENT_DOWN,    .handler = APP_Key_Cmd_Key1_Down },
    { .key_id = 1u, .event = KEY_EVENT_CLICK,   .handler = APP_Key_Cmd_Key1_Click },
    { .key_id = 1u, .event = KEY_EVENT_LONG,    .handler = APP_Key_Cmd_Key1_Long },
    { .key_id = 1u, .event = KEY_EVENT_LONG_UP, .handler = APP_Key_Cmd_Key1_Long_Up },
};

/*==============================================================================
 * 接口实现
 *============================================================================*/
/**
 * @brief 分发一次按键事件
 * @param key_id 业务按键编号
 * @param event  事件类型
 * @retval 无
 */
void APP_Key_Cmd_Dispatch(uint8_t key_id, key_event_t event)
{
    if (event == KEY_EVENT_NONE) {
        return;
    }

    for (uint8_t i = 0u; i < ARRAY_SIZE(APP_KEY_CMD_TABLE); i++) {
        if ((APP_KEY_CMD_TABLE[i].key_id == key_id)
            && (APP_KEY_CMD_TABLE[i].event == event)) {
            APP_KEY_CMD_TABLE[i].handler();
            return;
        }
    }
}
