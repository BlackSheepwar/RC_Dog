/**
 * @file task_oled.c
 * @brief 实现OLED任务
 * @author 李嘉图
 * @date 2026-05-08
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
#include "main.h"
#include "oled.h"
#include "oled_extend.h"
#include "app_se.h"

/*==============================================================================
 * 任务函数
 *============================================================================*/
void StartOLEDTask(void *argument)
{
  OLED_Init();
  for(;;)
  {
    OLED_NewFrame();
    OLED_PrintDecimal(0, 0, APP_SE_GetCurrent(1), &afont16x8, OLED_COLOR_NORMAL);
    OLED_UpdateScreenByStep();
    osDelay(1);
  }
}