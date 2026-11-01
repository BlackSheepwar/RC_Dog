/**
 * @file app_motor_debug.c
 * @brief 电机 PID 调参数据发送实现（调试用）
 * @author 李嘉图
 * @date 2026-07-07
 *
 * @note 从 APP_Motor 分离出来的调参辅助功能。
 *       不直接访问 motor_pool，通过 APP_Motor_GetPIDState 获取数据。
 *       可随调试阶段结束而整体移除，不影响电机控制主逻辑。
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "app_motor_debug.h"
// 功能包含
#include "app_motor.h"
#include "common_utils.h"
#include "bsp_uart.h"

/*==============================================================================
 * 调参数据发送
 *============================================================================*/
/**
 * @brief 发送所有电机的 PID 调参数据（串口绘图仪兼容格式）
 * @param uart_id 串口编号（与 BSP 层一致）
 * @note  格式: "PID: T1,A1,O1,T2,A2,O2,T3,A3,O3,T4,A4,O4\n"
 *        共 12 通道: 每电机 3 值（目标/实际/输出），逗号分隔，\n 结尾。
 *        使用阻塞式轮询发送，不占用 DMA，不与 TX 任务冲突。
 */
void APP_Motor_SendPIDData(uint8_t uart_id)
{
    static char txbuf[512];
    uint16_t pos = 0;
    char fbuf[20];

    /* 标签前缀（不包含 "image"，兼容串口绘图仪） */
    pos += sprintf(&txbuf[pos], "PID:");

    for (uint8_t i = 0; i < MOTOR_NUM; i++)
    {
        float target, actual, out;

        /* 通过公开接口获取 PID 状态，不直接访问 motor_pool */
        if (APP_Motor_GetPIDState((uint8_t)(i + 1), &target, &actual, &out) == 0)
            continue;

        /* 目标值 */
        float_to_str(target, fbuf, 3);
        uint16_t len = strlen(fbuf);
        memcpy(&txbuf[pos], fbuf, len);
        pos += len;
        txbuf[pos++] = ',';

        /* 实际值 */
        float_to_str(actual, fbuf, 3);
        len = strlen(fbuf);
        memcpy(&txbuf[pos], fbuf, len);
        pos += len;
        txbuf[pos++] = ',';

        /* 输出值 */
        float_to_str(out, fbuf, 3);
        len = strlen(fbuf);
        memcpy(&txbuf[pos], fbuf, len);
        pos += len;

        /* 最后一个电机不加逗号 */
        if (i < MOTOR_NUM - 1)
            txbuf[pos++] = ',';
    }

    /* 换行结尾 */
    txbuf[pos++] = '\n';

    /* 阻塞式轮询发送，不占用 DMA，不与 TX 任务冲突 */
    BSP_UART_Send(uart_id, (uint8_t*)txbuf, pos);
}
