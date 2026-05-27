/**
 * @file bsp_can.h
 * @brief CAN硬件接口
 * @author 李嘉图
 * @date 2026-05-14
 */

#ifndef __BSP_CAN_H__
#define __BSP_CAN_H__

/*==============================================================================
 * 头文件包含
 *============================================================================*/
#include "can.h"

/*==============================================================================
 * 宏定义与常量
 *============================================================================*/
// 滤波器编号
#define CAN_FILTER(x) ((x) << 3)

// 接收队列
#define CAN_FILT_0 (0 << 2)
#define CAN_FILT_1 (1 << 2)

// 标准帧或扩展帧
#define CAN_STDID (0 << 1)
#define CAN_EXTID (1 << 1)

// 数据帧或遥控针
#define CAN_DATA_TYPE (0 << 0)
#define CAN_REMOTE_TYPE (1 << 0)

/*==============================================================================
 * 初始化函数
 *============================================================================*/
 /**
 * @brief  CAN 初始化
 *
 * @param hcan1 CAN编号
 * @retval Callback_Function 处理回调函数
 */
void CAN_Init(CAN_HandleTypeDef *hcan);

/**
* @brief 配置CAN的滤波器
* @param hcan CAN编号
* @param Object_Para 编号[3:] | FIFOx[2:2] | ID类型[1:1] | 帧类型[0:0]
* @param ID ID * 
* @param Mask_ID 屏蔽位(0x7ff, 0x1fffffff) 
*/ 
void CAN_Filter_Mask_Config(CAN_HandleTypeDef *hcan, uint8_t Object_Para, 
uint32_t ID, uint32_t Mask_ID);

/*==============================================================================
 * 发送数据帧函数
 *============================================================================*/
/**
 * @brief 发送数据帧
 * @param hcan CAN编号
 * @param ID ID
 * @param Data 被发送的数据指针
 * @param Length 长度
 * @return uint8_t 执行状态 
 */ 
uint8_t CAN_Send_Data(CAN_HandleTypeDef *hcan, uint16_t ID, uint8_t *Data, uint16_t Length);

#endif