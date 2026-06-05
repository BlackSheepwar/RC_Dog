/**
 * @file app_usart.h
 * @brief 串口应用模块
 * @author 李嘉图
 * @date 2026-5-4
 */

#ifndef __APP_USART_H__
#define __APP_USART_H__

/*==============================================================================
 * 头文件包含
 *============================================================================*/
#include "bsp_usart.h"
#include "codec.h"

/*==============================================================================
 * 宏定义与常量
 *============================================================================*/
#define RX_BUF_MAX     256          // 解包缓冲区大小
#define PACKET_FIFO_SIZE 11         // 最多存 10 个未处理包(要保留一个空包)
#define USART_RX_TIMEOUT_MS   50    // 半包超时时间（单位：ms）

/*==============================================================================
 * 结构体
 *============================================================================*/
// FIFO 队列结构体，用于存储 Packet_t 类型的数据
typedef struct {
    Codec_Packet_t buffer[PACKET_FIFO_SIZE];  // 队列缓冲区
    uint8_t  head;                      // 队头索引（读位置）
    uint8_t  tail;                      // 队尾索引（写位置）
} APP_USART_PacketFIFO_t;

// 串口解包结构体
typedef struct
{
    uint8_t  id;                    // 串口编号（与 BSP 层一致）
    uint8_t  rx_buf[RX_BUF_MAX];    // 拼包缓冲区
    uint16_t rx_len;                // 当前缓冲区中有效数据长度
    uint32_t last_rx_time;          // 最后一次接收到数据的时间戳
} APP_USART_t;

/*==============================================================================
 * 初始化函数
 *============================================================================*/
/**
 * @brief APP初始化
 * @note 串口注册
 */
void APP_USART_Init(void);

/**
 * @brief 注册一个 APP 串口解析实例
 * @param id    串口编号（需与 BSP 层一致）
 * @param huart UART 硬件句柄（DMA句柄从 huart->hdmarx 自动获取）
 * @retval 1：注册成功
 * @retval 0：注册失败
 */
uint8_t APP_USART_RegisterPort(uint8_t id, UART_HandleTypeDef *huart);


/*==============================================================================
 * 消费数据包函数
 *============================================================================*/
/**
 * @brief 数据包处理
 * @note 每次调用仅处理队列头的一个数据包
 * @retval 1：成功消费一个数据包
 * @retval 0：没有数据包可消费
 */
uint8_t APP_USART_SendRxPacket(void);

/**
 * @brief 解包并推入接收 FIFO 队列（基于单包解析器）
 * @param id 串口编号
 */
void APP_USART_BuildRxPacket(uint8_t id);

/*==============================================================================
 * 发送数据包函数
 *============================================================================*/
/**
 * @brief 发送 FIFO 调度处理（每次发送一个数据包）
 * @note  从 txPacketFIFO 中取出一个包，组装成完整帧并通过底层发送
 */
void APP_USART_SendTxPacket(void);

/**
 * @brief 打包并推入发送 FIFO 队列
 * @param id   来源/目标编号
 * @param cmd  命令字
 * @param data 数据内容指针（len == 0 时可为 NULL）
 * @param len  数据长度
 * @retval 1：成功打包并入队
 * @retval 0：失败（参数非法、队列满等）
 */
uint8_t APP_USART_BuildTxPacket(uint8_t id, uint8_t cmd, const uint8_t *data, uint8_t len);


#endif
