/**
 * @file protocol.h
 * @brief 通信协议定义与格式转换
 * @author 李嘉图
 * @date 2026-06-04
 *
 * @note 定义上下位机之间的 CAN 协议：帧ID含义、数据格式、编码/解码函数。
 *       纯 C 标准，无硬件依赖，换 MCU 平台时此文件不动。
 */

#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

#include "common.h"

/*==============================================================================
 * CAN 帧ID 定义（上位机 → 下位机）
 *============================================================================*/
#define CANID_LEG1      0x201   // 左前腿舵机控制（3路角度）
#define CANID_LEG2      0x202   // 右前腿舵机控制（3路角度）
#define CANID_LEG3      0x203   // 左后腿舵机控制（3路角度）
#define CANID_LEG4      0x204   // 右后腿舵机控制（3路角度）

/* 兼容旧名 */
#define CANID_LEG_FRONT CANID_LEG1
#define CANID_LEG_REAR  CANID_LEG2

/*==============================================================================
 * CAN 数据帧格式规范
 *
 * 舵机控制帧 (0x201 / 0x202)   DLC=6
 * ┌────────┬────────┬────────┬────────┬────────┬────────┐
 * │ 舵机1  │ 舵机1  │ 舵机2  │ 舵机2  │ 舵机3  │ 舵机3  │
 * │ 低字节 │ 高字节 │ 低字节 │ 高字节 │ 低字节 │ 高字节 │
 * ├────────┼────────┼────────┼────────┼────────┼────────┤
 * │  data[0] │ data[1] │ data[2] │ data[3] │ data[4] │ data[5] │
 * └────────┴────────┴────────┴────────┴────────┴────────┘
 * 每个角度为 int16_t 小端序，范围 -125~+125
 *============================================================================*/
#define PROTOCOL_SE_DATA_LEN    6       // 舵机控制帧数据长度
#define PROTOCOL_SE_PER_FRAME   3       // 每帧控制的舵机数量
#define PROTOCOL_SE_ANGLE_MIN   (-125)  // 舵机角度下限
#define PROTOCOL_SE_ANGLE_MAX   125     // 舵机角度上限

/*==============================================================================
 * 协议转换函数
 *============================================================================*/
/**
 * @brief 角度值编码为小端序2字节
 * @param angle  目标角度（自动限幅 -125~+125）
 * @param bytes  输出缓冲区（2字节）
 */
void Protocol_AngleToBytes(int16_t angle, uint8_t *bytes);

/**
 * @brief 小端序2字节解码为角度值
 * @param bytes  输入缓冲区（2字节）
 * @return 解码后的角度值（已限幅 -125~+125）
 */
int16_t Protocol_BytesToAngle(const uint8_t *bytes);

/**
 * @brief 解码舵机控制帧（6字节 → 3个角度）
 * @param data    CAN帧数据（6字节）
 * @param angles  输出角度数组（3个元素）
 * @param count   解码数量（≤ PROTOCOL_SE_PER_FRAME）
 */
void Protocol_DecodeServoFrame(const uint8_t *data, int16_t *angles, uint8_t count);

/**
 * @brief 编码舵机控制帧（3个角度 → 6字节）
 * @param angles  输入角度数组
 * @param count   角度数量
 * @param data    输出CAN帧数据（6字节）
 */
void Protocol_EncodeServoFrame(const int16_t *angles, uint8_t count, uint8_t *data);

#endif /* __PROTOCOL_H__ */
