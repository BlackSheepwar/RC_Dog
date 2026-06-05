/**
 * @file bsp_can.c
 * @brief CAN硬件接口实现
 * @author 李嘉图
 * @date 2026-06-01
 *
 * @note 使用静态资源池管理CAN实例。
 *       支持双CAN（hcan1/hcan2），滤波器28个平均分配。
 *       滤波器 id1~id4 直通 HAL 寄存器字段，不介入位运算。
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
#include "main.h"
#include "bsp_can.h"
#include <string.h>

/*==============================================================================
 * 静态资源池
 *============================================================================*/
static BSP_CAN_t bsp_can_pool[BSP_CAN_MAX_NUM];
static uint8_t bsp_can_count = 0;

/*==============================================================================
 * 内部函数
 *============================================================================*/
/**
 * @brief 根据ID查找CAN实例（O(n)）
 * @param id CAN实例ID
 * @retval 非NULL：返回对应CAN实例结构体指针
 * @retval NULL：未找到
 */
static BSP_CAN_t *BSP_CAN_GetById(uint8_t id)
{
    for (uint8_t i = 0; i < bsp_can_count; i++)
    {
        if (bsp_can_pool[i].id == id)
            return &bsp_can_pool[i];
    }
    return NULL;
}

/**
 * @brief 根据句柄查找CAN实例（O(n)）
 * @param hcan CAN实例句柄
 * @retval 非NULL：返回对应CAN实例结构体指针
 * @retval NULL：未找到
 */
static BSP_CAN_t *BSP_CAN_GetHuartById(CAN_HandleTypeDef *hcan)
{
    for (uint8_t i = 0; i < bsp_can_count; i++)
    {
        if (bsp_can_pool[i].hcan == hcan)
            return &bsp_can_pool[i];
    }
    return NULL;
}

/*==============================================================================
 * 初始化函数
 *============================================================================*/
/**
 * @brief 初始化BSP CAN层
 * @note 清空整个资源池，已注册实例全部失效
 */
void BSP_CAN_Init(void)
{
    memset(bsp_can_pool, 0, sizeof(bsp_can_pool));
    bsp_can_count = 0;
}

/**
 * @brief 注册一个CAN实例到资源池
 * @param id   CAN实例ID（0=CAN1, 1=CAN2）
 * @param hcan CAN硬件句柄（来自CubeMX生成代码）
 * @retval 1: 注册成功
 * @retval 0: 注册失败（参数无效/重复注册/资源池已满）
 *
 * @note 注册仅保存句柄，不操作硬件。启动CAN需额外调用 BSP_CAN_Start。
 */
uint8_t BSP_CAN_Register(uint8_t id, CAN_HandleTypeDef *hcan)
{
    /* ---------- 参数检查 ---------- */
    if (!hcan) return 0;            // 无效句柄

    /* ---------- 查重（O(n)） ---------- */
    if (BSP_CAN_GetById(id) != NULL) return 0;  // 已存在

    /* ---------- 容量检查 ---------- */
    if (bsp_can_count >= BSP_CAN_MAX_NUM) return 0;  // 资源池已满

    /* ---------- 写入资源池 ---------- */
    BSP_CAN_t *can = &bsp_can_pool[bsp_can_count];
    can->id     = id;
    can->hcan   = hcan;
    can->started = 0;

    bsp_can_count++;
    return 1;
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
 * @note 启动后自动使能 FIFO0/FIFO1 消息挂起中断及溢出中断。
 *       重复调用安全，已启动则直接返回成功。
 */
uint8_t BSP_CAN_Start(uint8_t id)
{
    /* ---------- 查找实例 ---------- */
    BSP_CAN_t *can = BSP_CAN_GetById(id);
    if (!can) return 0;             // 未找到

    if (can->started) return 1;     // 已启动，直接返回

    /* ---------- HAL启动CAN ---------- */
    if (HAL_CAN_Start(can->hcan) != HAL_OK) return 0;   // 启动失败

    /* ---------- 使能FIFO接收中断 ---------- */
    __HAL_CAN_ENABLE_IT(can->hcan, CAN_IT_RX_FIFO0_MSG_PENDING);
    __HAL_CAN_ENABLE_IT(can->hcan, CAN_IT_RX_FIFO1_MSG_PENDING);

    /* ---------- 使能FIFO溢出中断 ---------- */
    __HAL_CAN_ENABLE_IT(can->hcan, CAN_IT_RX_FIFO0_OVERRUN);
    __HAL_CAN_ENABLE_IT(can->hcan, CAN_IT_RX_FIFO1_OVERRUN);

    can->started = 1;
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
    BSP_CAN_t *can = BSP_CAN_GetById(id);
    if (!can || !cfg) return 0;             // 实例未找到或参数为空

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
    if (HAL_CAN_ConfigFilter(can->hcan, &filter) != HAL_OK)
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
    BSP_CAN_t *can = BSP_CAN_GetById(id);
    if (!can) return 0;                     // 实例未找到

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
    if (HAL_CAN_AddTxMessage(can->hcan, &tx_header, data, &used_mailbox) != HAL_OK)
        return 0;   // 发送失败（邮箱满或CAN未启动）

    return 1;
}

/*==============================================================================
 * CAN中断回调
 *============================================================================*/
/**
 * @brief HAL库CAN接收FIFO0消息挂起中断回调
 * @param hcan 触发中断的CAN句柄
 * @note 释放 CAN_F0_BS 二进制信号量唤醒对应任务。
 *       由任务上下文调用 HAL_CAN_GetRxMessage 并排空所有待处理帧。
 */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    BSP_CAN_t* port = BSP_CAN_GetHuartById(hcan);
    BSP_CAN_Packet_t rx_pkt;
    rx_pkt.id = port->id;
    HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rx_pkt.header, rx_pkt.data);
    osMessageQueuePut(CAN_F0_QHandle, &rx_pkt, 0, 0);
}

/**
 * @brief HAL库CAN接收FIFO1消息挂起中断回调
 * @param hcan 触发中断的CAN句柄
 * @note 释放 CAN_F1_BS 二进制信号量唤醒对应任务。
 */
void HAL_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    BSP_CAN_t* port = BSP_CAN_GetHuartById(hcan);
    BSP_CAN_Packet_t rx_pkt;
    rx_pkt.id = port->id;
    HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO1, &rx_pkt.header, rx_pkt.data);
    osMessageQueuePut(CAN_F1_QHandle, &rx_pkt, 0, 0);
}

/*==============================================================================
 * CAN错误中断回调
 *============================================================================*/
/**
 * @brief HAL库CAN错误中断回调
 * @param hcan 触发错误中断的CAN句柄
 * @note 清除溢出标志。如需统计丢帧可在此处累加计数器。
 */
void HAL_CAN_ErrorCallback(CAN_HandleTypeDef *hcan)
{
    /* ---------- 清除FIFO0溢出标志 ---------- */
    if (__HAL_CAN_GET_FLAG(hcan, CAN_FLAG_FOV0))
    {
        __HAL_CAN_CLEAR_FLAG(hcan, CAN_FLAG_FOV0);
    }

    /* ---------- 清除FIFO1溢出标志 ---------- */
    if (__HAL_CAN_GET_FLAG(hcan, CAN_FLAG_FOV1))
    {
        __HAL_CAN_CLEAR_FLAG(hcan, CAN_FLAG_FOV1);
    }
}
