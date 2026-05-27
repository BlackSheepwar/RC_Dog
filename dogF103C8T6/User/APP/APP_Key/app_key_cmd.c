/**
 * @file app_key_cmd.c
 * @brief 按键命令分发
 * @author 李嘉图
 * @date 2026-04-18
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
#include "main.h"
#include "app_key_cmd.h"
#include "app_usart.h"
#include "app_se.h"
#include "bsp_can.h"

/*==============================================================================
 * 命令分发
 *============================================================================*/
static void App_Key_APP_A1(Debounce_Event_t event)
{
    switch (event) 
    {
        case KEY_EVENT_DOWN: 
        {
            APP_SE_SetincreaseTarget(1, 20);
        }
        break;     // 刚按下

        case KEY_EVENT_CLICK: 
        {
        }
        break;    // 短按释放

        case KEY_EVENT_LONG: 
        {
        }
        break;     // 长按触发

        case KEY_EVENT_LONG_UP: 
        {
        }
        break;  // 长按松开

        default: break; 
    }
}

static void App_Key_APP_A2(Debounce_Event_t event)
{
    switch (event) 
    {
        case KEY_EVENT_DOWN: 
        {
            APP_SE_SetincreaseTarget(1, -20);
        }
        break;     // 刚按下

        case KEY_EVENT_CLICK: 
        {
        }
        break;    // 短按释放

        case KEY_EVENT_LONG: 
        {
        }
        break;     // 长按触发

        case KEY_EVENT_LONG_UP: 
        {
        }
        break;  // 长按松开

        default: break; 
    }
}

static void App_Key_APP_A3(Debounce_Event_t event)
{
    switch (event) 
    {
        case KEY_EVENT_DOWN:
        {
            uint8_t data = 0x55;
            CAN_Send_Data(&hcan, 0x114, &data, 1);
        }
        break;     // 刚按下

        case KEY_EVENT_CLICK: 
        {
        }
        break;    // 短按释放

        case KEY_EVENT_LONG: 
        {
        }
        break;     // 长按触发

        case KEY_EVENT_LONG_UP: 
        {
            uint8_t data = 0x54;
            CAN_Send_Data(&hcan, 0x114, &data, 1);
        }
        break;  // 长按松开

        default: break; 
    }
}

static void App_Key_APP_A4(Debounce_Event_t event)
{
    switch (event) 
    {
        case KEY_EVENT_DOWN: 
        {
            HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        }
        break;     // 刚按下

        case KEY_EVENT_CLICK: 
        {
        }
        break;    // 短按释放

        case KEY_EVENT_LONG: 
        {
        }
        break;     // 长按触发

        case KEY_EVENT_LONG_UP: 
        {
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
        }
        break;  // 长按松开

        default: break; 
    }
}

static void App_Key_APP_A5(Debounce_Event_t event)
{
    switch (event) 
    {
        case KEY_EVENT_DOWN: 
        {
            HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        }
        break;     // 刚按下

        case KEY_EVENT_CLICK: 
        {
        }
        break;    // 短按释放

        case KEY_EVENT_LONG: 
        {
        }
        break;     // 长按触发

        case KEY_EVENT_LONG_UP: 
        {
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
        }
        break;  // 长按松开

        default: break; 
    }
}

/**
 * @brief 按键命令分发
 */
void App_Key_CMD_Packet(uint8_t id, Debounce_Event_t event)
{
    switch(id)
    {
        case 1: App_Key_APP_A1(event); break;
        case 2: App_Key_APP_A2(event); break;
        case 3: App_Key_APP_A3(event); break;
        case 4: App_Key_APP_A4(event); break;
        case 5: App_Key_APP_A5(event); break;

        default: break;
    }
}