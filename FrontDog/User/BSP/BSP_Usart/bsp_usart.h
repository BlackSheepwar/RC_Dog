/**
 * @file bsp_usart.h
 * @brief STM32 USART DMA+IDLE 接收驱动
 *        支持多串口接收与编号管理，使用环形队列缓存接收数据
 * @author 李嘉图
 * @date 2026-5-4
 */

#ifndef __BSP_USART_H__
#define __BSP_USART_H__

/*==============================================================================
 * 头文件包含
 *============================================================================*/
#include <usart.h>

/*==============================================================================
 * 类型前向声明（避免在.h中包含HAL头文件）
 *============================================================================*/

/*==============================================================================
 * 宏定义与常量
 *============================================================================*/
#define USART_MAX_PORTS     2           // 支持最大串口数量
#define USART_BUF_SIZE      128         // 数据接收缓冲区大小
#define UART_FIFO_SIZE      256         // 环形队列缓存长度

/*==============================================================================
 * 结构体定义
 *============================================================================*/
/**
 * @brief BSP层串口端口的内部数据结构
 */
typedef struct {
    uint8_t             id;             // 串口编号，由注册时指定
    uint8_t             data_ready;     // 数据准备标志，1 表示有新数据
    uint16_t            rx_size;        // 本次DMA接收到的数据长度
    UART_HandleTypeDef  *huart;         // UART 硬件句柄
    uint8_t             dma_buf[USART_BUF_SIZE]; // DMA 接收缓冲区
    uint8_t             fifo[UART_FIFO_SIZE]; // 环形队列缓存
    uint16_t            fifo_head;     // FIFO 写入指针
    uint16_t            fifo_tail;     // FIFO 读取指针
    uint8_t             tx_busy;       // 发送忙标志位 0表示空闲，1表示发送中
} BSP_USART_t;

/*==============================================================================
 * 初始化函数
 *============================================================================*/
/**
 * @brief 初始化 BSP 层（仅初始化内部状态，注册端口时启动 DMA）
 */
void BSP_USART_Init(void);

/**
 * @brief 注册一个 BSP 串口实例
 * @note 初始化 FIFO 和 DMA 接收，使用结构体内部的 DMA 缓冲区
 * @param id        串口编号（逻辑编号，不要求等于数组下标）
 * @param huart     UART 硬件句柄（DMA句柄从 huart->hdmarx 自动获取）
 * @retval 1: 注册成功
 * @retval 0: 注册失败
 */
uint8_t BSP_USART_RegisterPort(uint8_t id, UART_HandleTypeDef *huart);

/*==============================================================================
 * 发送函数
 *============================================================================*/
/**
 * @brief BSP 层发送数据接口
 * @param id   串口编号
 * @param buf  数据缓冲区指针
 * @param len  数据长度
 * @note 使用 DMA 发送数据，非阻塞
 */
void BSP_USART_Send(uint8_t id, uint8_t *buf, uint16_t len);

/**
 * @brief 获取UART发送忙状态
 * @retval 1 忙
 * @retval 0 空闲
 */
uint8_t BSP_USART_ReadTxBusy(uint8_t id);

/**
 * @brief 设置UART发送忙状态为1，表示发送中
 */
void BSP_USART_WriteTxBusy(uint8_t id);

/**
 * @brief 设置UART发送忙状态为0，表示发送完成
 */
void BSP_USART_WriteTxBusyFree(uint8_t id);

/*==============================================================================
 * 接收函数
 *============================================================================*/
/**
 * @brief 读取 FIFO 中的数据
 * @param id        串口编号
 * @param buf       输出缓冲区指针
 * @param buf_size  输出缓冲区大小
 * @retval uint16_t 实际读取字节数，0 表示 FIFO 空
 * @note 数据按 FIFO 顺序读取，读完后更新尾指针
 */
uint16_t BSP_USART_ReadRawData(uint8_t id, uint8_t *buf, uint16_t buf_size);

#endif
