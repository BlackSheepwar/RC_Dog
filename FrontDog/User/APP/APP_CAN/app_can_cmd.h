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
