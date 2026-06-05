/**
 * @file app_can_cmd.h
 * @brief CAN命令分发（CAN1 / CAN2 独立接口）
 * @author 李嘉图
 * @date 2026-06-04
 *
 * @note CAN1 和 CAN2 有各自的分发函数，分别在 app_can_cmd_1.c 和 app_can_cmd_2.c 中实现。
 *       FIFO0 和 FIFO1 独立分发，内部按 标准/扩展 × 数据/遥控 四种类型路由。
 */

#ifndef __APP_CAN_CMD_H__
#define __APP_CAN_CMD_H__

/*==============================================================================
 * 头文件包含
 *============================================================================*/
#include "can.h"

/*==============================================================================
 * CAN RX 配置表类型（CAN1/CAN2 共用）
 *============================================================================*/
typedef void (*can_frame_handler_t)(CAN_RxHeaderTypeDef *header, uint8_t *data);

typedef struct {
    uint32_t            can_id;
    uint8_t             ide;        // CAN_ID_STD / CAN_ID_EXT
    uint8_t             rtr;        // CAN_RTR_DATA / CAN_RTR_REMOTE
    can_frame_handler_t handler;
} can_rx_entry_t;

/**
 * @brief 通用配置表分发函数（inline，跨文件共用）
 * @param table   配置表
 * @param count   表项数
 * @param header  CAN帧头
 * @param data    帧数据
 */
static inline void CAN_DispatchByTable(const can_rx_entry_t *table, uint8_t count,
                                        CAN_RxHeaderTypeDef *header, uint8_t *data)
{
    uint32_t id = (header->IDE == CAN_ID_STD) ? header->StdId : header->ExtId;

    for (uint8_t i = 0; i < count; i++)
    {
        if (header->IDE == table[i].ide &&
            header->RTR == table[i].rtr &&
            id == table[i].can_id)
        {
            table[i].handler(header, data);
            return;
        }
    }
}

/*==============================================================================
 * CAN1 命令分发
 *============================================================================*/
/**
 * @brief CAN1 FIFO0 命令分发（高优先级消息：控制/急停）
 * @param header  CAN帧头（含ID/IDE/RTR/DLC）
 * @param data    帧数据（遥控帧时无有效数据）
 */
void APP_CAN1_F0_Cmd(CAN_RxHeaderTypeDef *header, uint8_t *data);

/**
 * @brief CAN1 FIFO1 命令分发（低优先级消息：遥测/状态）
 * @param header  CAN帧头
 * @param data    帧数据
 */
void APP_CAN1_F1_Cmd(CAN_RxHeaderTypeDef *header, uint8_t *data);

/*==============================================================================
 * CAN2 命令分发
 *============================================================================*/
/**
 * @brief CAN2 FIFO0 命令分发（高优先级消息：控制/急停）
 * @param header  CAN帧头
 * @param data    帧数据
 */
void APP_CAN2_F0_Cmd(CAN_RxHeaderTypeDef *header, uint8_t *data);

/**
 * @brief CAN2 FIFO1 命令分发（低优先级消息：遥测/状态）
 * @param header  CAN帧头
 * @param data    帧数据
 */
void APP_CAN2_F1_Cmd(CAN_RxHeaderTypeDef *header, uint8_t *data);

#endif
