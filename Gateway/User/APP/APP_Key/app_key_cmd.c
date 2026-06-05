/**
 * @file app_key_cmd.c
 * @brief 按键命令分发 - 调用操作层接口
 * @author 李嘉图
 * @date 2026-06-04
 *
 * @note 只负责"按键事件 → 操作接口"映射，不含业务逻辑。
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
#include "main.h"
#include "app_key_cmd.h"
#include "app_operation.h"

/*==============================================================================
 * 按键处理函数
 *============================================================================*/

/** @brief 按键 1 -  */
static void App_Key_APP_A1(Debounce_Event_t event)
{
    switch (event)
    {
        case KEY_EVENT_DOWN:   break;
        case KEY_EVENT_CLICK:  break;
        case KEY_EVENT_LONG:   break;
        case KEY_EVENT_LONG_UP:break;
        default: break;
    }
}

/** @brief 按键 2 -  */
static void App_Key_APP_A2(Debounce_Event_t event)
{
    switch (event)
    {
        case KEY_EVENT_DOWN:   break;
        case KEY_EVENT_CLICK:  break;
        case KEY_EVENT_LONG:   break;
        case KEY_EVENT_LONG_UP:break;
        default: break;
    }
}

/** @brief 按键 3 - 前腿蹲下 */
static void App_Key_APP_A3(Debounce_Event_t event)
{
    switch (event)
    {
        case KEY_EVENT_DOWN:
            APP_OP_LegSit(LEG1);
            APP_OP_LegSit(LEG2);
            break;
        case KEY_EVENT_CLICK:  break;
        case KEY_EVENT_LONG:   break;
        case KEY_EVENT_LONG_UP:break;
        default: break;
    }
}

/** @brief 按键 4 - 后退蹲下 */
static void App_Key_APP_A4(Debounce_Event_t event)
{
    switch (event)
    {
        case KEY_EVENT_DOWN:
            APP_OP_LegSit(LEG3);
            APP_OP_LegSit(LEG4);
            break;
        case KEY_EVENT_CLICK:  break;
        case KEY_EVENT_LONG:   break;
        case KEY_EVENT_LONG_UP:break;
        default: break;
    }
}

/** @brief 按键 5 - 前腿站立 */
static void App_Key_APP_A5(Debounce_Event_t event)
{
    switch (event)
    {
        case KEY_EVENT_DOWN:
            APP_OP_LegStand(LEG1);
            APP_OP_LegStand(LEG2);
            break;
        case KEY_EVENT_CLICK:  break;
        case KEY_EVENT_LONG:   break;
        case KEY_EVENT_LONG_UP:break;
        default: break;
    }
}

/** @brief 按键 6 - 后退站立 */
static void App_Key_APP_A6(Debounce_Event_t event)
{
    switch (event)
    {
        case KEY_EVENT_DOWN:
            APP_OP_LegStand(LEG3);
            APP_OP_LegStand(LEG4);
            break;
        case KEY_EVENT_CLICK:  break;
        case KEY_EVENT_LONG:   break;
        case KEY_EVENT_LONG_UP:break;
        default: break;
    }
}

/*==============================================================================
 * 命令分发入口
 *============================================================================*/
void App_Key_CMD_Packet(uint8_t id, Debounce_Event_t event)
{
    switch(id)
    {
        case 1: App_Key_APP_A1(event); break;
        case 2: App_Key_APP_A2(event); break;
        case 3: App_Key_APP_A3(event); break;
        case 4: App_Key_APP_A4(event); break;
        case 5: App_Key_APP_A5(event); break;
        case 6: App_Key_APP_A6(event); break;
        default: break;
    }
}
