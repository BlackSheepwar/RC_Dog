/**
 * @file app_can_cfg.h
 * @brief CAN 端口与滤波器配置
 * @author 李嘉图
 * @date 2026-06-05
 *
 * @note 集中管理 CAN 实例的端口ID、滤波器参数，TF Task 层只需引用配置常量。
 */

#ifndef __APP_CAN_CFG_H__
#define __APP_CAN_CFG_H__

/*==============================================================================
 * 头文件包含
 *============================================================================*/
#include "main.h"
#include "bsp_can.h"
#include "protocol.h"

/*==============================================================================
 * CAN 实例配置
 *============================================================================*/
#define CAN1_INSTANCE_ID    1   // CAN1 端口ID
// #define CAN2_INSTANCE_ID   2   // CAN2 暂未启用

/*==============================================================================
 * CAN1 FIFO0 滤波器配置（高优先级：舵机控制帧 0x201~0x204）
 *
 * 16位通道组模式：4个独立ID分别匹配四条腿
 *============================================================================*/
#define CAN1_FIFO0_FILTER_BANK   0
#define CAN1_FIFO0_FILTER_FIFO   BSP_CAN_FIFO0

static const BSP_CAN_FilterConfig_t CAN1_F0_FILTER_CFG = {
    .filter_bank = CAN1_FIFO0_FILTER_BANK,
    .fifo        = CAN1_FIFO0_FILTER_FIFO,
    .mode        = BSP_CAN_FILTER_MODE_LIST,
    .scale       = BSP_CAN_FILTER_SCALE_16,
    .id1 = BSP_CAN_STD16(CANID_LEG1, 0, 0),
    .id2 = BSP_CAN_STD16(CANID_LEG2, 0, 0),
    .id3 = BSP_CAN_STD16(CANID_LEG3, 0, 0),
    .id4 = BSP_CAN_STD16(CANID_LEG4, 0, 0),
};

/*==============================================================================
 * CAN1 FIFO1 滤波器配置（低优先级：备用）
 *============================================================================*/
#define CAN1_FIFO1_FILTER_BANK   1
#define CAN1_FIFO1_FILTER_FIFO   BSP_CAN_FIFO1

// FIFO1 滤波器配置（暂用默认值，后续根据实际协议补充）
static const BSP_CAN_FilterConfig_t CAN1_F1_FILTER_CFG = {
    .filter_bank = CAN1_FIFO1_FILTER_BANK,
    .fifo        = CAN1_FIFO1_FILTER_FIFO,
    .mode        = BSP_CAN_FILTER_MODE_MASK,
    .scale       = BSP_CAN_FILTER_SCALE_32,
    .id1 = 0,
    .id2 = 0,
    .id3 = 0,
    .id4 = 0,
};

#endif /* __APP_CAN_CFG_H__ */
