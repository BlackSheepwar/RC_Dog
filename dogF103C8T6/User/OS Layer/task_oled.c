/**
 * @file task_oled.c
 * @brief 实现OLED任务
 * @author 李嘉图
 * @date 2026-06-03
 *
 * @note 显示3个舵机的角度、光标位置和电位器锁定状态。
 *       使用 afont16x8 字体（16px高 x 8px宽），每行16px。
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
#include "main.h"
#include <stdio.h>
#include "oled.h"
#include "oled_extend.h"
#include "app_se.h"
#include "app_cursor.h"
#include "app_knob.h"

/*==============================================================================
 * 任务函数
 *============================================================================*/
void StartOLEDTask(void *argument)
{
    char buf[16];

    OLED_Init();

    for(;;)
    {
        OLED_NewFrame();

        /* ---- 显示3个舵机 ---- */
        for (uint8_t i = 0; i < 3; i++)
        {
            uint8_t id    = i + 1;
            int16_t angle = APP_SE_GetCurrent(id);
            uint8_t y     = i * 16;
            char    cursor = (g_cursor.current_id == id) ? '>' : ' ';

            sprintf(buf, "%c%d:%+04d", cursor, id, angle);
            OLED_PrintASCIIString(0, y, buf, &afont16x8, OLED_COLOR_NORMAL);
        }

        /* ---- 状态指示（第4行） ---- */
        uint8_t knob_en = APP_KNOB_GetEnable(1);
        if (!knob_en)
        {
            OLED_PrintASCIIString(0, 48, "K:OFF", &afont16x8, OLED_COLOR_NORMAL);
        }
        else if (g_cursor.lock)
        {
            OLED_PrintASCIIString(0, 48, " LOCK", &afont16x8, OLED_COLOR_NORMAL);
        }
        else
        {
            OLED_PrintASCIIString(0, 48, "K:ON ", &afont16x8, OLED_COLOR_NORMAL);
        }

        /* ---- 局部刷新（列0~63，行0~63覆盖4行） ---- */
        OLED_ShowFrameArea(0, 63, 0, 63);
        osDelay(20);
    }
}
