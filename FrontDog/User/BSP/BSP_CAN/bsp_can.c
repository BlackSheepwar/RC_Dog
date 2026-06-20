/**
 * @file bsp_can.c
 * @brief CAN硬件接口实现
 * @author 李嘉图
 * @date 2026-06-01
 *
 * @note 硬件映射表（id → hcan）在此文件中静态定义，
 *       上层通过 id 调用，无需知道具体 CAN 外设。
 *       映射表为编译期常量，运行时不修改。
 *       支持双CAN（hcan1/hcan2），滤波器28个平均分配。
 *       滤波器 id1~id4 直通 HAL 寄存器字段，不介入位运算。
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include <stdint.h>
#include <string.h>
#include "common.h"
#include "bsp_can.h"
#include "main.h"           /* CAN_F0/F1_QHandle, CAN_TX_BSHandle */

/*==============================================================================
 * CAN 硬件映射表（编译期常量）
 *
 * 将逻辑 ID（上层使用的 CAN 实例编号）映射到具体 CAN 硬件句柄。
 * 当前映射：
 *   id 0 → hcan1（CAN1）
 *   id 1 → hcan2（CAN2）
 *
 * ★ 此表为 BSP 内部实现，上层不可见。可根据需要扩展 ★
 *============================================================================*/
/**
 * @brief CAN实例配置
 * @note 编译期固定的硬件映射，运行时不修改
 */
typedef struct {
    uint8_t      id;             // CAN实例ID
    CAN_HandleTypeDef *hcan;     // CAN硬件句柄
} BSP_CAN_Map_t;

/** @brief CAN硬件映射表 */
static const BSP_CAN_Map_t BSP_CAN_MAP[BSP_CAN_MAX_NUM] = {
    { .id = 0, .hcan = &hcan1 },
    { .id = 1, .hcan = &hcan2 },
};

/** @brief CAN实例启动状态（运行时可变） */
static uint8_t bsp_can_started[BSP_CAN_MAX_NUM] = {0};

/*==============================================================================
 * 内部函数
 *============================================================================*/
/**
 * @brief 根据 ID 查找 CAN 硬件映射
 * @param id CAN实例ID
 * @retval 非NULL：找到
 * @retval NULL：未找到
 */
static const BSP_CAN_Map_t *BSP_CAN_FindMap(uint8_t id)
{
    for (uint8_t i = 0; i < ARRAY_SIZE(BSP_CAN_MAP); i++)
    {
        if (BSP_CAN_MAP[i].id == id)
            return &BSP_CAN_MAP[i];
    }
    return NULL;
}

/**
 * @brief 根据句柄查找 CAN 硬件映射
 * @param hcan CAN硬件句柄
 * @retval 非NULL：找到
 * @retval NULL：未找到
 */
static const BSP_CAN_Map_t *BSP_CAN_FindByHcan(CAN_HandleTypeDef *hcan)
{
    for (uint8_t i = 0; i < ARRAY_SIZE(BSP_CAN_MAP); i++)
    {
        if (BSP_CAN_MAP[i].hcan == hcan)
            return &BSP_CAN_MAP[i];
    }
    return NULL;
}

/*==============================================================================
 * 初始化函数
 *============================================================================*/
/**
 * @brief 初始化BSP CAN层
 * @note 映射表为编译期常量，仅复位运行时启动状态
 */
void BSP_CAN_Init(void)
{
    /* 消耗 CubeMX 生成的初始计数值（初始=1→0，适配 ISR 信号模式） */
    osSemaphoreAcquire(CAN_TX_BSHandle, 0);

    memset(bsp_can_started, 0, sizeof(bsp_can_started));
}

/**
 * @brief 根据CAN句柄获取实例ID
 * @param hcan CAN硬件句柄
 * @retval 0~BSP_CAN_MAX_NUM-1: 对应实例ID
 * @retval 0xFF: 未找到
 */
uint8_t BSP_CAN_GetIdByHcan(CAN_HandleTypeDef *hcan)
{
    const BSP_CAN_Map_t *map = BSP_CAN_FindByHcan(hcan);
    return map ? map->id : 0xFF;
}

/*==============================================================================
 * 控制函数
 *============================================================================*/
/**
 * @brief 启动CAN通信
 * @param id CAN实例ID
 * @retval 1: 启动成功
 * @retval 0: 启动失败
 *
 * @note 启动后自动使能 FIFO0/FIFO1 消息挂起中断及溢出中断，
 *       以及发送邮箱空中断（用于上层 TX 缓冲排空）。
 *       重复调用安全，已启动则直接返回成功。
 */
uint8_t BSP_CAN_Start(uint8_t id)
{
    /* ---------- 查找实例 ---------- */
    const BSP_CAN_Map_t *map = BSP_CAN_FindMap(id);
    if (!map) return 0;                     // 未找到
    uint8_t idx = map - BSP_CAN_MAP;        // 计算索引

    if (bsp_can_started[idx]) return 1;     // 已启动，直接返回

    /* ---------- HAL启动CAN ---------- */
    if (HAL_CAN_Start(map->hcan) != HAL_OK) return 0;   // 启动失败

    /* ---------- 使能FIFO接收中断 ---------- */
    __HAL_CAN_ENABLE_IT(map->hcan, CAN_IT_RX_FIFO0_MSG_PENDING);
    __HAL_CAN_ENABLE_IT(map->hcan, CAN_IT_RX_FIFO1_MSG_PENDING);

    /* ---------- 使能FIFO溢出中断 ---------- */
    __HAL_CAN_ENABLE_IT(map->hcan, CAN_IT_RX_FIFO0_OVERRUN);
    __HAL_CAN_ENABLE_IT(map->hcan, CAN_IT_RX_FIFO1_OVERRUN);

    /* ---------- 使能发送邮箱空中断 ---------- */
    __HAL_CAN_ENABLE_IT(map->hcan, CAN_IT_TX_MAILBOX_EMPTY);

    bsp_can_started[idx] = 1;
    return 1;
}

/*==============================================================================
 * 滤波器配置
 *============================================================================*/
/**
 * @brief 配置CAN滤波器
 * @param id  CAN实例ID
 * @param cfg 滤波器配置参数（id1~id4直通寄存器）
 * @retval 1: 配置成功
 * @retval 0: 配置失败
 *
 * @note F405共28个滤波器（0-27），0-13给CAN1，14-27给CAN2。
 *       SlaveStartFilterBank=14 自动分配。
 */
uint8_t BSP_CAN_FilterConfig(uint8_t id, const BSP_CAN_FilterConfig_t *cfg)
{
    /* ---------- 参数检查 ---------- */
    const BSP_CAN_Map_t *map = BSP_CAN_FindMap(id);
    if (!map || !cfg) return 0;             // 实例未找到或参数为空

    if (cfg->filter_bank > 27) return 0;    // F405滤波器序号0-27

    /* ---------- 组装HAL滤波器结构 ---------- */
    CAN_FilterTypeDef filter;

    filter.FilterBank           = cfg->filter_bank;
    filter.FilterMode           = cfg->mode;
    filter.FilterScale          = cfg->scale;
    filter.FilterFIFOAssignment = cfg->fifo;
    filter.FilterActivation     = ENABLE;
    filter.SlaveStartFilterBank = 14;   // 0-13给CAN1, 14-27给CAN2

    /* id1~id4 直接透传 */
    filter.FilterIdHigh      = cfg->id1;
    filter.FilterIdLow       = cfg->id2;
    filter.FilterMaskIdHigh  = cfg->id3;
    filter.FilterMaskIdLow   = cfg->id4;

    /* ---------- 写入硬件 ---------- */
    if (HAL_CAN_ConfigFilter(map->hcan, &filter) != HAL_OK)
        return 0;   // HAL配置失败

    return 1;
}

/*==============================================================================
 * 发送函数
 *============================================================================*/
/**
 * @brief 发送CAN帧（支持 标准/扩展 × 数据/遥控 四种组合）
 * @param id       CAN实例ID
 * @param can_id   ID值（标准帧11bit / 扩展帧29bit）
 * @param is_extid ID类型: BSP_CAN_STDID / BSP_CAN_EXTID
 * @param is_remote 帧类型: BSP_CAN_DATA_FRAME / BSP_CAN_REMOTE_FRAME
 * @param data     数据指针（遥控帧传 NULL）
 * @param len      数据长度 (0~8, 遥控帧时表示期望返回的DLC)
 * @retval 1: 发送成功
 * @retval 0: 发送失败
 *
 * @note 非阻塞发送，通过HAL_CAN_AddTxMessage将帧加入硬件发送邮箱。
 */
uint8_t BSP_CAN_SendMsg(uint8_t id, uint32_t can_id, uint8_t is_extid, uint8_t is_remote, uint8_t *data, uint8_t len)
{
    /* ---------- 参数检查 ---------- */
    const BSP_CAN_Map_t *map = BSP_CAN_FindMap(id);
    if (!map) return 0;                     // 实例未找到

    if (len > BSP_CAN_DATA_LEN) return 0;   // 长度非法
    if (!is_remote && !data) return 0;      // 数据帧必须有数据
    if (is_extid && can_id > 0x1FFFFFFF) return 0;  // 扩展ID超限
    if (!is_extid && can_id > 0x7FF) return 0;      // 标准ID超限

    /* ---------- 组装发送帧头 ---------- */
    CAN_TxHeaderTypeDef tx_header;
    uint32_t used_mailbox;

    if (is_extid)
    {
        tx_header.StdId = 0;
        tx_header.ExtId = can_id;
        tx_header.IDE   = CAN_ID_EXT;
    }
    else
    {
        tx_header.StdId = can_id;
        tx_header.ExtId = 0;
        tx_header.IDE   = CAN_ID_STD;
    }

    tx_header.RTR = is_remote ? CAN_RTR_REMOTE : CAN_RTR_DATA;
    tx_header.DLC = len;

    /* ---------- 加入硬件发送邮箱 ---------- */
    if (HAL_CAN_AddTxMessage(map->hcan, &tx_header, data, &used_mailbox) != HAL_OK)
        return 0;   // 发送失败（邮箱满或CAN未启动）

    return 1;
}

/*==============================================================================
 * CAN中断回调
 *============================================================================*/
/**
 * @brief HAL库CAN接收FIFO0消息挂起中断回调
 * @param hcan 触发中断的CAN句柄
 * @note 将接收帧放入 CAN_F0_QHandle 消息队列，由 Task_CAN_RXF0 处理。
 */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    const BSP_CAN_Map_t *map = BSP_CAN_FindByHcan(hcan);
    if (!map) return;
    BSP_CAN_Packet_t rx_pkt;
    rx_pkt.id = map->id;
    HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rx_pkt.header, rx_pkt.data);
    osMessageQueuePut(CAN_RXF0_QHandle, &rx_pkt, 0, 0);
}

/**
 * @brief HAL库CAN接收FIFO1消息挂起中断回调
 * @param hcan 触发中断的CAN句柄
 * @note 将接收帧放入 CAN_F1_QHandle 消息队列，由 Task_CAN_RXF1 处理。
 */
void HAL_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    const BSP_CAN_Map_t *map = BSP_CAN_FindByHcan(hcan);
    if (!map) return;
    BSP_CAN_Packet_t rx_pkt;
    rx_pkt.id = map->id;
    HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO1, &rx_pkt.header, rx_pkt.data);
    osMessageQueuePut(CAN_RXF1_QHandle, &rx_pkt, 0, 0);
}

/*==============================================================================
 * CAN TX 完成中断回调
 *
 * HAL 在任一发送邮箱空出后调用此回调，释放信号量唤醒 TX 服务任务。
 * 由 TX 服务任务在任务上下文中排空软件发送缓冲。
 *============================================================================*/
/**
 * @brief HAL库CAN发送邮箱0完成回调
 * @param hcan 触发中断的CAN句柄
 */
void HAL_CAN_TxMailbox0CompleteCallback(CAN_HandleTypeDef *hcan)
{
    (void)hcan;
    osSemaphoreRelease(CAN_TX_BSHandle);
}

/**
 * @brief HAL库CAN发送邮箱1完成回调
 * @param hcan 触发中断的CAN句柄
 */
void HAL_CAN_TxMailbox1CompleteCallback(CAN_HandleTypeDef *hcan)
{
    (void)hcan;
    osSemaphoreRelease(CAN_TX_BSHandle);
}

/**
 * @brief HAL库CAN发送邮箱2完成回调
 * @param hcan 触发中断的CAN句柄
 */
void HAL_CAN_TxMailbox2CompleteCallback(CAN_HandleTypeDef *hcan)
{
    (void)hcan;
    osSemaphoreRelease(CAN_TX_BSHandle);
}