/**
 * @file oled_ui.c
 * @brief OLED 显示 UI 组件实现
 * @author
 * @date 2026-06-25
 *
 * @note 提供两个 UI 组件：
 *       1. FPS 帧率显示（替代 task_oled.c 中的手写循环）
 *       2. 串口回环压力测试结果显示
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include "main.h"                  /* HAL_GetTick */
// 功能包含
#include "oled_extend.h"
#include "oled_ui.h"
#include "app_uart_cmd.h"

/*==============================================================================
 * FPS 上下文
 *============================================================================*/
static struct {
    uint32_t frame_cnt;         /**< 当前统计周期内的帧数 */
    uint32_t fps;               /**< 上一秒计算的 FPS 值 */
    uint32_t last_tick;         /**< 上次统计的时间戳 */
    uint8_t  cursor;            /**< 移动光标位置（0~127） */
} fps_ctx;

/*==============================================================================
 * FPS 帧率显示
 *============================================================================*/
void OLED_UI_FpsInit(void)
{
    fps_ctx.frame_cnt = 0;
    fps_ctx.fps       = 0;
    fps_ctx.last_tick = HAL_GetTick();
    fps_ctx.cursor    = 0;
}

void OLED_UI_FpsTick(void)
{
    fps_ctx.frame_cnt++;

    /* 每秒计算一次 FPS */
    uint32_t now     = HAL_GetTick();
    uint32_t elapsed = now - fps_ctx.last_tick;

    if (elapsed >= 1000)
    {
        fps_ctx.fps       = fps_ctx.frame_cnt * 1000 / elapsed;
        fps_ctx.frame_cnt = 0;
        fps_ctx.last_tick = now;
    }

    OLED_NewFrame();

    /* === 第 0 行：FPS 数值 === */
    OLED_PrintASCIIString(0, 0, "FPS:", &afont16x8, OLED_COLOR_NORMAL);
    OLED_PrintDecimal(56, 0, fps_ctx.fps, &afont16x8, OLED_COLOR_NORMAL);

    /* === 第 1 行：帧计数 === */
    OLED_PrintASCIIString(0, 16, "FRM:", &afont16x8, OLED_COLOR_NORMAL);
    OLED_PrintDecimal(56, 16, fps_ctx.frame_cnt, &afont16x8, OLED_COLOR_NORMAL);

    /* === 第 2 行：水平进度条（128px 满） === */
    uint32_t fps_val = (fps_ctx.fps > 0) ? fps_ctx.fps : 60;
    uint8_t bar_len  = fps_ctx.frame_cnt * 128 / fps_val;
    if (bar_len > 128) bar_len = 128;
    for (uint8_t i = 0; i < bar_len; i++)
        OLED_SetPixel(i, 40, OLED_COLOR_NORMAL);

    /* === 第 3 行：移动光标（每帧右移 1px，到边回卷） === */
    OLED_SetPixel(fps_ctx.cursor, 56, OLED_COLOR_NORMAL);
    fps_ctx.cursor = (fps_ctx.cursor + 1) & 0x7F;

    OLED_ShowFrame();
}
