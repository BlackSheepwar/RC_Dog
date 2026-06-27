/**
 * @file app_key_cmd.c
 * @brief 按键命令分发（配置表驱动）
 * @author 李嘉图
 * @date 2026-06-18
 *
 * @note 原先的 switch(event) + switch(id) 已替换为事件配置表，
 *       新增按键或事件只需在 KEY_DISPATCH_TABLE 表中添加一行。
 *       处理函数保持原有的业务逻辑不变。
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include <stdint.h>
#include "main.h"
#include "app_key_cmd.h"
#include "common.h"
// 功能包含
#include "app_key.h"
#include "app_uart.h"
#include "app_servo.h"
#include "bsp_gpio.h"

/*==============================================================================
 * 事件配置表
 *============================================================================*/
typedef struct {
    uint8_t             key_id;
    App_Key_Event_t     event;
    void                (*handler)(void);
} key_dispatch_entry_t;

/*==============================================================================
 * 按键事件处理函数
 *============================================================================*/
static void Key1_OnDown(void)
{
    BSP_GPIO_Toggle(1);
    APP_UART_BuildTxPacket(1, 0xAA, NULL, 0);
}

static void Key1_OnClick(void)
{
}

static void Key1_OnLong(void)
{
}

static void Key1_OnLongUp(void)
{
    BSP_GPIO_SetLevel(1, GPIO_LEVEL_LOW);
}

/*==============================================================================
 * 按键命令分发
 *============================================================================*/

static const key_dispatch_entry_t KEY_DISPATCH_TABLE[] = {
    { .key_id = 1, .event = APP_KEY_EVENT_DOWN,    .handler = Key1_OnDown },
    { .key_id = 1, .event = APP_KEY_EVENT_CLICK,   .handler = Key1_OnClick },
    { .key_id = 1, .event = APP_KEY_EVENT_LONG,    .handler = Key1_OnLong },
    { .key_id = 1, .event = APP_KEY_EVENT_LONG_UP, .handler = Key1_OnLongUp },
};

/**
 * @brief 按键命令分发入口
 * @param key_id 逻辑按键 ID
 * @param event  按键事件
 */
void App_Key_CMD_Packet(uint8_t key_id, App_Key_Event_t event)
{
    for (uint8_t i = 0; i < ARRAY_SIZE(KEY_DISPATCH_TABLE); i++)
    {
        if (KEY_DISPATCH_TABLE[i].key_id == key_id && KEY_DISPATCH_TABLE[i].event == event)
        {
            KEY_DISPATCH_TABLE[i].handler();
            return;
        }
    }
}
