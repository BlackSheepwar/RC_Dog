/**
 * @file app_can.h
 * @brief CAN 业务层：总线开闭、发送出口、接收分发入口
 * @author Ricardo
 * @date 2026-11-01
 *
 * @note 本层只做"业务收发口"，三件事不在这里：
 *       - 排队与不丢帧：Service/Svc_Can_Tx；排空由 TASK 周期驱动；
 *       - 帧 ID 的处理逻辑：APP_CAN_Cmd 的命令表；
 *       - 硬件细节：BSP_CAN。
 *
 *       所以本层不需要 cmsis_os.h，也不会去释放信号量唤醒谁。
 */

#ifndef __APP_CAN_H__
#define __APP_CAN_H__

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include <stdint.h>
#include "bsp_can.h"
#include "app_can_cfg.h"

/*==============================================================================
 * 宏定义与常量
 *============================================================================*/
/** @brief 本机 CAN 端口数量（当前只有 FDCAN1） */
#define APP_CAN_PORT_COUNT      1u
/** @brief 端口编号：1 = FDCAN1 */
#define APP_CAN_PORT_1          1u

/*==============================================================================
 * 初始化与开闭
 *============================================================================*/
/**
 * @brief 清空发送缓冲并复位 BSP 启动状态
 * @retval 0: 成功
 * @note 必须在创建线程前调用一次；不碰硬件。
 */
int APP_CAN_Init(void);

/**
 * @brief 按配置表配置滤波器并启动总线
 * @param port 端口编号，1 ~ APP_CAN_PORT_COUNT
 * @retval 0: 成功
 * @retval -1: 端口号非法
 * @retval -2: 某个滤波器配置失败
 * @retval -3: 总线启动失败
 * @note 滤波器来自 app_can_cfg.h 的静态表，重复调用结果一致。
 */
int APP_CAN_Open(uint8_t port);

/*==============================================================================
 * 发送
 *============================================================================*/
/**
 * @brief 组一帧并放进发送缓冲
 * @param port 端口编号
 * @param id   帧 ID
 * @param ide  ID 类型，取 bsp_can_id_t
 * @param data 数据，可为 NULL（dlc 须为 0）
 * @param dlc  数据长度 0 ~ 8
 * @retval 0: 已入队
 * @retval -1: 参数非法
 * @retval -2: 发送缓冲已满（丢弃计数加一）
 * @note 入队即返回，真正的发送由 APP_CAN_Flush 在 TASK 里完成。
 */
int APP_CAN_Send(uint8_t port, uint32_t id, uint8_t ide,
                 const uint8_t *data, uint8_t dlc);

/**
 * @brief 把各端口发送缓冲里的帧推进硬件邮箱
 * @retval 无
 * @note 由 CAN 发送线程每 1 ms 调一次；硬件邮箱满时自动留到下一轮。
 */
void APP_CAN_Flush(void);

/**
 * @brief 查询某端口的累计丢弃帧数
 * @param port 端口编号
 * @retval 累计丢弃帧数；端口非法时返回 0
 */
uint32_t APP_CAN_Dropped(uint8_t port);

#endif /* __APP_CAN_H__ */
