/**
 * @file task_can_rxf0.c
 * @brief CAN FIFO0 接收处理任务
 * @author 李嘉图
 * @date 2026-06-05
 *
 * @note 等待 CAN_F0_QHandle 消息队列，接收高优先级 CAN 帧（控制/急停）。
 *       滤波器配置已提取到 app_can_cfg.h，Task 层只做初始化→等待→分发。
 *       新增 CAN 端口只需在 routing 表中添加一行。
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include <stdint.h>
#include "main.h"
#include "common.h"
// 功能包含
#include "app_can.h"
#include "app_can_cmd.h"
#include "app_can_cfg.h"

/*==============================================================================
 * CAN 端口路由表
 *============================================================================*/
static const can_fifo_routing_entry_t CAN_FIFO0_ROUTING_TABLE[] = {
    { .id = 1, .handler = APP_CAN1_F0_Cmd },
    { .id = 2, .handler = APP_CAN2_F0_Cmd },
};

/*==============================================================================
 * 任务函数
 *============================================================================*/
void Task_CAN_RXF0(void *argument)
{
    /* ---------- 1. 初始化APP层（复位发送缓冲） ---------- */
    APP_CAN_Init();

    /* ---------- 2. 配置滤波器 ---------- */
    APP_CAN_FilterConfig(1, &CAN1_F0_FILTER_CFG);

    /* ---------- 3. 启动CAN ---------- */
    APP_CAN_Start(1);

    for (;;)
    {
        BSP_CAN_Packet_t rx_pkt;
        osMessageQueueGet(CAN_RXF0_QHandle, &rx_pkt, NULL, osWaitForever);

        /* 查表分发到对应 CAN 端口的处理函数 */
        for (uint8_t i = 0; i < ARRAY_SIZE(CAN_FIFO0_ROUTING_TABLE); i++)
        {
            if (CAN_FIFO0_ROUTING_TABLE[i].id == rx_pkt.id)
            {
                CAN_FIFO0_ROUTING_TABLE[i].handler(&rx_pkt.header, rx_pkt.data);
                break;
            }
        }
    }
}
