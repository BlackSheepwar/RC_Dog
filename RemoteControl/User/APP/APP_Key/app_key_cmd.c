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
#include "bsp_gpio.h"

/*==============================================================================
 * 事件配置表
 *============================================================================*/
typedef struct {
    uint8_t             key_id;
    App_Key_Event_t     event;
    void                (*handler)(void);
} key_dispatch_entry_t;

/*==================================================================
============
 * 按键事件处理函数
 *============================================================================*/
// key1
static void Key1_OnDown(void)
{
    BSP_GPIO_Toggle(0);
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
    BSP_GPIO_SetLevel(0, GPIO_LEVEL_HIGH);
}

// key2
static void Key2_OnDown(void)
{
    BSP_GPIO_Toggle(0);
}

static void Key2_OnClick(void)
{
}

static void Key2_OnLong(void)
{
}

static void Key2_OnLongUp(void)
{
    BSP_GPIO_SetLevel(0, GPIO_LEVEL_HIGH);
}

// key3
static void Key3_OnDown(void)
{
    BSP_GPIO_Toggle(0);
}

static void Key3_OnClick(void)
{
}

static void Key3_OnLong(void)
{
}

static void Key3_OnLongUp(void)
{
    BSP_GPIO_SetLevel(0, GPIO_LEVEL_HIGH);
}

// key4
static void Key4_OnDown(void)
{
    BSP_GPIO_Toggle(0);
}

static void Key4_OnClick(void)
{
}

static void Key4_OnLong(void)
{
}

static void Key4_OnLongUp(void)
{
    BSP_GPIO_SetLevel(0, GPIO_LEVEL_HIGH);
}

// key5
static void Key5_OnDown(void)
{
    BSP_GPIO_Toggle(0);
}

static void Key5_OnClick(void)
{
}

static void Key5_OnLong(void)
{
}

static void Key5_OnLongUp(void)
{
    BSP_GPIO_SetLevel(0, GPIO_LEVEL_HIGH);
}

// key6
static void Key6_OnDown(void)
{
    BSP_GPIO_Toggle(0);
}

static void Key6_OnClick(void)
{
}

static void Key6_OnLong(void)
{
}


static void Key6_OnLongUp(void)
{
    BSP_GPIO_SetLevel(0, GPIO_LEVEL_HIGH);
}

/*==============================================================================
 * 按键命令分发
 *============================================================================*/

static const key_dispatch_entry_t KEY_DISPATCH_TABLE[] = {
    /* Key1 */
    { .key_id = 1, .event = APP_KEY_EVENT_DOWN,    .handler = Key1_OnDown },
    { .key_id = 1, .event = APP_KEY_EVENT_CLICK,   .handler = Key1_OnClick },
    { .key_id = 1, .event = APP_KEY_EVENT_LONG,    .handler = Key1_OnLong },
    { .key_id = 1, .event = APP_KEY_EVENT_LONG_UP, .handler = Key1_OnLongUp },
    /* Key2 */
    { .key_id = 2, .event = APP_KEY_EVENT_DOWN,    .handler = Key2_OnDown },
    { .key_id = 2, .event = APP_KEY_EVENT_CLICK,   .handler = Key2_OnClick },
    { .key_id = 2, .event = APP_KEY_EVENT_LONG,    .handler = Key2_OnLong },
    { .key_id = 2, .event = APP_KEY_EVENT_LONG_UP, .handler = Key2_OnLongUp },
    /* Key3 */
    { .key_id = 3, .event = APP_KEY_EVENT_DOWN,    .handler = Key3_OnDown },
    { .key_id = 3, .event = APP_KEY_EVENT_CLICK,   .handler = Key3_OnClick },
    { .key_id = 3, .event = APP_KEY_EVENT_LONG,    .handler = Key3_OnLong },
    { .key_id = 3, .event = APP_KEY_EVENT_LONG_UP, .handler = Key3_OnLongUp },
    /* Key4 */
    { .key_id = 4, .event = APP_KEY_EVENT_DOWN,    .handler = Key4_OnDown },
    { .key_id = 4, .event = APP_KEY_EVENT_CLICK,   .handler = Key4_OnClick },
    { .key_id = 4, .event = APP_KEY_EVENT_LONG,    .handler = Key4_OnLong },
    { .key_id = 4, .event = APP_KEY_EVENT_LONG_UP, .handler = Key4_OnLongUp },
    /* Key5 */
    { .key_id = 5, .event = APP_KEY_EVENT_DOWN,    .handler = Key5_OnDown },
    { .key_id = 5, .event = APP_KEY_EVENT_CLICK,   .handler = Key5_OnClick },
    { .key_id = 5, .event = APP_KEY_EVENT_LONG,    .handler = Key5_OnLong },
    { .key_id = 5, .event = APP_KEY_EVENT_LONG_UP, .handler = Key5_OnLongUp },
    /* Key6 */
    { .key_id = 6, .event = APP_KEY_EVENT_DOWN,    .handler = Key6_OnDown },
    { .key_id = 6, .event = APP_KEY_EVENT_CLICK,   .handler = Key6_OnClick },
    { .key_id = 6, .event = APP_KEY_EVENT_LONG,    .handler = Key6_OnLong },
    { .key_id = 6, .event = APP_KEY_EVENT_LONG_UP, .handler = Key6_OnLongUp },
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
