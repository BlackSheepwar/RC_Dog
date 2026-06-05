/**
 * @file task_se.c
 * @brief 实现舵机控制处理任务
 * @author 李嘉图
 * @date 2026-5-12
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
#include "main.h"
#include "app_se.h"
#include "common.h"
#include <stdint.h>

/*==============================================================================
 * 任务函数
 *============================================================================*/
void Task_SE(void *argument)
{
#if HW_VERSION == HW_MCU_FRONT
  /*==============================================================================
   * 前腿（MCU1）：舵机ID 1-6，左腿=1/2/3，右腿=4/5/6
   *============================================================================*/
  APP_SE_Init();
  // APB2: 168MHz (TIM9/10/11)
  BSP_SetFreq(&htim9,  TIM_APB2_Hz, 330);
  BSP_SetFreq(&htim10, TIM_APB2_Hz, 330);
  // BSP_SetFreq(&htim11, TIM_APB2_Hz, 330);
  // APB1: 84MHz (TIM12/13/14)
  BSP_SetFreq(&htim12, TIM_APB1_Hz, 330);
  BSP_SetFreq(&htim13, TIM_APB1_Hz, 330);
  BSP_SetFreq(&htim14, TIM_APB1_Hz, 330);
  APP_SE_Add(1, &htim12, TIM_CHANNEL_2, 20, 0, 3, 1);
  APP_SE_Add(2, &htim12, TIM_CHANNEL_1, 16, 0, 3, 1);
  APP_SE_Add(3, &htim10, TIM_CHANNEL_1, 15, 0, 3, 1);
  APP_SE_Add(4, &htim14, TIM_CHANNEL_1, 8, 0, 3, 0);
  APP_SE_Add(5, &htim13, TIM_CHANNEL_1, 21, 0, 3, 0);
  APP_SE_Add(6, &htim9,  TIM_CHANNEL_2, 12, 0, 3, 0);
#else
  /*==============================================================================
   * 后腿（MCU2）：舵机ID 7-12，左腿=7/8/9，右腿=10/11/12
   *============================================================================*/
  APP_SE_Init();
  // APB2: 168MHz (TIM9/10/11)
  BSP_SetFreq(&htim9,  TIM_APB2_Hz, 330);
  BSP_SetFreq(&htim10, TIM_APB2_Hz, 330);
  // BSP_SetFreq(&htim11, TIM_APB2_Hz, 330);
  // APB1: 84MHz (TIM12/13/14)
  BSP_SetFreq(&htim12, TIM_APB1_Hz, 330);
  BSP_SetFreq(&htim13, TIM_APB1_Hz, 330);
  BSP_SetFreq(&htim14, TIM_APB1_Hz, 330);
  APP_SE_Add(7,  &htim12, TIM_CHANNEL_2, 20, 0, 3, 1);
  APP_SE_Add(8,  &htim12, TIM_CHANNEL_1, -12, 0, 3, 1);
  APP_SE_Add(9,  &htim10, TIM_CHANNEL_1, 14, 0, 3, 1);
  APP_SE_Add(10, &htim14, TIM_CHANNEL_1, 9, 0, 3, 0);
  APP_SE_Add(11, &htim13, TIM_CHANNEL_1, 0, 0, 3, 0);
  APP_SE_Add(12, &htim9, TIM_CHANNEL_2, 10, 0, 3, 0);
#endif

  for(;;)
  {
    APP_SE_Scheduler();
    osDelay(10);
  }
}