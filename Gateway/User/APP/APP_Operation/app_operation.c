/**
 * @file app_operation.c
 * @brief 操作指令层实现 - 按腿预设姿态
 * @author 李嘉图
 * @date 2026-06-04
 *
 * @note 每条腿 3 个角度值通过 protocol.h 编码后，
 *       经 APP_CAN_SendMsg 发送到对应 CAN ID。
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
#include "app_operation.h"
#include "app_can.h"
#include "protocol.h"

/*==============================================================================
 * 本地缓存
 *============================================================================*/
static uint8_t  op_tx_buf[PROTOCOL_SE_DATA_LEN];

/*==============================================================================
 * 预设姿态角度
 *============================================================================*/
static const int16_t POSE_STAND[3] = {   0, -33,  84 };   // 站立
static const int16_t POSE_SIT[3]   = {   0,   0,   0 };   // 蹲下

/*==============================================================================
 * 内部辅助
 *============================================================================*/
static void Op_SendPose(const int16_t pose[3], uint32_t leg_id)
{
    Protocol_EncodeServoFrame(pose, PROTOCOL_SE_PER_FRAME, op_tx_buf);
    APP_CAN_SendMsg(1, leg_id, BSP_CAN_STDID,
                    BSP_CAN_DATA_FRAME, op_tx_buf, PROTOCOL_SE_DATA_LEN);
}

/*==============================================================================
 * 预设姿态命令实现
 *============================================================================*/
void APP_OP_LegStand(uint32_t leg)
{
    Op_SendPose(POSE_STAND, leg);
}

void APP_OP_LegSit(uint32_t leg)
{
    Op_SendPose(POSE_SIT, leg);
}
