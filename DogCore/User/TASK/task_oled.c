/**
 * @file task_oled.c
 * @brief OLED任务 — 帧率测试
 * @author 李嘉图
 * @date 2026-06-23
 *
 * @note 测量 OLED_ShowFrame 实际刷新速率，显示 FPS。
 *       含移动光标视觉反馈，全程无延迟跑满速。
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include <stdint.h>
#include <string.h>
#include "main.h"
// 功能包含
#include "oled.h"
#include "oled_extend.h"
#include "oled_ui.h"

/*==============================================================================
 * 任务函数
 *============================================================================*/
void Task_OLED(void *argument)
{
    OLED_Init();
    OLED_UI_FpsInit();

    for (;;)
    {
        OLED_UI_FpsTick();
        osDelay(1);
    }
}