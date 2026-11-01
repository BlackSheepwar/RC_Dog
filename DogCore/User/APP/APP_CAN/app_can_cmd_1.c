/**
 * @file app_can_cmd_1.c
 * @brief CAN1 接收命令表：FIFO0（控制帧）/ FIFO1（遥测帧）
 * @author Ricardo
 * @date 2026-11-01
 *
 * @note 表里的处理函数目前都是桩：留着是为了把"哪个 ID 归哪一类"的
 *       结构先立起来。接真实协议时，只需在对应桩里写业务，
 *       表结构与分发代码不动。
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include <stddef.h>
#include "common.h"             /* ARRAY_SIZE */
#include "app_can_cmd.h"

/*==============================================================================
 * FIFO0 处理函数（高优先级）
 *============================================================================*/
/**
 * @brief 标准数据帧 0x7FF 处理（桩）
 * @param frame 收到的帧
 * @retval 无
 */
static void APP_CAN1_F0_StdData_Max(const bsp_can_frame_t *frame)
{
    (void)frame;
}

/**
 * @brief 标准遥控帧 0x7FF 处理（桩）
 * @param frame 收到的帧
 * @retval 无
 */
static void APP_CAN1_F0_StdRemote_Max(const bsp_can_frame_t *frame)
{
    (void)frame;
}

/**
 * @brief 扩展数据帧 0x1FFFFFFF 处理（桩）
 * @param frame 收到的帧
 * @retval 无
 */
static void APP_CAN1_F0_ExtData_Max(const bsp_can_frame_t *frame)
{
    (void)frame;
}

/**
 * @brief 扩展遥控帧 0x1FFFFFFF 处理（桩）
 * @param frame 收到的帧
 * @retval 无
 */
static void APP_CAN1_F0_ExtRemote_Max(const bsp_can_frame_t *frame)
{
    (void)frame;
}

/*==============================================================================
 * FIFO1 处理函数（低优先级）
 *============================================================================*/
/**
 * @brief 标准数据帧 0x7FF 处理（桩）
 * @param frame 收到的帧
 * @retval 无
 */
static void APP_CAN1_F1_StdData_Max(const bsp_can_frame_t *frame)
{
    (void)frame;
}

/**
 * @brief 标准遥控帧 0x7FF 处理（桩）
 * @param frame 收到的帧
 * @retval 无
 */
static void APP_CAN1_F1_StdRemote_Max(const bsp_can_frame_t *frame)
{
    (void)frame;
}

/**
 * @brief 扩展数据帧 0x1FFFFFFF 处理（桩）
 * @param frame 收到的帧
 * @retval 无
 */
static void APP_CAN1_F1_ExtData_Max(const bsp_can_frame_t *frame)
{
    (void)frame;
}

/**
 * @brief 扩展遥控帧 0x1FFFFFFF 处理（桩）
 * @param frame 收到的帧
 * @retval 无
 */
static void APP_CAN1_F1_ExtRemote_Max(const bsp_can_frame_t *frame)
{
    (void)frame;
}

/*==============================================================================
 * 命令表
 *============================================================================*/
/** @brief CAN1 FIFO0 命令表（高优先级：控制/急停） */
static const can_rx_entry_t APP_CAN1_F0_RX_TABLE[] = {
    { .can_id = 0x7FFu, .ide = (uint8_t)BSP_CAN_ID_STD, .rtr = (uint8_t)BSP_CAN_FRAME_DATA,
      .handler = APP_CAN1_F0_StdData_Max },
    { .can_id = 0x7FFu, .ide = (uint8_t)BSP_CAN_ID_STD, .rtr = (uint8_t)BSP_CAN_FRAME_REMOTE,
      .handler = APP_CAN1_F0_StdRemote_Max },
    { .can_id = 0x1FFFFFFFu, .ide = (uint8_t)BSP_CAN_ID_EXT, .rtr = (uint8_t)BSP_CAN_FRAME_DATA,
      .handler = APP_CAN1_F0_ExtData_Max },
    { .can_id = 0x1FFFFFFFu, .ide = (uint8_t)BSP_CAN_ID_EXT, .rtr = (uint8_t)BSP_CAN_FRAME_REMOTE,
      .handler = APP_CAN1_F0_ExtRemote_Max },
};

/** @brief CAN1 FIFO1 命令表（低优先级：遥测/状态） */
static const can_rx_entry_t APP_CAN1_F1_RX_TABLE[] = {
    { .can_id = 0x7FFu, .ide = (uint8_t)BSP_CAN_ID_STD, .rtr = (uint8_t)BSP_CAN_FRAME_DATA,
      .handler = APP_CAN1_F1_StdData_Max },
    { .can_id = 0x7FFu, .ide = (uint8_t)BSP_CAN_ID_STD, .rtr = (uint8_t)BSP_CAN_FRAME_REMOTE,
      .handler = APP_CAN1_F1_StdRemote_Max },
    { .can_id = 0x1FFFFFFFu, .ide = (uint8_t)BSP_CAN_ID_EXT, .rtr = (uint8_t)BSP_CAN_FRAME_DATA,
      .handler = APP_CAN1_F1_ExtData_Max },
    { .can_id = 0x1FFFFFFFu, .ide = (uint8_t)BSP_CAN_ID_EXT, .rtr = (uint8_t)BSP_CAN_FRAME_REMOTE,
      .handler = APP_CAN1_F1_ExtRemote_Max },
};

/*==============================================================================
 * 分发入口
 *============================================================================*/
/**
 * @brief CAN1 FIFO0 分发
 * @param frame 收到的帧，不可为 NULL
 * @retval 无
 */
void APP_CAN1_F0_Cmd(const bsp_can_frame_t *frame)
{
    APP_CAN_Cmd_Match(APP_CAN1_F0_RX_TABLE, (uint8_t)ARRAY_SIZE(APP_CAN1_F0_RX_TABLE), frame);
}

/**
 * @brief CAN1 FIFO1 分发
 * @param frame 收到的帧，不可为 NULL
 * @retval 无
 */
void APP_CAN1_F1_Cmd(const bsp_can_frame_t *frame)
{
    APP_CAN_Cmd_Match(APP_CAN1_F1_RX_TABLE, (uint8_t)ARRAY_SIZE(APP_CAN1_F1_RX_TABLE), frame);
}
