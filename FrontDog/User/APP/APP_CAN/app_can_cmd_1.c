/**
 * @file app_can_cmd_1.c
 * @brief CAN1 命令分发（配置表驱动）
 * @author 李嘉图
 * @date 2026-6-5
 *
 * @note 原先的嵌套 if(IDE)->if(RTR)->switch(StdId/ExtId) 已替换为
 *       CAN RX 配置表，每种帧组合对应一条表项，match 逻辑集中在一处。
 *       新增 CAN ID 只需在表中添加一行，不再改动分发函数结构。
 *       CAN1 FIFO0 和 FIFO1 各有独立配置表。
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include "main.h"
#include "app_can_cmd.h"
#include "common.h"
// 功能包含
#include "app_servo.h"

/*==============================================================================
 * FIFO0 处理函数（高优先级）
 *============================================================================*/
/* ---------- 标准帧 + 数据帧 ---------- */

static void APP_CAN1_F0_StdData_7FF(CAN_RxHeaderTypeDef *header, uint8_t *data)
{
    (void)header;
    (void)data;
}

/* ---------- 标准帧 + 遥控帧 ---------- */
static void APP_CAN1_F0_StdRemote_7FF(CAN_RxHeaderTypeDef *header, uint8_t *data)
{
    (void)header;
    (void)data;
}

/* ---------- 扩展帧 + 数据帧 ---------- */
static void APP_CAN1_F0_ExtData_1FFFFFFF(CAN_RxHeaderTypeDef *header, uint8_t *data)
{
    (void)header;
    (void)data;
}

/* ---------- 扩展帧 + 遥控帧 ---------- */
static void APP_CAN1_F0_ExtRemote_1FFFFFFF(CAN_RxHeaderTypeDef *header, uint8_t *data)
{
    (void)header;
    (void)data;
}

/*==============================================================================
 * FIFO1 处理函数（低优先级）
 *============================================================================*/
/* ---------- 标准帧 + 数据帧 ---------- */
static void APP_CAN1_F1_StdData_7FF(CAN_RxHeaderTypeDef *header, uint8_t *data)
{
    (void)header;
    (void)data;
}

/* ---------- 标准帧 + 遥控帧 ---------- */
static void APP_CAN1_F1_StdRemote_7FF(CAN_RxHeaderTypeDef *header, uint8_t *data)
{
    (void)header;
    (void)data;
}

/* ---------- 扩展帧 + 数据帧 ---------- */
static void APP_CAN1_F1_ExtData_1FFFFFFF(CAN_RxHeaderTypeDef *header, uint8_t *data)
{
    (void)header;
    (void)data;
}

/* ---------- 扩展帧 + 遥控帧 ---------- */
static void APP_CAN1_F1_ExtRemote_1FFFFFFF(CAN_RxHeaderTypeDef *header, uint8_t *data)
{
    (void)header;
    (void)data;
}

/*==============================================================================
 * CAN1 FIFO0 RX 配置表（高优先级）
 *============================================================================*/
static const can_rx_entry_t CAN1_F0_RX_TABLE[] = {
    /* 标准帧 + 数据：舵机控制 */
    { .can_id = 0x7FF, .ide = CAN_ID_STD, .rtr = CAN_RTR_DATA, .handler = APP_CAN1_F0_StdData_7FF },
    /* 标准帧 + 遥控 */
    { .can_id = 0x7FF, .ide = CAN_ID_STD, .rtr = CAN_RTR_REMOTE, .handler = APP_CAN1_F0_StdRemote_7FF },
    /* 扩展帧 + 数据 */
    { .can_id = 0x1FFFFFFF, .ide = CAN_ID_EXT, .rtr = CAN_RTR_DATA, .handler = APP_CAN1_F0_ExtData_1FFFFFFF },
    /* 扩展帧 + 遥控 */
    { .can_id = 0x1FFFFFFF, .ide = CAN_ID_EXT, .rtr = CAN_RTR_REMOTE, .handler = APP_CAN1_F0_ExtRemote_1FFFFFFF },
};

/*==============================================================================
 * CAN1 FIFO1 RX 配置表（低优先级）
 *============================================================================*/
static const can_rx_entry_t CAN1_F1_RX_TABLE[] = {
    /* 标准帧 + 数据 */
    { .can_id = 0x7FF, .ide = CAN_ID_STD, .rtr = CAN_RTR_DATA, .handler = APP_CAN1_F1_StdData_7FF },
    /* 标准帧 + 遥控 */
    { .can_id = 0x7FF, .ide = CAN_ID_STD, .rtr = CAN_RTR_REMOTE, .handler = APP_CAN1_F1_StdRemote_7FF },
    /* 扩展帧 + 数据 */
    { .can_id = 0x1FFFFFFF, .ide = CAN_ID_EXT, .rtr = CAN_RTR_DATA, .handler = APP_CAN1_F1_ExtData_1FFFFFFF },
    /* 扩展帧 + 遥控 */
    { .can_id = 0x1FFFFFFF, .ide = CAN_ID_EXT, .rtr = CAN_RTR_REMOTE, .handler = APP_CAN1_F1_ExtRemote_1FFFFFFF },
};

/*==============================================================================
 * CAN1 FIFO0 命令分发
 *============================================================================*/
void APP_CAN1_F0_Cmd(CAN_RxHeaderTypeDef *header, uint8_t *data)
{
    CAN_DispatchByTable(CAN1_F0_RX_TABLE, ARRAY_SIZE(CAN1_F0_RX_TABLE), header, data);
}

/*==============================================================================
 * CAN1 FIFO1 命令分发
 *============================================================================*/
void APP_CAN1_F1_Cmd(CAN_RxHeaderTypeDef *header, uint8_t *data)
{
    CAN_DispatchByTable(CAN1_F1_RX_TABLE, ARRAY_SIZE(CAN1_F1_RX_TABLE), header, data);
}
