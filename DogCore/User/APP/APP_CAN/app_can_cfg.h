/**
 * @file app_can_cfg.h
 * @brief CAN 端口与滤波器配置表（编译期常量）
 * @author Ricardo
 * @date 2026-11-01
 *
 * @note 只写"要收哪些 ID"，不写 HAL 类型：滤波器用 bsp_can_filter_t 描述，
 *       所以本文件可以放心被 APP 头文件包含，不会把 fdcan.h 拖进来。
 */

#ifndef __APP_CAN_Cfg_H__
#define __APP_CAN_Cfg_H__

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include <stdint.h>
#include "bsp_can.h"            /* bsp_can_filter_t / bsp_can_frame_t */
#include "app_can_cmd.h"        /* can_rx_handler_t */

/*==============================================================================
 * 类型定义
 *============================================================================*/
/**
 * @brief 端口路由项：某个端口的某个 FIFO 交给哪个命令表
 * @note 表元素是值类型，可以放进 const 数组。
 */
typedef struct {
    uint8_t           port;      /* 端口编号，1 起 */
    uint8_t           fifo;      /* 取 bsp_can_fifo_t */
    can_rx_handler_t  handler;   /* 该 FIFO 的分发入口 */
} app_can_route_t;

/*==============================================================================
 * 滤波器配置
 *============================================================================*/
/** @brief CAN1 接收 ID 上限（标准帧全收，先不做精细过滤） */
#define APP_CAN1_STD_ID_MAX     0x7FFu

/**
 * @brief CAN1 滤波器表
 * @note 一条规则收全部标准帧进 FIFO0；需要按 ID 分流时在这里加规则，
 *       index 不能重复（FDCAN 标准过滤列表 0 ~ 127）。
 */
static const bsp_can_filter_t APP_CAN1_FILTER_TABLE[] = {
    { .index = 0u, .fifo = (uint8_t)BSP_CAN_FIFO0, .mode = (uint8_t)BSP_CAN_FILTER_RANGE,
      .ide = (uint8_t)BSP_CAN_ID_STD, .id1 = 0u, .id2 = APP_CAN1_STD_ID_MAX },
    { .index = 1u, .fifo = (uint8_t)BSP_CAN_FIFO1, .mode = (uint8_t)BSP_CAN_FILTER_RANGE,
      .ide = (uint8_t)BSP_CAN_ID_EXT, .id1 = 0u, .id2 = BSP_CAN_EXT_ID_MAX },
};

/*==============================================================================
 * 端口 → 命令表路由
 *============================================================================*/
/**
 * @brief 接收路由表（TASK 用它把队列里的一帧交给对应分发函数）
 * @note 当前只有 CAN1 一路，FIFO0 / FIFO1 各一条。
 */
static const app_can_route_t APP_CAN_ROUTE_TABLE[] = {
    { .port = 1u, .fifo = (uint8_t)BSP_CAN_FIFO0, .handler = APP_CAN1_F0_Cmd },
    { .port = 1u, .fifo = (uint8_t)BSP_CAN_FIFO1, .handler = APP_CAN1_F1_Cmd },
};

#endif /* __APP_CAN_Cfg_H__ */
