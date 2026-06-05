/**
 * @file app_can_cmd_1.c
 * @brief CAN1 命令分发
 * @author 李嘉图
 * @date 2026-06-04
 *
 * @note CAN1 专用，FIFO0 和 FIFO1 独立分发，
 *       每种FIFO内再按四种组合展开:
 *       标准+数据 | 标准+遥控 | 扩展+数据 | 扩展+遥控
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
#include "main.h"
#include "app_can_cmd.h"
#include "common.h"
#include "protocol.h"
#include "app_se.h"

/*==============================================================================
 * FIFO0 处理函数（高优先级）
 *============================================================================*/
/* ---------- 标准帧 + 数据帧 ---------- */
#if HW_VERSION == HW_MCU_FRONT
/* 前腿MCU：响应 0x201/0x202，舵机ID 1-6 */
static const uint8_t se_cmd_map[][3] = {
    {1, 2, 3},   // CANID_LEG1 (0x201) → 舵机1/2/3
    {4, 5, 6},   // CANID_LEG2 (0x202) → 舵机4/5/6
};
#else
/* 后腿MCU：响应 0x203/0x204，舵机ID 7-12 */
static const uint8_t se_cmd_map[][3] = {
    {7, 8, 9},   // CANID_LEG3 (0x203) → 舵机7/8/9
    {10, 11,12}, // CANID_LEG4 (0x204) → 舵机10/11/12
};
#endif

static void APP_CAN1_F0_StdData_20x(CAN_RxHeaderTypeDef *header, uint8_t *data)
{
    if (header->DLC != PROTOCOL_SE_DATA_LEN) return;

#if HW_VERSION == HW_MCU_FRONT
    uint8_t idx = header->StdId - CANID_LEG1;
#else
    uint8_t idx = header->StdId - CANID_LEG3;
#endif

    int16_t angles[PROTOCOL_SE_PER_FRAME];

    Protocol_DecodeServoFrame(data, angles, PROTOCOL_SE_PER_FRAME);

    for (uint8_t i = 0; i < PROTOCOL_SE_PER_FRAME; i++)
    {
        APP_SE_SetTarget(se_cmd_map[idx][i], angles[i]);
    }
}

/* ---------- 标准帧 + 遥控帧 ---------- */
static void APP_CAN1_F0_StdRemote_7FF(CAN_RxHeaderTypeDef *header)
{
    (void)header;
}

/* ---------- 扩展帧 + 数据帧 ---------- */
static void APP_CAN1_F0_ExtData_1FFFFFFF(CAN_RxHeaderTypeDef *header, uint8_t *data)
{
    (void)header;
    (void)data;
}

/* ---------- 扩展帧 + 遥控帧 ---------- */
static void APP_CAN1_F0_ExtRemote_1FFFFFFF(CAN_RxHeaderTypeDef *header)
{
    (void)header;
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
static void APP_CAN1_F1_StdRemote_7FF(CAN_RxHeaderTypeDef *header)
{
    (void)header;
}

/* ---------- 扩展帧 + 数据帧 ---------- */
static void APP_CAN1_F1_ExtData_1FFFFFFF(CAN_RxHeaderTypeDef *header, uint8_t *data)
{
    (void)header;
    (void)data;
}

/* ---------- 扩展帧 + 遥控帧 ---------- */
static void APP_CAN1_F1_ExtRemote_1FFFFFFF(CAN_RxHeaderTypeDef *header)
{
    (void)header;
}

/*==============================================================================
 * CAN1 FIFO0 命令分发
 *============================================================================*/
void APP_CAN1_F0_Cmd(CAN_RxHeaderTypeDef *header, uint8_t *data)
{
    if (header->IDE == CAN_ID_STD)
    {
        if (header->RTR == CAN_RTR_DATA)
        {
            switch (header->StdId)
            {
#if HW_VERSION == HW_MCU_FRONT
                case CANID_LEG1:
                case CANID_LEG2:  APP_CAN1_F0_StdData_20x(header, data); break;
#else
                case CANID_LEG3:
                case CANID_LEG4:  APP_CAN1_F0_StdData_20x(header, data); break;
#endif
                default: break;
            }
        }
        else
        {
            switch (header->StdId)
            {
                case 0x7FF: APP_CAN1_F0_StdRemote_7FF(header); break;
                default: break;
            }
        }
    }
    else
    {
        if (header->RTR == CAN_RTR_DATA)
        {
            switch (header->ExtId)
            {
                case 0x1FFFFFFF: APP_CAN1_F0_ExtData_1FFFFFFF(header, data); break;
                default: break;
            }
        }
        else
        {
            switch (header->ExtId)
            {
                case 0x1FFFFFFF: APP_CAN1_F0_ExtRemote_1FFFFFFF(header); break;
                default: break;
            }
        }
    }
}

/*==============================================================================
 * CAN1 FIFO1 命令分发
 *============================================================================*/
void APP_CAN1_F1_Cmd(CAN_RxHeaderTypeDef *header, uint8_t *data)
{
    if (header->IDE == CAN_ID_STD)
    {
        if (header->RTR == CAN_RTR_DATA)
        {
            switch (header->StdId)
            {
                case 0x7FF: APP_CAN1_F1_StdData_7FF(header, data); break;
                default: break;
            }
        }
        else
        {
            switch (header->StdId)
            {
                case 0x7FF: APP_CAN1_F1_StdRemote_7FF(header); break;
                default: break;
            }
        }
    }
    else
    {
        if (header->RTR == CAN_RTR_DATA)
        {
            switch (header->ExtId)
            {
                case 0x1FFFFFFF: APP_CAN1_F1_ExtData_1FFFFFFF(header, data); break;
                default: break;
            }
        }
        else
        {
            switch (header->ExtId)
            {
                case 0x1FFFFFFF: APP_CAN1_F1_ExtRemote_1FFFFFFF(header); break;
                default: break;
            }
        }
    }
}
