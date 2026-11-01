/**
 * @file svc_can_tx.h
 * @brief CAN 发送服务：软件环形缓冲 + 排空策略，不碰 OS、不碰中断
 * @author Ricardo
 * @date 2026-11-01
 *
 * @note 硬件只有 3 个发送邮箱，一次性连发十几帧必然丢。
 *       本模块把"排队"这件事从 APP 里拿出来：
 *         - APP 只管 Push 一帧，满了返回错误，由业务决定要不要重发；
 *         - TASK 周期调 Flush，把缓冲里的帧尽量塞进硬件邮箱；
 *         - 硬件邮箱满时停在原地，下一轮继续，顺序不乱、不丢帧。
 *
 *       为什么放在 Service：这是"与产品无关的排队策略"，
 *       换成 UART 发送缓冲也是同一套逻辑，APP 里不该有环形下标。
 *
 *       并发约定：Push 与 Flush 分属不同线程时，head 只有 Push 写、
 *       tail 只有 Flush 写，count 两边都读写——因此本项目里两者都在
 *       TASK 上下文执行（同一线程），不需要加锁。若将来把 Push 挪到
 *       中断里，必须给 count 加临界区。
 */

#ifndef __Svc_Can_Tx_H__
#define __Svc_Can_Tx_H__

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include <stdint.h>
#include "bsp_can.h"            /* 帧值类型与总线句柄 */

/*==============================================================================
 * 宏定义与常量
 *============================================================================*/
/** @brief 每个总线一条发送缓冲，深度取 16（约 13 ms 的 1200 fps 负载） */
#define SVC_CAN_TX_DEPTH    16u

/*==============================================================================
 * 类型定义
 *============================================================================*/
/**
 * @brief CAN 发送缓冲（值类型：一条总线一份）
 * @note 成员是内部实现，上层只走下面四个函数。
 */
typedef struct {
    bsp_can_frame_t frames[SVC_CAN_TX_DEPTH];   /* 帧存储 */
    uint8_t         head;                       /* 写入位置（生产者） */
    uint8_t         tail;                       /* 读出位置（消费者） */
    uint8_t         count;                      /* 当前排队帧数 */
    uint32_t        dropped;                    /* 因满而丢弃的累计帧数 */
} svc_can_tx_t;

/*==============================================================================
 * 接口
 *============================================================================*/
/**
 * @brief 清空发送缓冲
 * @param tx 发送缓冲，可为 NULL（安全返回）
 * @retval 无
 * @note 会清零丢弃计数；上电初始化时调一次。
 */
void Svc_Can_Tx_Init(svc_can_tx_t *tx);

/**
 * @brief 把一帧放进发送缓冲
 * @param tx    发送缓冲，不可为 NULL
 * @param frame 待发帧，不可为 NULL
 * @retval 0: 已入队
 * @retval -1: 参数非法
 * @retval -2: 缓冲已满（丢弃计数加一，由业务决定是否重发）
 */
int Svc_Can_Tx_Push(svc_can_tx_t *tx, const bsp_can_frame_t *frame);

/**
 * @brief 尽量把缓冲里的帧塞进硬件邮箱
 * @param tx  发送缓冲，可为 NULL
 * @param can 总线句柄，可为 NULL
 * @retval 本次成功发出的帧数（0 表示缓冲空或硬件邮箱满）
 * @note 硬件邮箱满时立刻停下，已出队的帧会放回队首，不会丢。
 *       由 TASK 周期调用，例如 1 ms 一次。
 */
uint8_t Svc_Can_Tx_Flush(svc_can_tx_t *tx, const bsp_can_t *can);

/**
 * @brief 查询当前排队帧数
 * @param tx 发送缓冲，可为 NULL
 * @retval 排队帧数；tx 为 NULL 时返回 0
 */
uint8_t Svc_Can_Tx_Count(const svc_can_tx_t *tx);

/**
 * @brief 查询累计丢弃帧数
 * @param tx 发送缓冲，可为 NULL
 * @retval 累计丢弃帧数；tx 为 NULL 时返回 0
 * @note 只增不减，用来区分"没发"和"丢了"。
 */
uint32_t Svc_Can_Tx_Dropped(const svc_can_tx_t *tx);

#endif /* __Svc_Can_Tx_H__ */
