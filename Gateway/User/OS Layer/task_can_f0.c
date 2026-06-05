/**
 * @file task_can_f0.c
 * @brief CAN FIFO0 接收处理任务
 * @author 李嘉图
 * @date 2026-06-01
 *
 * @note 等待 CAN_F0_BS 二进制信号量，从硬件 FIFO0 读取帧并分发。
 *       FIFO0 用于高优先级消息（控制/急停指令）。
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
#include "main.h"
#include "app_can.h"
#include "app_can_cmd.h"

/*==============================================================================
 * 外部变量声明
 *============================================================================*/
extern CAN_HandleTypeDef hcan;

/*==============================================================================
 * 任务函数
 *============================================================================*/
void Task_CAN_F0(void *argument)
{
    /* ---------- 1. 初始化APP层 ---------- */
    APP_CAN_Init();

    /* ---------- 2. 注册CAN端口 ---------- */
    APP_CAN_Register(1, &hcan);

    /* ---------- 3. 配置滤波器 ---------- */
    /*     示例: 规则组+标准, ID=0x100, 掩码0x7FF(精确匹配) */
    /*     示例: 规则组+标准, ID=0x200, 掩码0x7FF(精确匹配) */
    BSP_CAN_FilterConfig_t cfg = {
        .filter_bank = 0,
        .fifo        = BSP_CAN_FIFO0,
        .mode        = BSP_CAN_FILTER_MODE_LIST,
        .scale       = BSP_CAN_FILTER_SCALE_16,
        .id1 = BSP_CAN_STD16(0x200, 0, 0),
        .id2 = BSP_CAN_STD16(0x300, 0, 0),
        .id3 = BSP_CAN_STD16(0x400, 0, 0),
        .id4 = BSP_CAN_STD16(0x500, 0, 0),
    };
    APP_CAN_FilterConfig(1, &cfg);

    /* ---------- 4. 启动CAN ---------- */
    APP_CAN_Start(1);

    for (;;)
    {
        BSP_CAN_Packet_t rx_pkt;
        osMessageQueueGet(CAN_F0_QHandle, &rx_pkt, NULL, osWaitForever);
        if (rx_pkt.id == 1) APP_CAN1_F0_Cmd(&rx_pkt.header, rx_pkt.data);
    }
}
