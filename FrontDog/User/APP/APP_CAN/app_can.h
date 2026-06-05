/**
 * @file app_can.h
 * @brief CAN应用层
 * @author 李嘉图
 * @date 2026-06-01
 *
 * @note 在BSP_CAN基础上封装，提供应用层CAN管理。
 *       采用静态资源池+注册模式，与app_key/app_knob风格一致。
 *       支持双CAN实例。APP_CAN_Register 内部调用 BSP_CAN_Register 完成底层注册。
 */

#ifndef __APP_CAN_H__
#define __APP_CAN_H__

/*==============================================================================
 * 头文件包含
 *============================================================================*/
#include "bsp_can.h"

/*==============================================================================
 * 结构体定义
 *============================================================================*/
/**
 * @brief APP层CAN端口结构
 */
typedef struct {
    uint8_t  id;                    // 端口ID（与BSP层一致）
} APP_CAN_t;

/*==============================================================================
 * 初始化函数
 *============================================================================*/
/**
 * @brief CAN应用层初始化
 * @note 内部调用 BSP_CAN_Init 初始化底层，清空APP资源池
 */
void APP_CAN_Init(void);

/**
 * @brief 注册一个CAN端口
 * @param id   端口ID（0=CAN1, 1=CAN2）
 * @param hcan CAN硬件句柄
 * @retval 1: 注册成功
 * @retval 0: 注册失败（参数无效/重复注册/资源池已满/BSP注册失败）
 *
 * @note 内部自动调用 BSP_CAN_Register 完成底层注册
 */
uint8_t APP_CAN_Register(uint8_t id, CAN_HandleTypeDef *hcan);

/*==============================================================================
 * 滤波器配置
 *============================================================================*/
/**
 * @brief 配置CAN滤波器
 * @param id 端口ID
 * @param cfg 滤波器配置参数（支持规则组/通道组 × 标准帧/扩展帧 共4种模式）
 * @retval 1: 配置成功
 * @retval 0: 配置失败
 */
uint8_t APP_CAN_FilterConfig(uint8_t id, BSP_CAN_FilterConfig_t *cfg);

/*==============================================================================
 * 启动函数
 *============================================================================*/
/**
 * @brief 启动CAN通信
 * @param id 端口ID
 * @retval 1: 启动成功
 * @retval 0: 启动失败
 *
 * @note 内部调用 BSP_CAN_Start，使能FIFO中断
 */
uint8_t APP_CAN_Start(uint8_t id);

/*==============================================================================
 * 发送函数
 *============================================================================*/
/**
 * @brief 发送CAN帧（支持标准/扩展 × 数据/遥控）
 * @param id       端口ID
 * @param can_id   ID值（标准帧11bit / 扩展帧29bit）
 * @param is_extid ID类型: BSP_CAN_STDID / BSP_CAN_EXTID
 * @param is_remote 帧类型: BSP_CAN_DATA_FRAME / BSP_CAN_REMOTE_FRAME
 * @param data     数据指针（遥控帧传NULL）
 * @param len      数据长度 (0~8)
 * @retval 1: 发送成功
 * @retval 0: 发送失败
 */
uint8_t APP_CAN_SendMsg(uint8_t id, uint32_t can_id, uint8_t is_extid, uint8_t is_remote, uint8_t *data, uint8_t len);

#endif
