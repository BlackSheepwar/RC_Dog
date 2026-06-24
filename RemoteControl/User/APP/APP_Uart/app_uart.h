/**
 * @file app_uart.h
 * @brief 串口应用模块
 * @author 李嘉图
 * @date 2026-06-20
 *
 * @note 架构调整（2026-06-20）：
 *       1. BSP 层采用 CAN 风格的编译期映射表，不再持有缓冲
 *       2. DMA 循环缓冲区由 APP 层分配并管理位置追踪
 *       3. 中断只通知不拷贝，数据在任务上下文中读出
 *       4. 接收使用共享数据池 + 描述符 FIFO，替代大结构体数组
 *       5. 发送单缓冲 + RTOS 消息队列，TX 完成信号驱动
 */

#ifndef __APP_UART_H__
#define __APP_UART_H__

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include "bsp_uart.h"
#include "codec.h"

/*==============================================================================
 * 宏定义与常量
 *============================================================================*/
#define APP_RX_DMA_BUF_SIZE     512         // DMA 循环接收缓冲区大小（每串口）
#define APP_RX_BUF_MAX          256         // 拼包缓冲区大小
#define APP_RX_POOL_SIZE       1024         // 共享接收数据池大小
#define APP_RX_DESC_MAX          20         // 每串口描述符 FIFO 深度
#define APP_RX_TIMEOUT_MS        50         // 半包超时（ms）

#define APP_TX_BUF_SIZE        128          // 发送帧最大长度

/*==============================================================================
 * 结构体定义
 *============================================================================*/
/**
 * @brief 接收数据池描述符
 * @note 指向共享池中的一段有效数据，代替原来直接存 Codec_Packet_t
 *       仅 6 字节，大幅节约 FIFO 内存
 */
typedef struct {
    uint16_t offset;        // 在共享池中的偏移
    uint8_t  len;           // payload 长度
    uint8_t  cmd;           // 命令字
    uint8_t  id;            // 来源串口 ID
} APP_RxDesc_t;

/**
 * @brief 描述符 FIFO（每串口一个）
 * @note head = 读位置, tail = 写位置
 *       空: head == tail
 *       满: (tail + 1) % APP_RX_DESC_MAX == head
 */
typedef struct {
    APP_RxDesc_t buffer[APP_RX_DESC_MAX];
    uint8_t head;
    uint8_t tail;
} APP_RxDescFIFO_t;

/**
 * @brief 发送帧结构（通过 RTOS 消息队列传递）
 */
typedef struct {
    uint8_t id;                         // 目标串口 ID
    uint8_t len;                        // 帧长度
    uint8_t data[APP_TX_BUF_SIZE];      // 编码后的完整帧
} APP_TxFrame_t;

/**
 * @brief 串口端口上下文
 * @note 每个串口一个实例，包含 DMA 缓冲、拼包缓冲、位置追踪
 */
typedef struct {
    uint8_t  id;                            // 串口编号（与 BSP 一致）
    /* --- DMA 循环接收 --- */
    uint8_t  rx_dma_buf[APP_RX_DMA_BUF_SIZE]; // DMA 循环缓冲区（APP 所有）
    uint16_t rx_last_pos;                   // 上次读到的 DMA 位置
    /* --- 拼包缓冲 --- */
    uint8_t  rx_buf[APP_RX_BUF_MAX];        // 拼包缓冲区
    uint16_t rx_len;                        // 当前有效数据长度
    uint32_t last_rx_time;                  // 最后接收时间戳
} APP_UART_Port_t;

/*==============================================================================
 * 初始化函数
 *============================================================================*/
/**
 * @brief APP 层初始化
 * @note 初始化 BSP、所有端口上下文，配置 DMA 循环接收。
 *       端口 ID 列表在 app_uart.c 内部编译期常量，无需外部注册。
 */
void APP_UART_Init(void);

/*==============================================================================
 * 接收函数
 *============================================================================*/
/**
 * @brief 从 DMA 循环缓冲读取新数据并解析
 * @param id 串口编号
 * @note 由 RX 任务在收到消息后调用。
 *       读取 DMA 新数据 → 拼入 rx_buf → Codec 解析
 *       → 完整包写入共享池 → 描述符入 FIFO
 */
void APP_UART_ProcessRxData(uint8_t id);

/**
 * @brief 处理一个接收数据包
 * @retval 1: 成功处理一个包
 * @retval 0: 无包可处理
 * @note 从描述符 FIFO 弹出一个包，从共享池中读取数据，调用命令处理
 */
uint8_t APP_UART_SendRxPacket(void);

/*==============================================================================
 * 发送函数
 *============================================================================*/
/**
 * @brief 打包并推入发送队列
 * @param id   目标串口 ID
 * @param cmd  命令字
 * @param data 数据内容（len==0 时可为 NULL）
 * @param len  数据长度
 * @retval 1: 成功入队
 * @retval 0: 失败（队列满/参数错误）
 */
uint8_t APP_UART_BuildTxPacket(uint8_t id, uint8_t cmd, const uint8_t *data, uint8_t len);

/**
 * @brief 发送一帧数据（由 TX 任务调用）
 * @param frame 已编码的帧（含端口 ID、数据、长度）
 * @note 封装 BSP_SendDMA，保持分层清晰。
 *       TX 任务不要直接调用 BSP 层函数。
 */
void APP_UART_SendFrame(const APP_TxFrame_t *frame);

/*==============================================================================
 * 双缓冲发送接口
 *============================================================================*/
/**
 * @brief 双缓冲发送一帧（由 TX 任务调用）
 * @param frame 待发送帧
 * @note 若 DMA 空闲则立即发，若忙但另一缓冲空闲则预装，
 *       若双缓冲均忙则放回队列末尾。
 */
void APP_UART_TrySendDual(const APP_TxFrame_t *frame);

/**
 * @brief TX 完成处理（由 TX 任务在收到标记帧后调用）
 * @param id 已完成发送的串口 ID
 * @note 尝试从队列取帧装填刚释放的缓冲。
 */
void APP_UART_OnTxComplete(uint8_t id);

#endif /* __APP_UART_H__ */
