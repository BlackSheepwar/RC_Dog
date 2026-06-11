/**
 * @file task_can_f1.c
 * @brief CAN FIFO1 接收处理任务
 * @author 李嘉图
 * @date 2026-06-01
 *
 * @note 等待 CAN_F1_BS 二进制信号量，排空 CAN1 硬件 FIFO1。
 *       FIFO1 用于低优先级消息（遥测/状态帧）。
 *       滤波器在初始化时配置，匹配 ID=0x101,0x102。
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
#include "main.h"
#include "bsp_can.h"
#include "app_can_cmd.h"
#include "app_can_cfg.h"
#include "common.h"

/*==============================================================================
 * 外部变量声明
 *============================================================================*/
extern CAN_HandleTypeDef hcan1;

/*==============================================================================
 * CAN 端口路由表
 *============================================================================*/
typedef struct {
    uint8_t id;
    void    (*handler)(CAN_RxHeaderTypeDef *header, uint8_t *data);
} can_fifo1_routing_entry_t;

static const can_fifo1_routing_entry_t CAN_FIFO1_ROUTING_TABLE[] = {
    { .id = 1, .handler = APP_CAN1_F1_Cmd },
    // { .id = 2, .handler = APP_CAN2_F1_Cmd },  // CAN2 暂未启用
};

/*==============================================================================
 * 任务函数
 *============================================================================*/
void Task_CAN_F1(void *argument)
{
    for (;;)
    {
        BSP_CAN_Packet_t rx_pkt;
        osMessageQueueGet(CAN_F1_QHandle, &rx_pkt, NULL, osWaitForever);

        /* 查表分发到对应 CAN 端口的处理函数 */
        for (uint8_t i = 0; i < ARRAY_SIZE(CAN_FIFO1_ROUTING_TABLE); i++)
        {
            if (CAN_FIFO1_ROUTING_TABLE[i].id == rx_pkt.id)
            {
                CAN_FIFO1_ROUTING_TABLE[i].handler(&rx_pkt.header, rx_pkt.data);
                break;
            }
        }
    }
}
