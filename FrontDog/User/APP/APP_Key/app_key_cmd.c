/**
 * @file app_key_cmd.c
 * @brief 按键命令分发（配置表驱动）
 * @author 李嘉图
 * @date 2026-6-5
 *
 * @note 原先的 switch(event) + switch(id) 已替换为事件配置表，
 *       新增按键或事件只需在 KEY_EVENTS 表中添加一行。
 *       处理函数保持原有的业务逻辑不变。
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
#include "main.h"
#include "app_key_cmd.h"
#include "app_usart.h"
#include "app_servo.h"
#include "bsp_can.h"
#include "common.h"

/*==============================================================================
 * 按键事件处理函数
 *============================================================================*/
static void Key1_OnDown(void)
{
    APP_Servo_SetincreaseTarget(1, -25);
}

static void Key1_OnClick(void)
{
 
}

static void Key1_OnLong(void)
{
    APP_Servo_SetTarget(1, -130);
}

static void Key1_OnLongUp(void)
{
    APP_Servo_SetTarget(1, APP_Servo_GetCurrent(1));
}

/*==============================================================================
 * 按键命令分发
 *============================================================================*/

static const key_dispatch_entry_t KEY_DISPATCH_TABLE[] = {
    { .key_id = 1, .event = KEY_EVENT_DOWN,    .handler = Key1_OnDown },
    { .key_id = 1, .event = KEY_EVENT_CLICK,   .handler = Key1_OnClick },
    { .key_id = 1, .event = KEY_EVENT_LONG,    .handler = Key1_OnLong },
    { .key_id = 1, .event = KEY_EVENT_LONG_UP, .handler = Key1_OnLongUp },
};

/**
 * @brief 按键命令分发入口
 * @param id    按键ID
 * @param event 按键事件
 */
void App_Key_CMD_Packet(uint8_t id, Debounce_Event_t event)
{
    for (uint8_t i = 0; i < ARRAY_SIZE(KEY_DISPATCH_TABLE); i++)
    {
        if (KEY_DISPATCH_TABLE[i].key_id == id && KEY_DISPATCH_TABLE[i].event == event)
        {
            KEY_DISPATCH_TABLE[i].handler();
            return;
        }
    }
}
