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

/*==============================================================================
 * CAN 端口路由表
 *============================================================================*/
typedef struct {
    uint8_t id;
    void    (*handler)(FDCAN_RxHeaderTypeDef *header, uint8_t *data);
} can_fifo_routing_entry_t;

/*==============================================================================
 * CAN1 FIFO0 滤波器配置（高优先级：舵机控制帧 0x201~0x204）
 *
 * 使用FDCAN RANGE模式，一个滤波器覆盖0x201~0x204所有ID
 *============================================================================*/
#define CAN1_FIFO0_FILTER_BANK   0
#define CAN1_FIFO0_FILTER_FIFO   BSP_CAN_FIFO0

static const BSP_CAN_FilterConfig_t CAN1_F0_FILTER_CFG = {
    .filter_bank = CAN1_FIFO0_FILTER_BANK,
    .fifo        = CAN1_FIFO0_FILTER_FIFO,
    .mode        = BSP_CAN_FILTER_MODE_RANGE,
    .scale       = BSP_CAN_FILTER_SCALE_16,
    .id1 = 0x201,
    .id2 = 0x204,
    .id3 = 0,
    .id4 = 0,
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
