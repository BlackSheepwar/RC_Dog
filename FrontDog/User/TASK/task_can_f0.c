/**
 * @file task_can_f0.c
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
#include "main.h"
#include "app_can.h"
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
} can_fifo0_routing_entry_t;

static const can_fifo0_routing_entry_t CAN_FIFO0_ROUTING_TABLE[] = {
    { .id = 1, .handler = APP_CAN1_F0_Cmd },
    // { .id = 2, .handler = APP_CAN2_F0_Cmd },  // CAN2 暂未启用
};

/*==============================================================================
 * 任务函数
 *============================================================================*/
void Task_CAN_F0(void *argument)
{
    /* ---------- 1. 初始化APP层 ---------- */
    APP_CAN_Init();

    /* ---------- 2. 注册CAN端口 ---------- */
    APP_CAN_Register(CAN1_INSTANCE_ID, &hcan1);

    /* ---------- 3. 配置滤波器 ---------- */
    APP_CAN_FilterConfig(CAN1_INSTANCE_ID, &CAN1_F0_FILTER_CFG);

    /* ---------- 4. 启动CAN ---------- */
    APP_CAN_Start(CAN1_INSTANCE_ID);

    for (;;)
    {
        BSP_CAN_Packet_t rx_pkt;
        osMessageQueueGet(CAN_F0_QHandle, &rx_pkt, NULL, osWaitForever);

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
