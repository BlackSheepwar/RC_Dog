/**
 * @file bsp_usart.c
 * @brief STM32 USART DMA+IDLE 接收驱动
 *        仅负责串口接收与缓存管理，不解析协议、不处理业务
 * @author 李嘉图
 * @date 2026-5-4
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
#include "main.h"
#include "usart.h"
#include <string.h>
#include "bsp_usart.h"

/*==============================================================================
 * 串口实例池
 *============================================================================*/
static BSP_USART_t bsp_usart_pool[USART_MAX_PORTS];
static uint8_t bsp_usart_count = 0;

/*==============================================================================
 * 内部函数
 *============================================================================*/
/**
 * @brief 根据串口 ID 查找 BSP 串口实例
 * @param id 串口编号
 * @retval 非NULL：返回对应串口实例结构体指针
 * @retval NULL：未找到
 */
static BSP_USART_t *BSP_USART_GetById(uint8_t id)
{
    for (uint8_t i = 0; i < bsp_usart_count; i++)
    {
        if (bsp_usart_pool[i].id == id)
        {
            return &bsp_usart_pool[i];
        }
    }
    return NULL;
}

/**
 * @brief 根据串口句柄 查找 BSP 串口实例
 * @param huart 串口句柄指针
 * @retval 非NULL：返回对应串口实例结构体指针
 * @retval NULL：未找到
 */
static BSP_USART_t *BSP_USART_GetHuartById(UART_HandleTypeDef *huart)
{
    for (uint8_t i = 0; i < bsp_usart_count; i++)
    {
        if (bsp_usart_pool[i].huart == huart)
            return &bsp_usart_pool[i];
    }
    return NULL;
}

/**
 * @brief 重启 DMA 接收
 * @param port 串口实例指针
 * @note 每次接收完成后调用，保证 DMA 可以接收新数据
 */
static void BSP_USART_restart_dma(BSP_USART_t *port)
{
    HAL_UARTEx_ReceiveToIdle_DMA(port->huart, port->dma_buf, USART_BUF_SIZE);
    __HAL_DMA_DISABLE_IT(port->hdma_rx, DMA_IT_HT);
}

/*==============================================================================
 * FIFO队列函数
 *============================================================================*/
/**
 * @brief 将DMA接收的数据入队到软件FIFO
 * @param port 串口设备端口指针
 * @note 此函数负责将数据从DMA缓冲区安全地拷贝到环形FIFO中。
 *       它从port->rx_size获取数据长度，并在处理完毕后将其清零。
 */
static void BSP_USART_enqueue_dma_data_to_fifo(BSP_USART_t *port)
{
    if (port->rx_size == 0)
    {
        return;
    }

    /* 计算环形队列剩余空间 */
    uint16_t fifo_free = (port->fifo_head >= port->fifo_tail)
                           ? (UART_FIFO_SIZE - (port->fifo_head - port->fifo_tail) - 1)
                           : (port->fifo_tail - port->fifo_head - 1);

    /* 如果空间不足，丢弃数据（或根据需求采取其他策略） */
    uint16_t write_len = (port->rx_size <= fifo_free) ? port->rx_size : fifo_free;

    if (write_len == 0)
    {
        port->rx_size = 0; // 清除长度，表示数据已“处理”
        return;
    }

    /* 计算从写指针到环形队列末尾的连续空间大小 */
    uint16_t first_chunk_len = UART_FIFO_SIZE - port->fifo_head;

    /* 如果连续空间足以存放所有数据 */
    if (first_chunk_len >= write_len)
    {
        memcpy(&port->fifo[port->fifo_head], port->dma_buf, write_len);
    }
    else /* 如果数据需要回绕存储 */
    {
        // 1. 先填满从写指针到末尾的空间
        memcpy(&port->fifo[port->fifo_head], port->dma_buf, first_chunk_len);
        // 2. 将剩余的数据从环形队列的开头开始存储
        memcpy(&port->fifo[0], &port->dma_buf[first_chunk_len], write_len - first_chunk_len);
    }

    /* 更新写指针，处理回绕 */
    port->fifo_head = (port->fifo_head + write_len) % UART_FIFO_SIZE;
    port->data_ready = 1; // 设置数据就绪标志
    port->rx_size = 0;    // 清除本次接收长度
}

/**
 * @brief 从指定串口 FIFO 中读取固定长度数据
 * @param id        串口编号
 * @param buf       输出缓冲区
 * @param read_len  需要读取的字节数
 * @retval uint16_t: 实际读取字节数
 */
static uint16_t BSP_USART_fifo_read_bytes(uint8_t id, uint8_t *buf, uint16_t read_len)
{
    BSP_USART_t *port = BSP_USART_GetById(id);

    // 计算第一段长度
    uint16_t first_chunk = UART_FIFO_SIZE - port->fifo_tail;
    if (first_chunk > read_len)
        first_chunk = read_len;

    // 拷贝第一段
    memcpy(buf, &port->fifo[port->fifo_tail], first_chunk);

    // 拷贝第二段（回绕）
    if (read_len > first_chunk)
    {
        memcpy(buf + first_chunk, &port->fifo[0], read_len - first_chunk);
    }

    // 更新 tail
    port->fifo_tail = (port->fifo_tail + read_len) % UART_FIFO_SIZE;

    return read_len;
}

/*==============================================================================
 * 初始化函数
 *============================================================================*/
/**
 * @brief 初始化 BSP 层
 * @note 清空端口状态，FIFO 和 DMA 状态
 */
void BSP_USART_Init(void)
{
    memset(bsp_usart_pool, 0, sizeof(bsp_usart_pool));
    bsp_usart_count = 0;
}

/**
 * @brief 注册一个 BSP 串口实例
 * @note 初始化 FIFO 和 DMA 接收，使用结构体内部的 DMA 缓冲区
 * @param id        串口编号（逻辑编号，不要求等于数组下标）
 * @param huart     UART 硬件句柄
 * @param hdma_rx   UART 对应 DMA 接收句柄
 * @retval 1: 注册成功
 * @retval 0: 注册失败
 */
uint8_t BSP_USART_RegisterPort(uint8_t id, UART_HandleTypeDef *huart, DMA_HandleTypeDef  *hdma_rx)
{
    if (!huart || !hdma_rx)
        return 0;

    /* 池满则失败 */
    if (bsp_usart_count >= USART_MAX_PORTS)
        return 0;

    /* 防止重复注册同一 ID */
    if (BSP_USART_GetById(id) != NULL)
        return 0;

    /* 使用下一个空闲槽位 */
    BSP_USART_t *port = &bsp_usart_pool[bsp_usart_count];
    port->id         = id;
    port->data_ready = 0;
    port->rx_size    = 0;
    port->huart      = huart;
    port->hdma_rx    = hdma_rx;
    port->fifo_head  = 0;
    port->fifo_tail  = 0;
    port->tx_busy    = 0;

    memset(port->dma_buf, 0, USART_BUF_SIZE);
    memset(port->fifo, 0, UART_FIFO_SIZE);

    BSP_USART_restart_dma(port);

    bsp_usart_count++;   /* 已注册数加一 */
    return 1;
}

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
void BSP_USART_Send(uint8_t id, uint8_t *buf, uint16_t len)
{
    BSP_USART_t *port = BSP_USART_GetById(id);
    if (!port || !buf || len == 0)
    return;

    HAL_UART_Transmit_DMA(port->huart, buf, len);
}

/**
 * @brief 获取UART发送忙状态
 * @retval 1 忙
 * @retval 0 空闲
 */
uint8_t BSP_USART_ReadTxBusy(uint8_t id)
{
    BSP_USART_t *port = BSP_USART_GetById(id);
    return port->tx_busy;
}

/**
 * @brief 设置UART发送忙状态为1，表示发送中
 */
void BSP_USART_WriteTxBusy(uint8_t id)
{
    BSP_USART_t *port = BSP_USART_GetById(id);
    port->tx_busy = 1;
}

/**
 * @brief 设置UART发送忙状态为0，表示发送完成
 */
void BSP_USART_WriteTxBusyFree(uint8_t id)
{
    BSP_USART_t *port = BSP_USART_GetById(id);
    port->tx_busy = 0;
}

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
uint16_t BSP_USART_ReadRawData(uint8_t id, uint8_t *buf, uint16_t buf_size)
{
    BSP_USART_t *port = BSP_USART_GetById(id);

    if (!port || !buf || buf_size == 0)
        return 0;

    // FIFO 数据量
    uint16_t fifo_count = (port->fifo_head + UART_FIFO_SIZE - port->fifo_tail) % UART_FIFO_SIZE;

    if (fifo_count == 0)
        return 0;

    // 调用内部函数
    return BSP_USART_fifo_read_bytes(id, buf, fifo_count);
}

/*==============================================================================
 * 串口中断回调
 *============================================================================*/
/**
 * @brief HAL UART DMA+IDLE 接收回调
 * @param huart UART 硬件句柄
 * @param Size  本次接收字节数
 */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    BSP_USART_t* port = BSP_USART_GetHuartById(huart);
    if (!port)
    {
        // 对于未注册的串口中断，仅重启其DMA以避免中断风暴
        HAL_UARTEx_ReceiveToIdle_DMA(huart, NULL, 0); // 使用NULL和0安全地停止和重置
        return;
    }

    if (Size > 0)
    {
        // 1. 记录本次接收长度
        port->rx_size = Size;
        uint8_t id = port->id;

        // 2. 将数据从DMA缓冲区快速拷贝到软件FIFO，防止数据被覆盖
        BSP_USART_enqueue_dma_data_to_fifo(port);

        // 3. 发送通知到消息队列，告知处理任务有新数据到达
        osMessageQueuePut(RX_QHandle, &id, 0, 0);
    }

    // 4. 重新启动DMA接收，为下一次数据接收做准备
    BSP_USART_restart_dma(port);
}

/**
 * @brief HAL UART DMA 发送完成回调
 * @param huart UART 硬件句柄
 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    BSP_USART_t* port = BSP_USART_GetHuartById(huart);
    if (!port)
    {
        // 对于未注册的串口中断，仅重启其DMA以避免中断风暴
        HAL_UART_Transmit_DMA(huart, NULL, 0); // 使用NULL和0安全地停止和重置
        return;
    }

    if (BSP_USART_ReadTxBusy(port->id) == 1)
    {
        BSP_USART_WriteTxBusyFree(port->id);
        osSemaphoreRelease(TX_BSHandle);
    }
}