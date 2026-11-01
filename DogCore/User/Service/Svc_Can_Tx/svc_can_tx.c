/**
 * @file svc_can_tx.c
 * @brief CAN 发送服务实现：环形缓冲与排空
 * @author Ricardo
 * @date 2026-11-01
 *
 * @note 环形缓冲用 head/tail/count 三个下标，而不是"空一个位置区分满空"：
 *       count 让"满"和"空"都能 O(1) 判断，也让上层能直接查到排队深度。
 *       队列满时丢新帧（而不是覆盖旧帧）——控制帧宁可少一拍，
 *       也不要打乱先后顺序。
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include <stddef.h>
#include "svc_can_tx.h"

/*==============================================================================
 * 接口实现
 *============================================================================*/
/**
 * @brief 清空发送缓冲
 * @param tx 发送缓冲，可为 NULL（安全返回）
 * @retval 无
 */
void Svc_Can_Tx_Init(svc_can_tx_t *tx)
{
    if (tx == NULL) {
        return;
    }

    tx->head    = 0u;
    tx->tail    = 0u;
    tx->count   = 0u;
    tx->dropped = 0u;
}

/**
 * @brief 把一帧放进发送缓冲
 * @param tx    发送缓冲，不可为 NULL
 * @param frame 待发帧，不可为 NULL
 * @retval 0: 已入队
 * @retval -1: 参数非法
 * @retval -2: 缓冲已满
 */
int Svc_Can_Tx_Push(svc_can_tx_t *tx, const bsp_can_frame_t *frame)
{
    if ((tx == NULL) || (frame == NULL)) {
        return -1;
    }

    if (tx->count >= SVC_CAN_TX_DEPTH) {
        tx->dropped++;
        return -2;
    }

    tx->frames[tx->head] = *frame;
    tx->head = (uint8_t)((tx->head + 1u) % SVC_CAN_TX_DEPTH);
    tx->count++;

    return 0;
}

/**
 * @brief 尽量把缓冲里的帧塞进硬件邮箱
 * @param tx  发送缓冲，可为 NULL
 * @param can 总线句柄，可为 NULL
 * @retval 本次成功发出的帧数
 */
uint8_t Svc_Can_Tx_Flush(svc_can_tx_t *tx, const bsp_can_t *can)
{
    uint8_t sent = 0u;

    if ((tx == NULL) || (can == NULL)) {
        return 0u;
    }

    while (tx->count > 0u) {
        /* ---------- 1. 尝试发出队首这一帧 ---------- */
        if (BSP_CAN_Send(can, &tx->frames[tx->tail]) != 0) {
            /* 硬件邮箱满：保留队首，下一轮重试，顺序不变 */
            break;
        }

        /* ---------- 2. 发出成功才推进读指针 ---------- */
        tx->tail = (uint8_t)((tx->tail + 1u) % SVC_CAN_TX_DEPTH);
        tx->count--;
        sent++;
    }

    return sent;
}

/**
 * @brief 查询当前排队帧数
 * @param tx 发送缓冲，可为 NULL
 * @retval 排队帧数
 */
uint8_t Svc_Can_Tx_Count(const svc_can_tx_t *tx)
{
    return (tx != NULL) ? tx->count : 0u;
}

/**
 * @brief 查询累计丢弃帧数
 * @param tx 发送缓冲，可为 NULL
 * @retval 累计丢弃帧数
 */
uint32_t Svc_Can_Tx_Dropped(const svc_can_tx_t *tx)
{
    return (tx != NULL) ? tx->dropped : 0u;
}
