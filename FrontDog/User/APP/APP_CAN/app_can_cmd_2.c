/**
 * @file app_can_cmd_2.c
 * @brief CAN2 命令分发（配置表驱动，保留接口）
 * @author 李嘉图
 * @date 2026-06-05
 *
 * @note 与 app_can_cmd_1.c 同结构，使用 CAN RX 配置表。
 *       所有 handler 目前保留为桩函数，后续有实际逻辑时在表中添加条目即可。
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
#include "main.h"
#include "app_can_cmd.h"
#include "common.h"

/*==============================================================================
 * FIFO0 处理函数（高优先级，均为桩函数）
 *============================================================================*/
/* ---------- 标准帧 + 数据帧 ---------- */
static void APP_CAN2_F0_StdData_7FF(CAN_RxHeaderTypeDef *header, uint8_t *data)
{
    (void)header;
    (void)data;
}

/* ---------- 标准帧 + 遥控帧 ---------- */
static void APP_CAN2_F0_StdRemote_7FF(CAN_RxHeaderTypeDef *header, uint8_t *data)
{
    (void)header;
    (void)data;
}

/* ---------- 扩展帧 + 数据帧 ---------- */
static void APP_CAN2_F0_ExtData_1FFFFFFF(CAN_RxHeaderTypeDef *header, uint8_t *data)
{
    (void)header;
    (void)data;
}

/* ---------- 扩展帧 + 遥控帧 ---------- */
static void APP_CAN2_F0_ExtRemote_1FFFFFFF(CAN_RxHeaderTypeDef *header, uint8_t *data)
{
    (void)header;
    (void)data;
}

/*==============================================================================
 * FIFO1 处理函数（低优先级，均为桩函数）
 *============================================================================*/
/* ---------- 标准帧 + 数据帧 ---------- */
static void APP_CAN2_F1_StdData_7FF(CAN_RxHeaderTypeDef *header, uint8_t *data)
{
    (void)header;
    (void)data;
}

/* ---------- 标准帧 + 遥控帧 ---------- */
static void APP_CAN2_F1_StdRemote_7FF(CAN_RxHeaderTypeDef *header, uint8_t *data)
{
    (void)header;
    (void)data;
}

/* ---------- 扩展帧 + 数据帧 ---------- */
static void APP_CAN2_F1_ExtData_1FFFFFFF(CAN_RxHeaderTypeDef *header, uint8_t *data)
{
    (void)header;
    (void)data;
}

/* ---------- 扩展帧 + 遥控帧 ---------- */
static void APP_CAN2_F1_ExtRemote_1FFFFFFF(CAN_RxHeaderTypeDef *header, uint8_t *data)
{
    (void)header;
    (void)data;
}

/*==============================================================================
 * CAN2 FIFO0 RX 配置表（高优先级）
 *============================================================================*/
static const can_rx_entry_t CAN2_F0_RX_TABLE[] = {
    /* 标准帧 + 数据 */
    { .can_id = 0x7FF, .ide = CAN_ID_STD, .rtr = CAN_RTR_DATA, .handler = APP_CAN2_F0_StdData_7FF },
    /* 标准帧 + 遥控 */
    { .can_id = 0x7FF, .ide = CAN_ID_STD, .rtr = CAN_RTR_REMOTE, .handler = APP_CAN2_F0_StdRemote_7FF },
    /* 扩展帧 + 数据 */
    { .can_id = 0x1FFFFFFF, .ide = CAN_ID_EXT, .rtr = CAN_RTR_DATA, .handler = APP_CAN2_F0_ExtData_1FFFFFFF },
    /* 扩展帧 + 遥控 */
    { .can_id = 0x1FFFFFFF, .ide = CAN_ID_EXT, .rtr = CAN_RTR_REMOTE, .handler = APP_CAN2_F0_ExtRemote_1FFFFFFF },
};

/*==============================================================================
 * CAN2 FIFO1 RX 配置表（低优先级）
 *============================================================================*/
static const can_rx_entry_t CAN2_F1_RX_TABLE[] = {
    /* 标准帧 + 数据 */
    { .can_id = 0x7FF, .ide = CAN_ID_STD, .rtr = CAN_RTR_DATA, .handler = APP_CAN2_F1_StdData_7FF },
    /* 标准帧 + 遥控 */
    { .can_id = 0x7FF, .ide = CAN_ID_STD, .rtr = CAN_RTR_REMOTE, .handler = APP_CAN2_F1_StdRemote_7FF },
    /* 扩展帧 + 数据 */
    { .can_id = 0x1FFFFFFF, .ide = CAN_ID_EXT, .rtr = CAN_RTR_DATA, .handler = APP_CAN2_F1_ExtData_1FFFFFFF },
    /* 扩展帧 + 遥控 */
    { .can_id = 0x1FFFFFFF, .ide = CAN_ID_EXT, .rtr = CAN_RTR_REMOTE, .handler = APP_CAN2_F1_ExtRemote_1FFFFFFF },
};

/*==============================================================================
 * CAN2 FIFO0 命令分发
 *============================================================================*/
void APP_CAN2_F0_Cmd(CAN_RxHeaderTypeDef *header, uint8_t *data)
{
    CAN_DispatchByTable(CAN2_F0_RX_TABLE, ARRAY_SIZE(CAN2_F0_RX_TABLE), header, data);
}

/*==============================================================================
 * CAN2 FIFO1 命令分发
 *============================================================================*/
void APP_CAN2_F1_Cmd(CAN_RxHeaderTypeDef *header, uint8_t *data)
{
    CAN_DispatchByTable(CAN2_F1_RX_TABLE, ARRAY_SIZE(CAN2_F1_RX_TABLE), header, data);
}
