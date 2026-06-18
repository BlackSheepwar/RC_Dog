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
// /**
//  * @brief 按键1按下：四腿姿态切换
//  * @note 每按一次在「中立」和「弯曲」两态间切换。
//  *       4条腿（舵机1~8）同步动作，每条腿 [髋, 膝]。
//  */
// static void Key1_OnDown(void)
// {
//     static uint8_t pose = 0;    /* 0=中立, 1=弯曲 */
//     pose ^= 1;                  /* 翻转状态 */

//     int16_t hip, knee;
//     if (pose)
//     {
//         hip  = 10;     /* 髋前摆 */
//         knee = -65;    /* 膝弯曲 */
//     }
//     else
//     {
//         hip  = 0;
//         knee = 0;
//     }

//     for (uint8_t i = 0; i < 4; i++)
//     {
//         APP_Servo_SetTarget(2 * i + 1, hip);
//         APP_Servo_SetTarget(2 * i + 2, knee);
//     }
//     HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_2);
// }

/**
 * @brief 按键1按下：四腿姿态切换
 * @note 每按一次在「中立」和「弯曲」两态间切换。
 *       4条腿（舵机1~8）同步动作，每条腿 [髋, 膝]。
 */
static void Key1_OnDown(void)
{
    APP_Servo_SetTarget(1, 10);
    APP_Servo_SetTarget(2, -10);       
    HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_2);
}

static void Key1_OnClick(void)
{
 
}

static void Key1_OnLong(void)
{
    APP_Servo_SetTarget(1, 90);
    APP_Servo_SetTarget(2, -90);       
    HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_2);
}

static void Key1_OnLongUp(void)
{
    APP_Servo_SetTarget(1,APP_Servo_GetCurrent(1));
    APP_Servo_SetTarget(2,APP_Servo_GetCurrent(2));    
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
