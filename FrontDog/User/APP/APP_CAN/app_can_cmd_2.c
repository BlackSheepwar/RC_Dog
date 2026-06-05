/**
 * @file app_can_cmd_2.c
 * @brief CAN2 命令分发
 * @author 李嘉图
 * @date 2026-06-04
 *
 * @note CAN2 专用，FIFO0 和 FIFO1 独立分发，
 *       每种FIFO内再按四种组合展开:
 *       标准+数据 | 标准+遥控 | 扩展+数据 | 扩展+遥控
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
#include "main.h"
#include "app_can_cmd.h"

/*==============================================================================
 * FIFO0 处理函数（高优先级）
 *============================================================================*/
/* ---------- 标准帧 + 数据帧 ---------- */
static void APP_CAN2_F0_StdData_7FF(CAN_RxHeaderTypeDef *header, uint8_t *data)
{
    (void)header;
    (void)data;
}

/* ---------- 标准帧 + 遥控帧 ---------- */
static void APP_CAN2_F0_StdRemote_7FF(CAN_RxHeaderTypeDef *header)
{
    (void)header;
}

/* ---------- 扩展帧 + 数据帧 ---------- */
static void APP_CAN2_F0_ExtData_1FFFFFFF(CAN_RxHeaderTypeDef *header, uint8_t *data)
{
    (void)header;
    (void)data;
}

/* ---------- 扩展帧 + 遥控帧 ---------- */
static void APP_CAN2_F0_ExtRemote_1FFFFFFF(CAN_RxHeaderTypeDef *header)
{
    (void)header;
}

/*==============================================================================
 * FIFO1 处理函数（低优先级）
 *============================================================================*/
/* ---------- 标准帧 + 数据帧 ---------- */
static void APP_CAN2_F1_StdData_7FF(CAN_RxHeaderTypeDef *header, uint8_t *data)
{
    (void)header;
    (void)data;
}

/* ---------- 标准帧 + 遥控帧 ---------- */
static void APP_CAN2_F1_StdRemote_7FF(CAN_RxHeaderTypeDef *header)
{
    (void)header;
}

/* ---------- 扩展帧 + 数据帧 ---------- */
static void APP_CAN2_F1_ExtData_1FFFFFFF(CAN_RxHeaderTypeDef *header, uint8_t *data)
{
    (void)header;
    (void)data;
}

/* ---------- 扩展帧 + 遥控帧 ---------- */
static void APP_CAN2_F1_ExtRemote_1FFFFFFF(CAN_RxHeaderTypeDef *header)
{
    (void)header;
}

/*==============================================================================
 * CAN2 FIFO0 命令分发
 *============================================================================*/
void APP_CAN2_F0_Cmd(CAN_RxHeaderTypeDef *header, uint8_t *data)
{
    if (header->IDE == CAN_ID_STD)
    {
        if (header->RTR == CAN_RTR_DATA)
        {
            switch (header->StdId)
            {
                case 0x7FF: APP_CAN2_F0_StdData_7FF(header, data); break;
                default: break;
            }
        }
        else
        {
            switch (header->StdId)
            {
                case 0x7FF: APP_CAN2_F0_StdRemote_7FF(header); break;
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
                case 0x1FFFFFFF: APP_CAN2_F0_ExtData_1FFFFFFF(header, data); break;
                default: break;
            }
        }
        else
        {
            switch (header->ExtId)
            {
                case 0x1FFFFFFF: APP_CAN2_F0_ExtRemote_1FFFFFFF(header); break;
                default: break;
            }
        }
    }
}

/*==============================================================================
 * CAN2 FIFO1 命令分发
 *============================================================================*/
void APP_CAN2_F1_Cmd(CAN_RxHeaderTypeDef *header, uint8_t *data)
{
    if (header->IDE == CAN_ID_STD)
    {
        if (header->RTR == CAN_RTR_DATA)
        {
            switch (header->StdId)
            {
                case 0x7FF: APP_CAN2_F1_StdData_7FF(header, data); break;
                default: break;
            }
        }
        else
        {
            switch (header->StdId)
            {
                case 0x7FF: APP_CAN2_F1_StdRemote_7FF(header); break;
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
                case 0x1FFFFFFF: APP_CAN2_F1_ExtData_1FFFFFFF(header, data); break;
                default: break;
            }
        }
        else
        {
            switch (header->ExtId)
            {
                case 0x1FFFFFFF: APP_CAN2_F1_ExtRemote_1FFFFFFF(header); break;
                default: break;
            }
        }
    }
}
