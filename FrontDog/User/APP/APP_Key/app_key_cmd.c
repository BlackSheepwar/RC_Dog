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
#include "app_se.h"
#include "bsp_can.h"
#include "common.h"

/*==============================================================================
 * 事件配置表
 *============================================================================*/
typedef struct {
    Debounce_Event_t event;
    void             (*handler)(void);
} key_event_entry_t;

/*==============================================================================
 * 按键事件处理函数
 *============================================================================*/
static void Key1_OnDown(void)
{
    HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_2);
}

static void Key1_OnClick(void)
{
}

static void Key1_OnLong(void)
{
}

static void Key1_OnLongUp(void)
{
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_RESET);
}

/* 按键1的事件映射表 */
static const key_event_entry_t KEY1_EVENTS[] = {
    { .event = KEY_EVENT_DOWN,    .handler = Key1_OnDown },
    { .event = KEY_EVENT_CLICK,   .handler = Key1_OnClick },
    { .event = KEY_EVENT_LONG,    .handler = Key1_OnLong },
    { .event = KEY_EVENT_LONG_UP, .handler = Key1_OnLongUp },
};

/*==============================================================================
 * 按键命令分发
 *============================================================================*/
/**
 * @brief 按键1命令处理
 * @param event 按键事件
 */
static void App_Key_APP_A1(Debounce_Event_t event)
{
    for (uint8_t i = 0; i < ARRAY_SIZE(KEY1_EVENTS); i++)
    {
        if (KEY1_EVENTS[i].event == event)
        {
            KEY1_EVENTS[i].handler();
            return;
        }
    }
}

/* 按键ID → 处理函数映射表 */
typedef struct {
    uint8_t key_id;
    void    (*handler)(Debounce_Event_t event);
} key_dispatch_entry_t;

static const key_dispatch_entry_t KEY_DISPATCH_TABLE[] = {
    { .key_id = 1, .handler = App_Key_APP_A1 },
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
        if (KEY_DISPATCH_TABLE[i].key_id == id)
        {
            KEY_DISPATCH_TABLE[i].handler(event);
            return;
        }
    }
}
