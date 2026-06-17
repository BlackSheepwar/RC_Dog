/**
 * @file app_cursor.c
 * @brief 多舵机光标选择与电位器锁定控制实现
 * @author 李嘉图
 * @date 2026-06-03
 *
 * @note 纯数据管理模块，无硬件依赖。
 *       全局变量 g_cursor 供其他模块直接读取。
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
#include "app_cursor.h"

/*==============================================================================
 * 舵机ID映射表
 *============================================================================*/
static const uint8_t servo_ids[CURSOR_SERVO_NUM] = CURSOR_SERVO_IDS;

/*==============================================================================
 * 全局变量
 *============================================================================*/
APP_Cursor_t g_cursor;

/*==============================================================================
 * 功能函数
 *============================================================================*/
/**
 * @brief 初始化光标
 * @note  默认光标指向第 1 个舵机，电位器未锁定
 */
void APP_Cursor_Init(void)
{
    g_cursor.index      = 0;
    g_cursor.current_id = servo_ids[0];
    g_cursor.lock       = 0;
}

/**
 * @brief 光标向前切换（1→2→3→1…）
 */
void APP_Cursor_Next(void)
{
    g_cursor.index++;
    if (g_cursor.index >= CURSOR_SERVO_NUM)
        g_cursor.index = 0;

    g_cursor.current_id = servo_ids[g_cursor.index];
}

/**
 * @brief 光标向后切换（1→3→2→1…）
 */
void APP_Cursor_Prev(void)
{
    if (g_cursor.index == 0)
        g_cursor.index = CURSOR_SERVO_NUM - 1;
    else
        g_cursor.index--;

    g_cursor.current_id = servo_ids[g_cursor.index];
}

/**
 * @brief 反转电位器锁定状态（锁定↔解锁）
 */
void APP_Cursor_ToggleLock(void)
{
    g_cursor.lock = !g_cursor.lock;
}
