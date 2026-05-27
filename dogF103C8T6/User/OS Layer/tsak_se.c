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
  APP_SE_Add(1, &htim2, TIM_CHANNEL_3, 0, 0, 1);
  //APP_SE_Add(1, &htim3, TIM_CHANNEL_3, 10, 0, 1);
  //APP_SE_Add(2, &htim3, TIM_CHANNEL_2, 20, 0, 1);
  //APP_SE_Add(3, &htim3, TIM_CHANNEL_1, 10, 0, 1);

  for(;;)
  {
    APP_SE_Scheduler();
    osDelay(10);
  }
}