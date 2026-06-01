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
  // APB2: 168MHz (TIM9/10/11)
  BSP_SetFreq(&htim9, TIM_APB2_Hz, 330);
  BSP_SetFreq(&htim10, TIM_APB2_Hz, 330);
  BSP_SetFreq(&htim11, TIM_APB2_Hz, 330);
  // APB1: 84MHz (TIM12/13/14)
  BSP_SetFreq(&htim12, TIM_APB1_Hz, 330);
  // BSP_SetFreq(&htim13, TIM_APB1_Hz, 330);
  // BSP_SetFreq(&htim14, TIM_APB1_Hz, 330);
  APP_SE_Add(1, &htim12, TIM_CHANNEL_2, 0, 0, 1);
  APP_SE_Add(2, &htim12, TIM_CHANNEL_1, 0, 0, 3);
  APP_SE_Add(3, &htim10, TIM_CHANNEL_1, 0, 0, 3);
  APP_SE_Add(4, &htim11, TIM_CHANNEL_1, 0, 0, 3);
  APP_SE_Add(5, &htim9, TIM_CHANNEL_1, 0, 0, 3);
  APP_SE_Add(6, &htim9, TIM_CHANNEL_2, 0, 0, 3);
  // APP_SE_Add(7, &htim13, TIM_CHANNEL_1, 0, 0, 3);
  // APP_SE_Add(8, &htim14, TIM_CHANNEL_1, 0, 0, 3);
  for(;;)
  {
    APP_SE_Scheduler();
    osDelay(10);
  }
}