/**
 * @file task_can_rxf0.c
 * @brief CAN FIFO0 接收线程：取一帧，按端口路由到命令表
 * @author Ricardo
 * @date 2026-11-01
 *
 * @note 总线已在 freertos.c 的 RTOS_QUEUES 段里（调度前、队列建好之后）
 *       初始化并打开，本线程常驻阻塞在队列上。
 *       "哪一帧归哪张表"由 APP_CAN_ROUTE_TABLE 决定，
 *       线程体里没有 switch，也没有任何具体 ID。
 *
 *       队列由 CubeMX 创建（CAN_RXF0_Q），BSP 的接收中断负责投递。
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include <stdint.h>
#include "common.h"             /* ARRAY_SIZE */
#include "cmsis_os2.h"
#include "main.h"               /* CAN_RXF0_QHandle */
// 功能包含
#include "app_can.h"
#include "app_can_cmd.h"
#include "app_can_cfg.h"

/*==============================================================================
 * 线程函数
 *============================================================================*/
/**
 * @brief CAN FIFO0 接收线程（入口名由 CubeMX .ioc 决定，不能改）
 * @param argument 线程参数（未使用）
 * @retval 无
 */
void Task_CAN_RXF0(void *argument)
{
    (void)argument;

    for (;;) {
        bsp_can_frame_t frame;

        if (osMessageQueueGet(CAN_RXF0_QHandle, &frame, NULL, osWaitForever) != osOK) {
            continue;
        }

        /* ---------- 逐帧交给命令表 ---------- */
        for (uint8_t i = 0u; i < ARRAY_SIZE(APP_CAN_ROUTE_TABLE); i++) {
            if ((APP_CAN_ROUTE_TABLE[i].port == APP_CAN_PORT_1)
                && (APP_CAN_ROUTE_TABLE[i].fifo == (uint8_t)BSP_CAN_FIFO0)) {
                APP_CAN_ROUTE_TABLE[i].handler(&frame);
                break;
            }
        }
    }
}
