/**
 * @file bsp_can.c
 * @brief CAN（FDCAN）硬件驱动实现：句柄 → hfdcan，帧 → HAL 结构体
 * @author Ricardo
 * @date 2026-11-01
 *
 * @note 工程里只有一路 FDCAN，所以句柄表只有 bsp_can1；
 *       加第二路时新增一个句柄实例，函数一行不用改。
 *
 *       中断里只做"取一帧、投队列"：不解析、不处理、不打印。
 *       队列由 freertos.c 创建，满了就丢——丢帧计数留给上层统计，
 *       ISR 里不做任何可能阻塞的操作。
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include <stddef.h>
#include "main.h"               /* HAL 与 hfdcan1 */
#include "bsp_can.h"
// 功能包含
#include "cmsis_os.h"           /* 中断里投递消息队列 */
#include "fdcan.h"              /* FDCAN_HandleTypeDef 与 hfdcan1 */

/*==============================================================================
 * 类型定义
 *============================================================================*/
/**
 * @brief CAN 总线句柄（一路一份）
 * @note hal 用 void * 存，FDCAN_HandleTypeDef * 的转换只在本文件出现。
 */
struct bsp_can {
    void *hal;          /* FDCAN 句柄（本文件内还原成 FDCAN_HandleTypeDef *） */
};

/*==============================================================================
 * 句柄实例
 *============================================================================*/
/** @brief FDCAN1 总线（唯一的 CAN 口） */
const bsp_can_t bsp_can1 = { &hfdcan1 };

/*==============================================================================
 * 内部变量
 *============================================================================*/
/** @brief 启动标志（运行时状态，不进 const 句柄） */
static uint8_t s_can_started = 0u;

/*==============================================================================
 * 内部函数
 *============================================================================*/
/**
 * @brief 把 BSP 帧类型翻译成 HAL 帧头类型
 * @param frame 待发帧，不可为 NULL
 * @param header 输出 HAL 帧头，不可为 NULL
 * @retval 无
 */
static void BSP_CAN_Fill_Tx_Header(const bsp_can_frame_t *frame,
                                   FDCAN_TxHeaderTypeDef *header)
{
    header->Identifier          = frame->id;
    header->IdType              = (frame->ide == (uint8_t)BSP_CAN_ID_EXT)
                                  ? FDCAN_EXTENDED_ID : FDCAN_STANDARD_ID;
    header->TxFrameType         = (frame->rtr == (uint8_t)BSP_CAN_FRAME_REMOTE)
                                  ? FDCAN_REMOTE_FRAME : FDCAN_DATA_FRAME;
    header->DataLength          = frame->dlc;
    header->ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    header->BitRateSwitch       = FDCAN_BRS_OFF;
    header->FDFormat            = FDCAN_CLASSIC_CAN;
    header->TxEventFifoControl  = FDCAN_NO_TX_EVENTS;
    header->MessageMarker       = 0u;
}

/**
 * @brief 从指定 FIFO 取一帧并投进对应的消息队列
 * @param hfdcan 触发中断的 HAL 句柄
 * @param fifo   取帧的 FIFO，取 bsp_can_fifo_t
 * @param queue 目标队列句柄（由 freertos.c 创建）
 * @retval 无
 * @note 只在中断上下文调用。
 */
static void BSP_CAN_Rx_To_Queue(FDCAN_HandleTypeDef *hfdcan, uint8_t fifo,
                                osMessageQueueId_t queue)
{
    bsp_can_frame_t        frame;
    FDCAN_RxHeaderTypeDef  header;
    uint8_t                data[BSP_CAN_DATA_LEN] = {0};

    if ((hfdcan == NULL) || (queue == NULL)) {
        return;
    }

    if (fifo == (uint8_t)BSP_CAN_FIFO0) {
        if (HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO0, &header, data) != HAL_OK) {
            return;
        }
    } else {
        if (HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO1, &header, data) != HAL_OK) {
            return;
        }
    }

    frame.id  = header.Identifier;
    frame.ide = (header.IdType == FDCAN_EXTENDED_ID) ? (uint8_t)BSP_CAN_ID_EXT
                                                     : (uint8_t)BSP_CAN_ID_STD;
    frame.rtr = (header.RxFrameType == FDCAN_REMOTE_FRAME) ? (uint8_t)BSP_CAN_FRAME_REMOTE
                                                           : (uint8_t)BSP_CAN_FRAME_DATA;
    frame.dlc = (uint8_t)header.DataLength;

    for (uint8_t i = 0u; i < BSP_CAN_DATA_LEN; i++) {
        frame.data[i] = data[i];
    }

    /* 队列满就丢这一帧：ISR 里不允许等待，也不允许重试 */
    (void)osMessageQueuePut(queue, &frame, 0u, 0u);
}

/*==============================================================================
 * 初始化与启停
 *============================================================================*/
/**
 * @brief 复位启动标志（不碰硬件）
 * @retval 0: 成功
 */
int BSP_CAN_Init(void)
{
    s_can_started = 0u;
    return 0;
}

/**
 * @brief 启动总线并使能 FIFO0 / FIFO1 接收中断
 * @param can 总线句柄，不可为 NULL
 * @retval 0: 已启动
 * @retval -1: 句柄为 NULL 或 HAL 启动失败
 */
int BSP_CAN_Start(const bsp_can_t *can)
{
    FDCAN_HandleTypeDef *hfdcan;

    if (can == NULL) {
        return -1;
    }

    if (s_can_started != 0u) {
        return 0;
    }

    hfdcan = (FDCAN_HandleTypeDef *)can->hal;
    if (hfdcan == NULL) {
        return -1;
    }

    if (HAL_FDCAN_Start(hfdcan) != HAL_OK) {
        return -1;
    }

    if (HAL_FDCAN_ActivateNotification(hfdcan,
            FDCAN_IT_RX_FIFO0_NEW_MESSAGE | FDCAN_IT_RX_FIFO1_NEW_MESSAGE,
            0u) != HAL_OK) {
        return -1;
    }

    s_can_started = 1u;
    return 0;
}

/*==============================================================================
 * 滤波器
 *============================================================================*/
/**
 * @brief 配置一条滤波器规则
 * @param can 总线句柄，不可为 NULL
 * @param cfg 规则，不可为 NULL
 * @retval 0: 配置成功
 * @retval -1: 参数非法
 * @retval -2: HAL 配置失败
 */
int BSP_CAN_Set_Filter(const bsp_can_t *can, const bsp_can_filter_t *cfg)
{
    FDCAN_HandleTypeDef *hfdcan;
    FDCAN_FilterTypeDef  filter;
    uint32_t             id_max;

    if ((can == NULL) || (cfg == NULL)) {
        return -1;
    }

    if (cfg->index > 127u) {
        return -1;
    }

    id_max = (cfg->ide == (uint8_t)BSP_CAN_ID_EXT) ? BSP_CAN_EXT_ID_MAX
                                                   : BSP_CAN_STD_ID_MAX;
    if ((cfg->id1 > id_max) || (cfg->id2 > id_max)) {
        return -1;
    }

    hfdcan = (FDCAN_HandleTypeDef *)can->hal;
    if (hfdcan == NULL) {
        return -1;
    }

    /* ---------- 组装 HAL 滤波器 ---------- */
    filter.IdType       = (cfg->ide == (uint8_t)BSP_CAN_ID_EXT)
                          ? FDCAN_EXTENDED_ID : FDCAN_STANDARD_ID;
    filter.FilterIndex  = cfg->index;
    filter.FilterConfig = (cfg->fifo == (uint8_t)BSP_CAN_FIFO0)
                          ? FDCAN_FILTER_TO_RXFIFO0 : FDCAN_FILTER_TO_RXFIFO1;
    filter.FilterID1    = cfg->id1;
    filter.FilterID2    = cfg->id2;

    switch (cfg->mode) {
        case BSP_CAN_FILTER_RANGE:
            filter.FilterType = FDCAN_FILTER_RANGE;
            break;
        case BSP_CAN_FILTER_DUAL:
            filter.FilterType = FDCAN_FILTER_DUAL;
            break;
        case BSP_CAN_FILTER_MASK:
        default:
            filter.FilterType = FDCAN_FILTER_MASK;
            break;
    }

    if (HAL_FDCAN_ConfigFilter(hfdcan, &filter) != HAL_OK) {
        return -2;
    }

    return 0;
}

/*==============================================================================
 * 收发
 *============================================================================*/
/**
 * @brief 把一帧放进硬件发送 FIFO（非阻塞）
 * @param can   总线句柄，不可为 NULL
 * @param frame 待发帧，不可为 NULL
 * @retval 0: 已进入硬件 FIFO
 * @retval -1: 参数非法
 * @retval -2: 硬件 FIFO 满或总线未启动
 */
int BSP_CAN_Send(const bsp_can_t *can, const bsp_can_frame_t *frame)
{
    FDCAN_HandleTypeDef  *hfdcan;
    FDCAN_TxHeaderTypeDef header;

    if ((can == NULL) || (frame == NULL)) {
        return -1;
    }

    if (frame->dlc > BSP_CAN_DATA_LEN) {
        return -1;
    }

    if (frame->ide == (uint8_t)BSP_CAN_ID_EXT) {
        if (frame->id > BSP_CAN_EXT_ID_MAX) {
            return -1;
        }
    } else {
        if (frame->id > BSP_CAN_STD_ID_MAX) {
            return -1;
        }
    }

    hfdcan = (FDCAN_HandleTypeDef *)can->hal;
    if (hfdcan == NULL) {
        return -1;
    }

    BSP_CAN_Fill_Tx_Header(frame, &header);

    if (HAL_FDCAN_AddMessageToTxFifoQ(hfdcan, &header,
                                      (uint8_t *)frame->data) != HAL_OK) {
        return -2;
    }

    return 0;
}

/*==============================================================================
 * 中断回调
 *
 * ISR 只搬字节：取帧 → 投队列，不做任何业务判断。
 *============================================================================*/
/**
 * @brief HAL 接收 FIFO0 中断回调
 * @param hfdcan      触发中断的 HAL 句柄
 * @param RxFifoBufferIT 未使用
 * @retval 无
 */
void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifoBufferIT)
{
    (void)RxFifoBufferIT;
    BSP_CAN_Rx_To_Queue(hfdcan, (uint8_t)BSP_CAN_FIFO0, CAN_RXF0_QHandle);
}

/**
 * @brief HAL 接收 FIFO1 中断回调
 * @param hfdcan      触发中断的 HAL 句柄
 * @param RxFifoBufferIT 未使用
 * @retval 无
 */
void HAL_FDCAN_RxFifo1Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifoBufferIT)
{
    (void)RxFifoBufferIT;
    BSP_CAN_Rx_To_Queue(hfdcan, (uint8_t)BSP_CAN_FIFO1, CAN_RXF1_QHandle);
}
