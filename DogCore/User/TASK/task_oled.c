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

/*==============================================================================
 * 任务函数
 *============================================================================*/
// void Task_OLED(void *argument)
// {
//     OLED_Init();

//     uint32_t frame_cnt  = 0;         // 帧计数器
//     uint32_t fps        = 0;         // 计算结果
//     uint32_t last_tick  = HAL_GetTick();
//     uint8_t  cursor     = 0;         // 移动光标(0~127)

//     for (;;)
//     {
//         frame_cnt++;

//         /* ========== 每秒统计一次 FPS ========== */
//         uint32_t now = HAL_GetTick();
//         uint32_t elapsed = now - last_tick;
//         if (elapsed >= 1000)
//         {
//             fps = frame_cnt * 1000 / elapsed;
//             frame_cnt = 0;
//             last_tick = now;
//         }

//         /* ========== 绘制测试图 ========== */
//         OLED_NewFrame();

//         /* 第一行: FPS 数值 */
//         OLED_PrintASCIIString(0, 0, "FPS:", &afont16x8, OLED_COLOR_NORMAL);
//         OLED_PrintDecimal(56, 0, fps, &afont16x8, OLED_COLOR_NORMAL);

//         /* 第二行: 帧计数 */
//         OLED_PrintASCIIString(0, 16, "FRM:", &afont16x8, OLED_COLOR_NORMAL);
//         OLED_PrintDecimal(56, 16, frame_cnt, &afont16x8, OLED_COLOR_NORMAL);

//         /* 第三行: 水平进度条 (128px 满) */
//         uint8_t bar_len = (frame_cnt * 128) / (elapsed > 0 ? (fps > 0 ? fps : 60) : 60);
//         if (bar_len > 128) bar_len = 128;
//         for (uint8_t i = 0; i < bar_len; i++)
//             OLED_SetPixel(i, 40, OLED_COLOR_NORMAL);

//         /* 第四行: 移动光标 — 每帧右移1px，到边回卷 */
//         OLED_SetPixel(cursor, 56, OLED_COLOR_NORMAL);
//         cursor = (cursor + 1) & 0x7F;

//         /* 额外画两条竖线分隔视觉区域 */
//         OLED_SetPixel(50, 32, OLED_COLOR_NORMAL);
//         OLED_SetPixel(50, 33, OLED_COLOR_NORMAL);
//         OLED_SetPixel(50, 34, OLED_COLOR_NORMAL);

//         OLED_ShowFrame();
//         osDelay(1);
//     }
// }