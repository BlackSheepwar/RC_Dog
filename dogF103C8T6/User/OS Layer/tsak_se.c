/**
 * @file tsak_se.c
 * @brief 实现舵机控制处理任务
 * @author 李嘉图
 * @date 2026-5-12
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
#include "main.h"
#include "app_se.h"
#include "app_knob.h"
#include <stdint.h>

/*==============================================================================
 * 任务函数
 *============================================================================*/
/**
 * @brief SETask 任务入口函数
 * @param argument 任务参数（未使用）
 */
void StartSETask(void *argument)
{
  APP_SE_Init();
  APP_SE_Add(1, &htim2, TIM_CHANNEL_3, 0, 0, 3);

  BSP_ADC_Register(1, &hadc1, 2);
  APP_KNOB_Register(1, 1, 1, 125, -125);
  int32_t knob_val = 0;

  for(;;)
  {
    if (APP_KNOB_GetValue(1, &knob_val) == 0)
    {
      APP_SE_SetTarget(1, (int16_t)knob_val);
    }
    APP_SE_Scheduler();
    osDelay(30);
  }
}