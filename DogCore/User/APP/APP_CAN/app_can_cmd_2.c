/**
 * @file app_can_cmd_2.c
 * @brief CAN2 接收命令表（保留接口，当前硬件没有 CAN2）
 * @author Ricardo
 * @date 2026-11-01
 *
 * @note 结构与 app_can_cmd_1.c 完全一致：两路 CAN 各自一张表，
 *       分发入口签名相同，路由表里换一下函数指针就能启用。
 *       所有处理函数都是桩，接上第二路 CAN 时再填业务。
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include <stddef.h>
#include "common.h"             /* ARRAY_SIZE */
#include "app_can_cmd.h"

/*==============================================================================
 * FIFO0 处理函数（高优先级，桩）
 *============================================================================*/
/**
 * @brief 标准数据帧 0x7FF 处理（桩）
 * @param frame 收到的帧
 * @retval 无
 */
static void APP_CAN2_F0_StdData_Max(const bsp_can_frame_t *frame)
{
    (void)frame;
}

/**
 * @brief 标准遥控帧 0x7FF 处理（桩）
 * @param frame 收到的帧
 * @retval 无
 */
static void APP_CAN2_F0_StdRemote_Max(const bsp_can_frame_t *frame)
{
    (void)frame;
}

/**
 * @brief 扩展数据帧 0x1FFFFFFF 处理（桩）
 * @param frame 收到的帧
 * @retval 无
 */
static void APP_CAN2_F0_ExtData_Max(const bsp_can_frame_t *frame)
{
    (void)frame;
}

/**
 * @brief 扩展遥控帧 0x1FFFFFFF 处理（桩）
 * @param frame 收到的帧
 * @retval 无
 */
static void APP_CAN2_F0_ExtRemote_Max(const bsp_can_frame_t *frame)
{
    (void)frame;
}

/*==============================================================================
 * FIFO1 处理函数（低优先级，桩）
 *============================================================================*/
/**
 * @brief 标准数据帧 0x7FF 处理（桩）
 * @param frame 收到的帧
 * @retval 无
 */
static void APP_CAN2_F1_StdData_Max(const bsp_can_frame_t *frame)
{
    (void)frame;
}

/**
 * @brief 标准遥控帧 0x7FF 处理（桩）
 * @param frame 收到的帧
 * @retval 无
 */
static void APP_CAN2_F1_StdRemote_Max(const bsp_can_frame_t *frame)
{
    (void)frame;
}

/**
 * @brief 扩展数据帧 0x1FFFFFFF 处理（桩）
 * @param frame 收到的帧
 * @retval 无
 */
static void APP_CAN2_F1_ExtData_Max(const bsp_can_frame_t *frame)
{
    (void)frame;
}

/**
 * @brief 扩展遥控帧 0x1FFFFFFF 处理（桩）
 * @param frame 收到的帧
 * @retval 无
 */
static void APP_CAN2_F1_ExtRemote_Max(const bsp_can_frame_t *frame)
{
    (void)frame;
}

/*==============================================================================
 * 命令表
 *============================================================================*/
/** @brief CAN2 FIFO0 命令表 */
static const can_rx_entry_t APP_CAN2_F0_RX_TABLE[] = {
    { .can_id = 0x7FFu, .ide = (uint8_t)BSP_CAN_ID_STD, .rtr = (uint8_t)BSP_CAN_FRAME_DATA,
      .handler = APP_CAN2_F0_StdData_Max },
    { .can_id = 0x7FFu, .ide = (uint8_t)BSP_CAN_ID_STD, .rtr = (uint8_t)BSP_CAN_FRAME_REMOTE,
      .handler = APP_CAN2_F0_StdRemote_Max },
    { .can_id = 0x1FFFFFFFu, .ide = (uint8_t)BSP_CAN_ID_EXT, .rtr = (uint8_t)BSP_CAN_FRAME_DATA,
      .handler = APP_CAN2_F0_ExtData_Max },
    { .can_id = 0x1FFFFFFFu, .ide = (uint8_t)BSP_CAN_ID_EXT, .rtr = (uint8_t)BSP_CAN_FRAME_REMOTE,
      .handler = APP_CAN2_F0_ExtRemote_Max },
};

/** @brief CAN2 FIFO1 命令表 */
static const can_rx_entry_t APP_CAN2_F1_RX_TABLE[] = {
    { .can_id = 0x7FFu, .ide = (uint8_t)BSP_CAN_ID_STD, .rtr = (uint8_t)BSP_CAN_FRAME_DATA,
      .handler = APP_CAN2_F1_StdData_Max },
    { .can_id = 0x7FFu, .ide = (uint8_t)BSP_CAN_ID_STD, .rtr = (uint8_t)BSP_CAN_FRAME_REMOTE,
      .handler = APP_CAN2_F1_StdRemote_Max },
    { .can_id = 0x1FFFFFFFu, .ide = (uint8_t)BSP_CAN_ID_EXT, .rtr = (uint8_t)BSP_CAN_FRAME_DATA,
      .handler = APP_CAN2_F1_ExtData_Max },
    { .can_id = 0x1FFFFFFFu, .ide = (uint8_t)BSP_CAN_ID_EXT, .rtr = (uint8_t)BSP_CAN_FRAME_REMOTE,
      .handler = APP_CAN2_F1_ExtRemote_Max },
};

/*==============================================================================
 * 分发入口
 *============================================================================*/
/**
 * @brief CAN2 FIFO0 分发
 * @param frame 收到的帧，不可为 NULL
 * @retval 无
 */
void APP_CAN2_F0_Cmd(const bsp_can_frame_t *frame)
{
    APP_CAN_Cmd_Match(APP_CAN2_F0_RX_TABLE, (uint8_t)ARRAY_SIZE(APP_CAN2_F0_RX_TABLE), frame);
}

/**
 * @brief CAN2 FIFO1 分发
 * @param frame 收到的帧，不可为 NULL
 * @retval 无
 */
void APP_CAN2_F1_Cmd(const bsp_can_frame_t *frame)
{
    APP_CAN_Cmd_Match(APP_CAN2_F1_RX_TABLE, (uint8_t)ARRAY_SIZE(APP_CAN2_F1_RX_TABLE), frame);
}
