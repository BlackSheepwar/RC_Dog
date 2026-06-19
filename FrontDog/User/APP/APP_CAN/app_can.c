/**
 * @file app_can.c
 * @brief CAN应用层实现
 * @author 李嘉图
 * @date 2026-06-01
 *
 * @note 在BSP_CAN基础上提供应用层封装。
 *       硬件映射为编译期常量，无需运行时注册。
 *       每端口配备发送缓冲（环形队列），高频发送时优先走硬件邮箱，
 *       邮箱满则暂存缓冲，由 TX 完成中断自动排空。
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include <string.h>
#include "main.h"
#include "app_can.h"
// 功能包含
#include "can.h"            // HAL CAN 类型（用于 TX 完成回调）

/*==============================================================================
 * 发送缓冲（环形队列）
 *
 * CAN 硬件只有 3 个发送邮箱，高频场景下容易占满。
 * 软件缓冲缓存待发帧，由 TX 完成中断驱动排空。
 *============================================================================*/
/**
 * @brief 发送帧缓冲区结构
 */
typedef struct {
    uint32_t can_id;                    // 标准/扩展ID值
    uint8_t  is_extid;                  // ID类型: BSP_CAN_STDID / BSP_CAN_EXTID
    uint8_t  is_remote;                 // 帧类型: BSP_CAN_DATA_FRAME / BSP_CAN_REMOTE_FRAME
    uint8_t  data[BSP_CAN_DATA_LEN];    // 数据拷贝（8字节）
    uint8_t  len;                       // 数据长度
} APP_CAN_TxFrame_t;

/**
 * @brief 发送缓冲环形队列
 */
typedef struct {
    APP_CAN_TxFrame_t frames[APP_CAN_TX_BUF_SIZE];
    volatile uint8_t head;              // 写入位置（生产者）
    volatile uint8_t tail;              // 读取位置（消费者）
    volatile uint8_t count;             // 待发帧数
} APP_CAN_TxBuf_t;

/** @brief 每端口独立发送缓冲 */
static APP_CAN_TxBuf_t app_can_tx_buf[BSP_CAN_MAX_NUM];

/*==============================================================================
 * 发送缓冲内部操作
 *============================================================================*/
/**
 * @brief 入队一帧到发送缓冲
 * @param buf  目标缓冲指针
 * @param frame 帧数据
 * @retval 1: 入队成功
 * @retval 0: 缓冲已满
 */
static uint8_t APP_CAN_TxBuf_Enqueue(APP_CAN_TxBuf_t *buf, const APP_CAN_TxFrame_t *frame)
{
    if (buf->count >= APP_CAN_TX_BUF_SIZE)
        return 0;

    buf->frames[buf->head] = *frame;
    buf->head = (buf->head + 1) % APP_CAN_TX_BUF_SIZE;
    buf->count++;
    return 1;
}

/**
 * @brief 出队一帧
 * @param buf   目标缓冲指针
 * @param frame 输出：帧数据
 * @retval 1: 出队成功
 * @retval 0: 缓冲为空
 */
static uint8_t APP_CAN_TxBuf_Dequeue(APP_CAN_TxBuf_t *buf, APP_CAN_TxFrame_t *frame)
{
    if (buf->count == 0)
        return 0;

    *frame = buf->frames[buf->tail];
    buf->tail = (buf->tail + 1) % APP_CAN_TX_BUF_SIZE;
    buf->count--;
    return 1;
}

/**
 * @brief 排空指定端口的发送缓冲
 * @param id 端口ID
 *
 * @note 不断出队并调用 BSP_CAN_SendMsg，直到缓冲为空或硬件邮箱满。
 */
void APP_CAN_FlushTxBuf(uint8_t id)
{
    APP_CAN_TxBuf_t    *buf = &app_can_tx_buf[id];
    APP_CAN_TxFrame_t   frame;

    while (APP_CAN_TxBuf_Dequeue(buf, &frame))
    {
        if (!BSP_CAN_SendMsg(id, frame.can_id, frame.is_extid,
                             frame.is_remote, frame.data, frame.len))
        {
            /* 硬件邮箱满，重新入队（放回队首） */
            buf->tail = (buf->tail == 0)
                        ? (APP_CAN_TX_BUF_SIZE - 1)
                        : (buf->tail - 1);
            buf->count++;
            break;
        }
    }
}

/*==============================================================================
 * 初始化函数
 *============================================================================*/
/**
 * @brief CAN应用层初始化
 * @note 初始化 BSP 层（消耗信号量初始计数、复位启动状态），清空发送缓冲。
 */
void APP_CAN_Init(void)
{
    BSP_CAN_Init();
    memset(app_can_tx_buf, 0, sizeof(app_can_tx_buf));
}

/*==============================================================================
 * 滤波器配置
 *============================================================================*/
/**
 * @brief 配置CAN滤波器
 * @param id  端口ID（0=CAN1, 1=CAN2）
 * @param cfg 滤波器配置参数
 * @retval 1: 配置成功
 * @retval 0: 配置失败（ID无效/cfg为空）
 *
 * @note 委托 BSP_CAN_FilterConfig 完成硬件配置
 */
uint8_t APP_CAN_FilterConfig(uint8_t id, const BSP_CAN_FilterConfig_t *cfg)
{
    return BSP_CAN_FilterConfig(id, cfg);
}

/*==============================================================================
 * 启动函数
 *============================================================================*/
/**
 * @brief 启动CAN通信
 * @param id 端口ID（0=CAN1, 1=CAN2）
 * @retval 1: 启动成功
 * @retval 0: 启动失败
 *
 * @note 委托 BSP_CAN_Start 启动硬件并使能FIFO中断
 */
uint8_t APP_CAN_Start(uint8_t id)
{
    return BSP_CAN_Start(id);
}

/*==============================================================================
 * 发送函数
 *============================================================================*/
/**
 * @brief 发送CAN帧（支持标准/扩展 × 数据/遥控）
 * @param id       端口ID
 * @param can_id   ID值
 * @param is_extid ID类型
 * @param is_remote 帧类型
 * @param data     数据指针
 * @param len      数据长度
 * @retval 1: 入队成功（由 TX 服务任务异步发送）
 * @retval 0: 发送失败（参数错误或发送缓冲已满）
 *
 * @note 所有帧统一入队缓冲管理，由 TX 服务任务异步排空。
 *       保证发送顺序与调用顺序一致（FIFO）。
 *       入队后释放 CAN_TX_BSHandle 信号量，唤醒 Task_CAN_TX 开始发送。
 *       后续由 TX 完成中断持续驱动排空，形成完整的生产者-消费者链。
 */
uint8_t APP_CAN_SendMsg(uint8_t id, uint32_t can_id, uint8_t is_extid, uint8_t is_remote, uint8_t *data, uint8_t len)
{
    /* ---------- 参数检查 ---------- */
    if (id >= BSP_CAN_MAX_NUM) return 0;
    if (len > BSP_CAN_DATA_LEN) return 0;
    if (!is_remote && !data) return 0;

    /* ---------- 组装帧，入队缓冲 ---------- */
    APP_CAN_TxFrame_t frame;
    frame.can_id    = can_id;
    frame.is_extid  = is_extid;
    frame.is_remote = is_remote;
    frame.len       = len;
    if (data)
        memcpy(frame.data, data, len);
    else
        memset(frame.data, 0, sizeof(frame.data));

    if (!APP_CAN_TxBuf_Enqueue(&app_can_tx_buf[id], &frame))
        return 0;                               // 缓冲已满

    /* 触发 TX 任务开始发送（首次入队或 TX 任务空闲时唤醒） */
    osSemaphoreRelease(CAN_TX_BSHandle);

    return 1;
}
