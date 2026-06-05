/**
 * @file protocol.c
 * @brief 通信协议格式转换实现
 * @author 李嘉图
 * @date 2026-06-04
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
#include "protocol.h"

/*==============================================================================
 * 角度 → 2字节（小端序）
 *============================================================================*/
void Protocol_AngleToBytes(int16_t angle, uint8_t *bytes)
{
    if (angle < PROTOCOL_SE_ANGLE_MIN) angle = PROTOCOL_SE_ANGLE_MIN;
    if (angle > PROTOCOL_SE_ANGLE_MAX) angle = PROTOCOL_SE_ANGLE_MAX;

    bytes[0] = (uint8_t)(angle & 0xFF);         // 低字节
    bytes[1] = (uint8_t)((angle >> 8) & 0xFF);   // 高字节
}

/*==============================================================================
 * 2字节 → 角度（小端序）
 *============================================================================*/
int16_t Protocol_BytesToAngle(const uint8_t *bytes)
{
    int16_t angle = (int16_t)(bytes[0] | ((uint16_t)bytes[1] << 8));

    if (angle < PROTOCOL_SE_ANGLE_MIN) angle = PROTOCOL_SE_ANGLE_MIN;
    if (angle > PROTOCOL_SE_ANGLE_MAX) angle = PROTOCOL_SE_ANGLE_MAX;

    return angle;
}

/*==============================================================================
 * 批量解码（帧 → 角度数组）
 *============================================================================*/
void Protocol_DecodeServoFrame(const uint8_t *data, int16_t *angles, uint8_t count)
{
    for (uint8_t i = 0; i < count; i++)
    {
        angles[i] = Protocol_BytesToAngle(&data[i * 2]);
    }
}

/*==============================================================================
 * 批量编码（角度数组 → 帧）
 *============================================================================*/
void Protocol_EncodeServoFrame(const int16_t *angles, uint8_t count, uint8_t *data)
{
    for (uint8_t i = 0; i < count; i++)
    {
        Protocol_AngleToBytes(angles[i], &data[i * 2]);
    }
}
