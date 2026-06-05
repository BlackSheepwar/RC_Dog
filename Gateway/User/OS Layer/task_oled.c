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
#include <stdint.h>
#include "oled.h"
#include "oled_extend.h"
#include "app_knob.h"

/*==============================================================================
 * 任务函数
 *============================================================================*/
void Task_OLED(void *argument)
{
  OLED_Init();

  BSP_ADC_Register(1, &hadc1, 3);
  APP_KNOB_Register(1, 1, 1, 125, -125);
  APP_KNOB_Register(2, 1, 2, 125, -125);
  int32_t knob_val = 0;
  for(;;)
  {
    OLED_NewFrame();
    if (APP_KNOB_GetValue(1, &knob_val) == 0)
    {
      OLED_PrintDecimal(0, 0, knob_val, &afont16x8, OLED_COLOR_NORMAL);
    }
    if (APP_KNOB_GetValue(2, &knob_val) == 0)
    {
      OLED_PrintDecimal(0, 18, knob_val, &afont16x8, OLED_COLOR_NORMAL);
    }
    OLED_ShowFrameArea(0, 31, 0, 31);
    osDelay(10);
  }
}