/**
 * @file task_can_rxf1.c
 * @brief CAN FIFO1 接收线程：取一帧，按端口路由到命令表
 * @author Ricardo
 * @date 2026-11-01
 *
 * @note 与 task_can_rxf0.c 结构相同，只是队列与 FIFO 不同。
 *       FIFO1 留给低优先级（遥测/状态）帧，分开两个线程是为了让
 *       高优先级帧的处理不被慢业务挡住。
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include <stdint.h>
#include "common.h"             /* ARRAY_SIZE */
#include "cmsis_os2.h"
#include "main.h"               /* CAN_RXF1_QHandle */
// 功能包含
#include "app_can.h"
#include "app_can_cmd.h"
#include "app_can_cfg.h"

/*==============================================================================
 * 线程函数
 *============================================================================*/
/**
 * @brief CAN FIFO1 接收线程（入口名由 CubeMX .ioc 决定，不能改）
 * @param argument 线程参数（未使用）
 * @retval 无
 */
void Task_CAN_RXF1(void *argument)
{
    (void)argument;

    for (;;) {
        bsp_can_frame_t frame;

        if (osMessageQueueGet(CAN_RXF1_QHandle, &frame, NULL, osWaitForever) != osOK) {
            continue;
        }

        for (uint8_t i = 0u; i < ARRAY_SIZE(APP_CAN_ROUTE_TABLE); i++) {
            if ((APP_CAN_ROUTE_TABLE[i].port == APP_CAN_PORT_1)
                && (APP_CAN_ROUTE_TABLE[i].fifo == (uint8_t)BSP_CAN_FIFO1)) {
                APP_CAN_ROUTE_TABLE[i].handler(&frame);
                break;
            }
        }
    }
}
