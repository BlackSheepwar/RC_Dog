/**
 * @file app_can_cmd.h
 * @brief CAN 接收命令表：ID + 帧类型 → 处理函数
 * @author Ricardo
 * @date 2026-11-01
 *
 * @note 分发不再有 if(IDE)→if(RTR)→switch(ID) 的嵌套：
 *       每个"帧组合"一条表项，匹配逻辑只写在 APP_CAN_Cmd_Match 一处。
 *       新增一个 ID 就是加一行，加一个端口就是加一张表。
 *
 *       处理函数的形参是 bsp_can_frame_t（值类型），不是 HAL 帧头：
 *       APP 不该认识 FDCAN_RxHeaderTypeDef。
 */

#ifndef __APP_CAN_Cmd_H__
#define __APP_CAN_Cmd_H__

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include <stddef.h>             /* NULL：行内匹配函数要判空 */
#include <stdint.h>
#include "bsp_can.h"

/*==============================================================================
 * 类型定义
 *============================================================================*/
/**
 * @brief 帧处理函数原型
 * @note 在 TASK 上下文执行，可以慢（启动步态、改 PID），但不能阻塞等事件。
 */
typedef void (*can_rx_handler_t)(const bsp_can_frame_t *frame);

/**
 * @brief 命令表项（值类型：表元素）
 */
typedef struct {
    uint32_t          can_id;   /* 匹配的帧 ID */
    uint8_t           ide;      /* ID 类型，取 bsp_can_id_t */
    uint8_t           rtr;      /* 帧类型，取 bsp_can_frame_type_t */
    can_rx_handler_t  handler;  /* 命中后调用 */
} can_rx_entry_t;

/*==============================================================================
 * 分发辅助（行内函数：调用点只有两处，放头文件省一个符号）
 *============================================================================*/
/**
 * @brief 在一张命令表里查找并调用匹配项
 * @param table  命令表，不可为 NULL
 * @param count  表项数
 * @param frame  收到的帧，不可为 NULL
 * @retval 无
 * @note ID / ID 类型 / 帧类型三者都对上才算命中；未命中静默丢弃
 *       （现场调试靠 BSP 层的接收计数判断是不是"没人收"）。
 */
static inline void APP_CAN_Cmd_Match(const can_rx_entry_t *table, uint8_t count,
                                     const bsp_can_frame_t *frame)
{
    if ((table == NULL) || (frame == NULL)) {
        return;
    }

    for (uint8_t i = 0u; i < count; i++) {
        if ((table[i].can_id == frame->id)
            && (table[i].ide == frame->ide)
            && (table[i].rtr == frame->rtr)) {
            table[i].handler(frame);
            return;
        }
    }
}

/*==============================================================================
 * 各端口 FIFO 分发入口
 *============================================================================*/
/**
 * @brief CAN1 FIFO0 分发（高优先级：控制帧）
 * @param frame 收到的帧，不可为 NULL
 * @retval 无
 */
void APP_CAN1_F0_Cmd(const bsp_can_frame_t *frame);

/**
 * @brief CAN1 FIFO1 分发（低优先级：遥测帧）
 * @param frame 收到的帧，不可为 NULL
 * @retval 无
 */
void APP_CAN1_F1_Cmd(const bsp_can_frame_t *frame);

/**
 * @brief CAN2 FIFO0 分发（保留接口，当前无 CAN2 硬件）
 * @param frame 收到的帧，不可为 NULL
 * @retval 无
 */
void APP_CAN2_F0_Cmd(const bsp_can_frame_t *frame);

/**
 * @brief CAN2 FIFO1 分发（保留接口，当前无 CAN2 硬件）
 * @param frame 收到的帧，不可为 NULL
 * @retval 无
 */
void APP_CAN2_F1_Cmd(const bsp_can_frame_t *frame);

#endif /* __APP_CAN_Cmd_H__ */
