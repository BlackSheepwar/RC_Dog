/**
 * @file app_debug.c
 * @brief 调试输出实现：速度环数据 → 串口绘图仪格式
 * @author Ricardo
 * @date 2026-11-01
 *
 * @note 不碰电机槽位内部，只走 Svc_Motor_Speed_Get_State，
 *       所以删掉本文件不影响任何控制逻辑。
 *
 *       缓冲区是 static 的：512 字节放栈上会吃掉一个 128 字线程栈的大半，
 *       而本函数只会在同一个调试线程里调用（单写者，无需保护）。
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "app_debug.h"
// 功能包含
#include "svc_motor_speed.h"
#include "common_utils.h"       /* float_to_str */
#include "bsp_uart.h"           /* BSP_UART_Find / BSP_UART_Send */

/*==============================================================================
 * 内部变量
 *============================================================================*/
/** @brief 发送拼装缓冲（只在本线程使用） */
static char s_tx_buf[512];

/*==============================================================================
 * 内部函数
 *============================================================================*/
/**
 * @brief 把一个浮点数写成字符串并追加到缓冲
 * @param buf   目标缓冲
 * @param pos   当前写入位置（函数内推进）
 * @param value 待写数值
 * @retval 无
 */
static void APP_Debug_Append_Float(char *buf, uint16_t *pos, float value)
{
    char     tmp[20];
    uint16_t len;

    float_to_str(value, tmp, 3);
    len = (uint16_t)strlen(tmp);

    memcpy(&buf[*pos], tmp, len);
    *pos = (uint16_t)(*pos + len);
}

/*==============================================================================
 * 接口实现
 *============================================================================*/
/**
 * @brief 把所有电机的速度环数据打包成一行并发出去
 * @param port_id 串口逻辑编号
 * @retval 无
 */
void APP_Debug_Send_Motor_PID(uint8_t port_id)
{
    const bsp_uart_t *port  = BSP_UART_Find(port_id);
    uint8_t           count = Svc_Motor_Speed_Count();
    uint16_t          pos   = 0u;

    if ((port == NULL) || (count == 0u)) {
        return;
    }

    /* ---------- 1. 标签前缀 ---------- */
    pos = (uint16_t)sprintf(s_tx_buf, "PID:");

    /* ---------- 2. 逐路拼装 ---------- */
    for (uint8_t i = 0u; i < count; i++) {
        const svc_motor_speed_t *speed = Svc_Motor_Speed_Get(i);
        float                    target = 0.0f;
        float                    actual = 0.0f;
        float                    out    = 0.0f;

        if (Svc_Motor_Speed_Get_State(speed, &target, &actual, &out) != 0) {
            continue;
        }

        APP_Debug_Append_Float(s_tx_buf, &pos, target);
        s_tx_buf[pos++] = ',';
        APP_Debug_Append_Float(s_tx_buf, &pos, actual);
        s_tx_buf[pos++] = ',';
        APP_Debug_Append_Float(s_tx_buf, &pos, out);

        if (i < (count - 1u)) {
            s_tx_buf[pos++] = ',';
        }
    }

    /* ---------- 3. 换行并发送 ---------- */
    s_tx_buf[pos++] = '\n';

    (void)BSP_UART_Send(port, (const uint8_t *)s_tx_buf, pos, BSP_UART_SEND_TIMEOUT);
}
