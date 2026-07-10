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

/*==============================================================================
 * 按键事件处理函数
 *============================================================================*/

/* --- Key1 ----------------------------------------- */
static void Key1_OnDown(void)    
{ 
    BSP_GPIO_Toggle(0);
    BSP_GPIO_Toggle(12);
    BSP_GPIO_Toggle(13);
    BSP_GPIO_Toggle(14);
    BSP_GPIO_Toggle(15);
    BSP_GPIO_Toggle(16);    
    BSP_GPIO_Toggle(17);
    BSP_GPIO_Toggle(18);
}
static void Key1_OnClick(void)   { }
static void Key1_OnLong(void)    { }
static void Key1_OnLongUp(void)  
{ 
    BSP_GPIO_SetLevel(0,  GPIO_LEVEL_HIGH);
    BSP_GPIO_SetLevel(12, GPIO_LEVEL_LOW);
    BSP_GPIO_SetLevel(13, GPIO_LEVEL_LOW);
    BSP_GPIO_SetLevel(14, GPIO_LEVEL_LOW);
    BSP_GPIO_SetLevel(15, GPIO_LEVEL_LOW);
    BSP_GPIO_SetLevel(16, GPIO_LEVEL_LOW);    
    BSP_GPIO_SetLevel(17, GPIO_LEVEL_LOW);
    BSP_GPIO_SetLevel(18, GPIO_LEVEL_LOW);
}

/* --- Key2 ----------------------------------------- */
static void Key2_OnDown(void)
{
    BSP_GPIO_Toggle(0);
    BSP_GPIO_Toggle(12);
    BSP_GPIO_Toggle(13);
    BSP_GPIO_Toggle(14);
    BSP_GPIO_Toggle(15);
    BSP_GPIO_Toggle(17);
    BSP_GPIO_Toggle(18);
}
static void Key2_OnClick(void)   { }
static void Key2_OnLong(void)    { }
static void Key2_OnLongUp(void)
{
    BSP_GPIO_SetLevel(0,  GPIO_LEVEL_HIGH);
    BSP_GPIO_SetLevel(12, GPIO_LEVEL_LOW);
    BSP_GPIO_SetLevel(13, GPIO_LEVEL_LOW);
    BSP_GPIO_SetLevel(14, GPIO_LEVEL_LOW);
    BSP_GPIO_SetLevel(15, GPIO_LEVEL_LOW);
    BSP_GPIO_SetLevel(17, GPIO_LEVEL_LOW);
    BSP_GPIO_SetLevel(18, GPIO_LEVEL_LOW);
}

/* --- Key3 ----------------------------------------- */
static void Key3_OnDown(void)
{
    BSP_GPIO_Toggle(0);
    BSP_GPIO_Toggle(12);
    BSP_GPIO_Toggle(13);
    BSP_GPIO_Toggle(14);
    BSP_GPIO_Toggle(15);
    BSP_GPIO_Toggle(17);
    BSP_GPIO_Toggle(18);
}
static void Key3_OnClick(void)   { }
static void Key3_OnLong(void)    { }
static void Key3_OnLongUp(void)
{
    BSP_GPIO_SetLevel(0,  GPIO_LEVEL_HIGH);
    BSP_GPIO_SetLevel(12, GPIO_LEVEL_LOW);
    BSP_GPIO_SetLevel(13, GPIO_LEVEL_LOW);
    BSP_GPIO_SetLevel(14, GPIO_LEVEL_LOW);
    BSP_GPIO_SetLevel(15, GPIO_LEVEL_LOW);
    BSP_GPIO_SetLevel(17, GPIO_LEVEL_LOW);
    BSP_GPIO_SetLevel(18, GPIO_LEVEL_LOW);
}

/* --- Key4 ----------------------------------------- */
static void Key4_OnDown(void)
{
    BSP_GPIO_Toggle(0);
    BSP_GPIO_Toggle(12);
    BSP_GPIO_Toggle(13);
    BSP_GPIO_Toggle(14);
    BSP_GPIO_Toggle(15);
    BSP_GPIO_Toggle(17);
    BSP_GPIO_Toggle(18);
}
static void Key4_OnClick(void)   { }
static void Key4_OnLong(void)    { }
static void Key4_OnLongUp(void)
{
    BSP_GPIO_SetLevel(0,  GPIO_LEVEL_HIGH);
    BSP_GPIO_SetLevel(12, GPIO_LEVEL_LOW);
    BSP_GPIO_SetLevel(13, GPIO_LEVEL_LOW);
    BSP_GPIO_SetLevel(14, GPIO_LEVEL_LOW);
    BSP_GPIO_SetLevel(15, GPIO_LEVEL_LOW);
    BSP_GPIO_SetLevel(17, GPIO_LEVEL_LOW);
    BSP_GPIO_SetLevel(18, GPIO_LEVEL_LOW);
}

/* --- Key5 ----------------------------------------- */
static void Key5_OnDown(void)
{
    BSP_GPIO_Toggle(0);
    BSP_GPIO_Toggle(12);
    BSP_GPIO_Toggle(13);
    BSP_GPIO_Toggle(14);
    BSP_GPIO_Toggle(15);
    BSP_GPIO_Toggle(17);
    BSP_GPIO_Toggle(18);
}
static void Key5_OnClick(void)   { }
static void Key5_OnLong(void)    { }
static void Key5_OnLongUp(void)
{
    BSP_GPIO_SetLevel(0,  GPIO_LEVEL_HIGH);
    BSP_GPIO_SetLevel(12, GPIO_LEVEL_LOW);
    BSP_GPIO_SetLevel(13, GPIO_LEVEL_LOW);
    BSP_GPIO_SetLevel(14, GPIO_LEVEL_LOW);
    BSP_GPIO_SetLevel(15, GPIO_LEVEL_LOW);
    BSP_GPIO_SetLevel(17, GPIO_LEVEL_LOW);
    BSP_GPIO_SetLevel(18, GPIO_LEVEL_LOW);
}

/* --- Key6 ----------------------------------------- */
static void Key6_OnDown(void)
{
    BSP_GPIO_Toggle(0);
    BSP_GPIO_Toggle(12);
    BSP_GPIO_Toggle(13);
    BSP_GPIO_Toggle(14);
    BSP_GPIO_Toggle(15);
    BSP_GPIO_Toggle(17);
    BSP_GPIO_Toggle(18);
}
static void Key6_OnClick(void)   { }
static void Key6_OnLong(void)    { }
static void Key6_OnLongUp(void)
{
    BSP_GPIO_SetLevel(0,  GPIO_LEVEL_HIGH);
    BSP_GPIO_SetLevel(12, GPIO_LEVEL_LOW);
    BSP_GPIO_SetLevel(13, GPIO_LEVEL_LOW);
    BSP_GPIO_SetLevel(14, GPIO_LEVEL_LOW);
    BSP_GPIO_SetLevel(15, GPIO_LEVEL_LOW);
    BSP_GPIO_SetLevel(17, GPIO_LEVEL_LOW);
    BSP_GPIO_SetLevel(18, GPIO_LEVEL_LOW);
}

/* --- Key7 ----------------------------------------- */
static void Key7_OnDown(void)
{
    BSP_GPIO_Toggle(0);
    BSP_GPIO_Toggle(12);
    BSP_GPIO_Toggle(13);
    BSP_GPIO_Toggle(14);
    BSP_GPIO_Toggle(15);
    BSP_GPIO_Toggle(17);
    BSP_GPIO_Toggle(18);
}
static void Key7_OnClick(void)   { }
static void Key7_OnLong(void)    { }
static void Key7_OnLongUp(void)
{
    BSP_GPIO_SetLevel(0,  GPIO_LEVEL_HIGH);
    BSP_GPIO_SetLevel(12, GPIO_LEVEL_LOW);
    BSP_GPIO_SetLevel(13, GPIO_LEVEL_LOW);
    BSP_GPIO_SetLevel(14, GPIO_LEVEL_LOW);
    BSP_GPIO_SetLevel(15, GPIO_LEVEL_LOW);
    BSP_GPIO_SetLevel(17, GPIO_LEVEL_LOW);
    BSP_GPIO_SetLevel(18, GPIO_LEVEL_LOW);
}

/* --- Key8 ----------------------------------------- */
static void Key8_OnDown(void)
{
    BSP_GPIO_Toggle(0);
    BSP_GPIO_Toggle(12);
    BSP_GPIO_Toggle(13);
    BSP_GPIO_Toggle(14);
    BSP_GPIO_Toggle(15);
    BSP_GPIO_Toggle(17);
    BSP_GPIO_Toggle(18);
}
static void Key8_OnClick(void)   { }
static void Key8_OnLong(void)    { }
static void Key8_OnLongUp(void)
{
    BSP_GPIO_SetLevel(0,  GPIO_LEVEL_HIGH);
    BSP_GPIO_SetLevel(12, GPIO_LEVEL_LOW);
    BSP_GPIO_SetLevel(13, GPIO_LEVEL_LOW);
    BSP_GPIO_SetLevel(14, GPIO_LEVEL_LOW);
    BSP_GPIO_SetLevel(15, GPIO_LEVEL_LOW);
    BSP_GPIO_SetLevel(17, GPIO_LEVEL_LOW);
    BSP_GPIO_SetLevel(18, GPIO_LEVEL_LOW);
}

/* --- Key9 ----------------------------------------- */
static void Key9_OnDown(void)
{
    BSP_GPIO_Toggle(0);
    BSP_GPIO_Toggle(12);
    BSP_GPIO_Toggle(13);
    BSP_GPIO_Toggle(14);
    BSP_GPIO_Toggle(15);
    BSP_GPIO_Toggle(17);
    BSP_GPIO_Toggle(18);
}
static void Key9_OnClick(void)   { }
static void Key9_OnLong(void)    { }
static void Key9_OnLongUp(void)
{
    BSP_GPIO_SetLevel(0,  GPIO_LEVEL_HIGH);
    BSP_GPIO_SetLevel(12, GPIO_LEVEL_LOW);
    BSP_GPIO_SetLevel(13, GPIO_LEVEL_LOW);
    BSP_GPIO_SetLevel(14, GPIO_LEVEL_LOW);
    BSP_GPIO_SetLevel(15, GPIO_LEVEL_LOW);
    BSP_GPIO_SetLevel(17, GPIO_LEVEL_LOW);
    BSP_GPIO_SetLevel(18, GPIO_LEVEL_LOW);
}

/* --- Key10 ---------------------------------------- */
static void Key10_OnDown(void)
{
    BSP_GPIO_Toggle(0);
    BSP_GPIO_Toggle(12);
    BSP_GPIO_Toggle(13);
    BSP_GPIO_Toggle(14);
    BSP_GPIO_Toggle(15);
    BSP_GPIO_Toggle(17);
    BSP_GPIO_Toggle(18);
}
static void Key10_OnClick(void)  { }
static void Key10_OnLong(void)   { }
static void Key10_OnLongUp(void)
{
    BSP_GPIO_SetLevel(0,  GPIO_LEVEL_HIGH);
    BSP_GPIO_SetLevel(12, GPIO_LEVEL_LOW);
    BSP_GPIO_SetLevel(13, GPIO_LEVEL_LOW);
    BSP_GPIO_SetLevel(14, GPIO_LEVEL_LOW);
    BSP_GPIO_SetLevel(15, GPIO_LEVEL_LOW);
    BSP_GPIO_SetLevel(17, GPIO_LEVEL_LOW);
    BSP_GPIO_SetLevel(18, GPIO_LEVEL_LOW);
}

/* --- Key11 ---------------------------------------- */
static void Key11_OnDown(void)
{
    BSP_GPIO_Toggle(0);
    BSP_GPIO_Toggle(12);
    BSP_GPIO_Toggle(13);
    BSP_GPIO_Toggle(14);
    BSP_GPIO_Toggle(15);
    BSP_GPIO_Toggle(17);
    BSP_GPIO_Toggle(18);
}
static void Key11_OnClick(void)  { }
static void Key11_OnLong(void)   { }
static void Key11_OnLongUp(void)
{
    BSP_GPIO_SetLevel(0,  GPIO_LEVEL_HIGH);
    BSP_GPIO_SetLevel(12, GPIO_LEVEL_LOW);
    BSP_GPIO_SetLevel(13, GPIO_LEVEL_LOW);
    BSP_GPIO_SetLevel(14, GPIO_LEVEL_LOW);
    BSP_GPIO_SetLevel(15, GPIO_LEVEL_LOW);
    BSP_GPIO_SetLevel(17, GPIO_LEVEL_LOW);
    BSP_GPIO_SetLevel(18, GPIO_LEVEL_LOW);
}

/* --- Key12 ---------------------------------------- */
static void Key12_OnDown(void)
{
    BSP_GPIO_Toggle(0);
    BSP_GPIO_Toggle(12);
    BSP_GPIO_Toggle(13);
    BSP_GPIO_Toggle(14);
    BSP_GPIO_Toggle(15);
    BSP_GPIO_Toggle(17);
    BSP_GPIO_Toggle(18);
}
static void Key12_OnClick(void)  { }
static void Key12_OnLong(void)   { }
static void Key12_OnLongUp(void)
{
    BSP_GPIO_SetLevel(0,  GPIO_LEVEL_HIGH);
    BSP_GPIO_SetLevel(12, GPIO_LEVEL_LOW);
    BSP_GPIO_SetLevel(13, GPIO_LEVEL_LOW);
    BSP_GPIO_SetLevel(14, GPIO_LEVEL_LOW);
    BSP_GPIO_SetLevel(15, GPIO_LEVEL_LOW);
    BSP_GPIO_SetLevel(17, GPIO_LEVEL_LOW);
    BSP_GPIO_SetLevel(18, GPIO_LEVEL_LOW);
}

/*==============================================================================
 * 按键命令分发
 *============================================================================*/

static const key_dispatch_entry_t KEY_DISPATCH_TABLE[] = {
    { .key_id = 1,  .event = APP_KEY_EVENT_DOWN,    .handler = Key1_OnDown    },
    { .key_id = 1,  .event = APP_KEY_EVENT_CLICK,   .handler = Key1_OnClick   },
    { .key_id = 1,  .event = APP_KEY_EVENT_LONG,    .handler = Key1_OnLong    },
    { .key_id = 1,  .event = APP_KEY_EVENT_LONG_UP, .handler = Key1_OnLongUp  },
    { .key_id = 2,  .event = APP_KEY_EVENT_DOWN,    .handler = Key2_OnDown    },
    { .key_id = 2,  .event = APP_KEY_EVENT_CLICK,   .handler = Key2_OnClick   },
    { .key_id = 2,  .event = APP_KEY_EVENT_LONG,    .handler = Key2_OnLong    },
    { .key_id = 2,  .event = APP_KEY_EVENT_LONG_UP, .handler = Key2_OnLongUp  },
    { .key_id = 3,  .event = APP_KEY_EVENT_DOWN,    .handler = Key3_OnDown    },
    { .key_id = 3,  .event = APP_KEY_EVENT_CLICK,   .handler = Key3_OnClick   },
    { .key_id = 3,  .event = APP_KEY_EVENT_LONG,    .handler = Key3_OnLong    },
    { .key_id = 3,  .event = APP_KEY_EVENT_LONG_UP, .handler = Key3_OnLongUp  },
    { .key_id = 4,  .event = APP_KEY_EVENT_DOWN,    .handler = Key4_OnDown    },
    { .key_id = 4,  .event = APP_KEY_EVENT_CLICK,   .handler = Key4_OnClick   },
    { .key_id = 4,  .event = APP_KEY_EVENT_LONG,    .handler = Key4_OnLong    },
    { .key_id = 4,  .event = APP_KEY_EVENT_LONG_UP, .handler = Key4_OnLongUp  },
    { .key_id = 5,  .event = APP_KEY_EVENT_DOWN,    .handler = Key5_OnDown    },
    { .key_id = 5,  .event = APP_KEY_EVENT_CLICK,   .handler = Key5_OnClick   },
    { .key_id = 5,  .event = APP_KEY_EVENT_LONG,    .handler = Key5_OnLong    },
    { .key_id = 5,  .event = APP_KEY_EVENT_LONG_UP, .handler = Key5_OnLongUp  },
    { .key_id = 6,  .event = APP_KEY_EVENT_DOWN,    .handler = Key6_OnDown    },
    { .key_id = 6,  .event = APP_KEY_EVENT_CLICK,   .handler = Key6_OnClick   },
    { .key_id = 6,  .event = APP_KEY_EVENT_LONG,    .handler = Key6_OnLong    },
    { .key_id = 6,  .event = APP_KEY_EVENT_LONG_UP, .handler = Key6_OnLongUp  },
    { .key_id = 7,  .event = APP_KEY_EVENT_DOWN,    .handler = Key7_OnDown    },
    { .key_id = 7,  .event = APP_KEY_EVENT_CLICK,   .handler = Key7_OnClick   },
    { .key_id = 7,  .event = APP_KEY_EVENT_LONG,    .handler = Key7_OnLong    },
    { .key_id = 7,  .event = APP_KEY_EVENT_LONG_UP, .handler = Key7_OnLongUp  },
    { .key_id = 8,  .event = APP_KEY_EVENT_DOWN,    .handler = Key8_OnDown    },
    { .key_id = 8,  .event = APP_KEY_EVENT_CLICK,   .handler = Key8_OnClick   },
    { .key_id = 8,  .event = APP_KEY_EVENT_LONG,    .handler = Key8_OnLong    },
    { .key_id = 8,  .event = APP_KEY_EVENT_LONG_UP, .handler = Key8_OnLongUp  },
    { .key_id = 9,  .event = APP_KEY_EVENT_DOWN,    .handler = Key9_OnDown    },
    { .key_id = 9,  .event = APP_KEY_EVENT_CLICK,   .handler = Key9_OnClick   },
    { .key_id = 9,  .event = APP_KEY_EVENT_LONG,    .handler = Key9_OnLong    },
    { .key_id = 9,  .event = APP_KEY_EVENT_LONG_UP, .handler = Key9_OnLongUp  },
    { .key_id = 10, .event = APP_KEY_EVENT_DOWN,    .handler = Key10_OnDown   },
    { .key_id = 10, .event = APP_KEY_EVENT_CLICK,   .handler = Key10_OnClick  },
    { .key_id = 10, .event = APP_KEY_EVENT_LONG,    .handler = Key10_OnLong   },
    { .key_id = 10, .event = APP_KEY_EVENT_LONG_UP, .handler = Key10_OnLongUp },
    { .key_id = 11, .event = APP_KEY_EVENT_DOWN,    .handler = Key11_OnDown   },
    { .key_id = 11, .event = APP_KEY_EVENT_CLICK,   .handler = Key11_OnClick  },
    { .key_id = 11, .event = APP_KEY_EVENT_LONG,    .handler = Key11_OnLong   },
    { .key_id = 11, .event = APP_KEY_EVENT_LONG_UP, .handler = Key11_OnLongUp },
    { .key_id = 12, .event = APP_KEY_EVENT_DOWN,    .handler = Key12_OnDown   },
    { .key_id = 12, .event = APP_KEY_EVENT_CLICK,   .handler = Key12_OnClick  },
    { .key_id = 12, .event = APP_KEY_EVENT_LONG,    .handler = Key12_OnLong   },
    { .key_id = 12, .event = APP_KEY_EVENT_LONG_UP, .handler = Key12_OnLongUp },
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
