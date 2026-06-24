/**
 * @file bsp_uart.c
 * @brief UART 硬件接口实现
 * @author 李嘉图
 * @date 2026-06-20
 *
 * @note 硬件映射表（id → huart）在此文件中静态定义，
 *       上层通过 id 调用，无需知道具体 UART 外设。
 *       映射表为编译期常量，运行时不修改。
 *
 *       遵循 CAN 接口风格：
 *         - BSP 只操作寄存器，不做缓冲管理
 *         - DMA 循环缓冲区由 APP 提供（ConfigDMARx 注入）
 *         - 中断只通知不拷贝（位置计算由 APP 完成）
 *
 *       三个中断入口信号统一：
 *         - IDLE  (HAL_UARTEx_RxEventCallback)
 *         - HT    (HAL_UART_RxHalfCpltCallback)
 *         - TC    (HAL_UART_RxCpltCallback)
 *         → 全部走同一轻量处理：读 ID → 发消息 → 退出
 *
 *       发送采用忙标志 + 队列驱动，无信号量依赖。
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include <stdint.h>
#include <string.h>
#include "main.h"           /* UART_RX_QHandle, UART_TX_QHandle */
#include "common.h"         /* ARRAY_SIZE */
#include "bsp_uart.h"

/*==============================================================================
 * UART 硬件映射表（编译期常量）
 *
 * 将逻辑 ID（上层使用的 UART 实例编号）映射到具体 UART 硬件句柄。
 * 当前映射：
 *   id 1 → huart4
 *   id 2 → huart5
 *
 * ★ 此表为 BSP 内部实现，上层不可见。可根据需要扩展 ★
 *============================================================================*/
/**
 * @brief UART 实例配置
 * @note 编译期固定的硬件映射，运行时不修改
 */
typedef struct {
    uint8_t             id;         // UART 实例 ID
    UART_HandleTypeDef *huart;      // UART 硬件句柄
} BSP_UART_Map_t;

/** @brief UART 硬件映射表 */
static const BSP_UART_Map_t BSP_UART_MAP[BSP_UART_MAX_NUM] = {
    { .id = 1, .huart = &huart4 },
    { .id = 2, .huart = &huart5 },
};

/**
 * @brief DMA 接收运行时上下文
 * @note BSP 仅持有 DMA 缓冲区的地址和大小用于配置和位置计算，
 *       不拥有缓冲区本身（所有权在 APP 层）。
 */
static struct {
    uint8_t  *rx_buf;       // DMA 循环缓冲区地址（APP 注入）
    uint16_t rx_size;       // 缓冲区大小
} uart_dma_ctx[BSP_UART_MAX_NUM];

/**
 * @brief 发送忙标志（由 TxCpltCallback ISR 清除）
 * @note 无信号量，无消息队列，仅一个字节标志。
 *       TX 任务通过轮询此标志等待 DMA 完成后再发下一帧。
 */
static volatile uint8_t tx_busy[BSP_UART_MAX_NUM];

/*==============================================================================
 * 内部函数
 *============================================================================*/
/**
 * @brief 根据 ID 查找 UART 硬件映射
 * @param id UART 实例 ID
 * @retval 非 NULL: 找到
 * @retval NULL: 未找到
 */
static const BSP_UART_Map_t *BSP_UART_FindMap(uint8_t id)
{
    for (uint8_t i = 0; i < ARRAY_SIZE(BSP_UART_MAP); i++)
    {
        if (BSP_UART_MAP[i].id == id)
            return &BSP_UART_MAP[i];
    }
    return NULL;
}

/**
 * @brief 根据句柄查找 UART 硬件映射
 * @param huart UART 硬件句柄
 * @retval 非 NULL: 找到
 * @retval NULL: 未找到
 */
static const BSP_UART_Map_t *BSP_UART_FindByHuart(UART_HandleTypeDef *huart)
{
    for (uint8_t i = 0; i < ARRAY_SIZE(BSP_UART_MAP); i++)
    {
        if (BSP_UART_MAP[i].huart == huart)
            return &BSP_UART_MAP[i];
    }
    return NULL;
}

/*==============================================================================
 * 初始化函数
 *============================================================================*/
/**
 * @brief 初始化 BSP UART 层
 * @note 映射表为编译期常量，仅复位运行时 DMA 上下文和发送忙状态。
 */
void BSP_UART_Init(void)
{
    memset(uart_dma_ctx, 0, sizeof(uart_dma_ctx));
    memset((void*)tx_busy, 0, sizeof(tx_busy));
}

/**
 * @brief 根据 UART 句柄获取实例 ID
 * @param huart UART 硬件句柄
 * @retval 0~BSP_UART_MAX_NUM-1: 对应实例 ID
 * @retval 0xFF: 未找到
 */
uint8_t BSP_UART_GetIdByHuart(UART_HandleTypeDef *huart)
{
    const BSP_UART_Map_t *map = BSP_UART_FindByHuart(huart);
    return map ? map->id : 0xFF;
}

/*==============================================================================
 * DMA 接收配置
 *============================================================================*/
/**
 * @brief 配置并启动 DMA 循环接收
 * @param id    UART 实例 ID
 * @param buf   DMA 接收缓冲区（APP 层提供）
 * @param size  缓冲区大小
 *
 * @note DMA 以循环模式写入，永不停止。
 *       使能空闲中断和半满中断，确保任何速率下都不丢数据。
 *       重复调用会先停止旧的 DMA 再重启。
 */
void BSP_UART_ConfigDMARx(uint8_t id, uint8_t *buf, uint16_t size)
{
    const BSP_UART_Map_t *map = BSP_UART_FindMap(id);
    if (!map || !buf || size == 0)
        return;

    uint8_t idx = map - BSP_UART_MAP;

    /* 保存 DMA 缓冲信息用于位置计算 */
    uart_dma_ctx[idx].rx_buf  = buf;
    uart_dma_ctx[idx].rx_size = size;

    /* 停止旧 DMA 接收（防止重复配置时残留） */
    HAL_UART_DMAStop(map->huart);

    /* 强制 DMA 循环模式（CubeMX 默认生成 NORMAL，程序覆盖） */
    map->huart->hdmarx->Init.Mode = DMA_CIRCULAR;
    HAL_DMA_Init(map->huart->hdmarx);

    /* 启动 DMA 循环接收 */
    HAL_UART_Receive_DMA(map->huart, buf, size);

    /* 使能半满中断（CubeMX 可能已配置，显式确保） */
    __HAL_DMA_ENABLE_IT(map->huart->hdmarx, DMA_IT_HT);
}

/**
 * @brief 获取当前 DMA 写入位置
 * @param id UART 实例 ID
 * @return DMA 在循环缓冲区中的写入偏移（0 ~ size-1）
 *
 * @note 通过 DMA Counter 寄存器反算：
 *         写入位置 = 缓冲区大小 - Counter 剩余值
 *       当写入位置等于上次读取位置时，表示无新数据。
 */
uint16_t BSP_UART_GetDMAPos(uint8_t id)
{
    const BSP_UART_Map_t *map = BSP_UART_FindMap(id);
    if (!map)
        return 0;

    uint8_t idx = map - BSP_UART_MAP;

    if (uart_dma_ctx[idx].rx_size == 0)
        return 0;

    return uart_dma_ctx[idx].rx_size
           - __HAL_DMA_GET_COUNTER(map->huart->hdmarx);
}

/*==============================================================================
 * 发送函数
 *============================================================================*/
/**
 * @brief TX 完成回调函数指针（上层注册，用于双缓冲等扩展）
 */
static BSP_UART_TxCpltFn_t user_txcplt_fn = NULL;

/**
 * @brief 注册 TX 完成回调
 * @param fn 回调函数
 */
void BSP_UART_SetTxCpltFn(BSP_UART_TxCpltFn_t fn)
{
    user_txcplt_fn = fn;
}

/**
 * @brief 通过 DMA 发送数据（非阻塞）
 * @param id  UART 实例 ID
 * @param buf 待发送数据指针
 * @param len 发送长度
 *
 * @note 直接调用 HAL 的 DMA 发送，标记忙标志。
 *       由 TX 任务保证前一次发送已完成（通过 BSP_UART_IsTxBusy 轮询）。
 */
void BSP_UART_SendDMA(uint8_t id, const uint8_t *buf, uint16_t len)
{
    const BSP_UART_Map_t *map = BSP_UART_FindMap(id);
    if (!map || !buf || len == 0)
        return;

    uint8_t idx = map - BSP_UART_MAP;
    tx_busy[idx] = 1;

    HAL_UART_Transmit_DMA(map->huart, buf, len);
}

/**
 * @brief 查询指定串口 DMA 发送是否忙
 * @param id UART 实例 ID
 * @retval 1: 发送中，不可发送新数据
 * @retval 0: 空闲
 */
uint8_t BSP_UART_IsTxBusy(uint8_t id)
{
    const BSP_UART_Map_t *map = BSP_UART_FindMap(id);
    if (!map) return 0;
    return tx_busy[map - BSP_UART_MAP];
}

/*==============================================================================
 * UART 中断回调
 *
 * 三个 RX 中断入口统一处理：
 *   - IDLE 中断（空闲线检测）
 *   - HT  中断（半满）
 *   - TC  中断（全满/回绕）
 *
 * 全部只做一件事：通知 RX 任务有新数据可读。
 * 不拷贝数据、不做 FIFO 操作，最大程度缩短中断路径。
 *============================================================================*/
/**
 * @brief 通用 RX 事件处理
 * @param huart 触发中断的 UART 句柄
 */
static void BSP_UART_OnRxEvent(UART_HandleTypeDef *huart)
{
    uint8_t id = BSP_UART_GetIdByHuart(huart);
    if (id == 0xFF)
        return;

    osMessageQueuePut(UART_RX_QHandle, &id, 0, 0);
}

/**
 * @brief HAL UART IDLE 空闲中断回调
 * @param huart UART 硬件句柄
 * @param Size  本次接收字节数（未使用，由 DMA Counter 计算）
 */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    (void)Size;
    BSP_UART_OnRxEvent(huart);
}

/**
 * @brief HAL UART DMA 半满中断回调
 * @param huart UART 硬件句柄
 */
void HAL_UART_RxHalfCpltCallback(UART_HandleTypeDef *huart)
{
    BSP_UART_OnRxEvent(huart);
}

/**
 * @brief HAL UART DMA 全满中断回调
 * @param huart UART 硬件句柄
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    BSP_UART_OnRxEvent(huart);
}

/**
 * @brief HAL UART DMA 发送完成回调
 * @param huart UART 硬件句柄
 * @note 清除发送忙标志，TX 任务通过轮询此标志同步。
 *       无信号量、无消息队列操作，中断路径极短。
 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    const BSP_UART_Map_t *map = BSP_UART_FindByHuart(huart);
    if (!map) return;

    uint8_t id = map->id;
    uint8_t idx = map - BSP_UART_MAP;

    tx_busy[idx] = 0;

    /* 调用上层注册的回调（如双缓冲切换），通知此端口发送完成 */
    if (user_txcplt_fn)
        user_txcplt_fn(id);
}
