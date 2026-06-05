/**
 * @file app_can.c
 * @brief CAN应用层实现
 * @author 李嘉图
 * @date 2026-06-01
 *
 * @note 在BSP_CAN基础上提供应用层封装。
 *       使用静态资源池，注册时自动调用BSP注册。
 *       发送/接收均委托BSP层完成。
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
#include "main.h"
#include "app_can.h"
#include <string.h>

/*==============================================================================
 * 静态资源池
 *============================================================================*/
static APP_CAN_t app_can_pool[BSP_CAN_MAX_NUM];
static uint8_t app_can_count = 0;

/*==============================================================================
 * 内部函数
 *============================================================================*/
/**
 * @brief 根据ID查找CAN端口（O(n)）
 * @param id 端口ID
 * @retval 非NULL：返回对应端口结构体指针
 * @retval NULL：未找到
 */
static APP_CAN_t *APP_CAN_GetById(uint8_t id)
{
    for (uint8_t i = 0; i < app_can_count; i++)
    {
        if (app_can_pool[i].id == id)
            return &app_can_pool[i];
    }
    return NULL;
}

/*==============================================================================
 * 初始化函数
 *============================================================================*/
/**
 * @brief CAN应用层初始化
 * @note 初始化底层BSP并清空APP资源池
 */
void APP_CAN_Init(void)
{
    /* ---------- 初始化底层 ---------- */
    BSP_CAN_Init();

    /* ---------- 清空资源池 ---------- */
    memset(app_can_pool, 0, sizeof(app_can_pool));
    app_can_count = 0;
}

/**
 * @brief 注册一个CAN端口
 * @param id   端口ID（0=CAN1, 1=CAN2）
 * @param hcan CAN硬件句柄
 * @retval 1: 注册成功
 * @retval 0: 注册失败
 *
 * @note 委托 BSP_CAN_Register 完成底层注册，成功后在本层保存端口绑定关系。
 */
uint8_t APP_CAN_Register(uint8_t id, CAN_HandleTypeDef *hcan)
{
    /* ---------- 参数检查 ---------- */
    if (!hcan) return 0;            // 无效句柄

    /* ---------- 查重（O(n)） ---------- */
    if (APP_CAN_GetById(id) != NULL) return 0;  // 已存在

    /* ---------- 容量检查 ---------- */
    if (app_can_count >= BSP_CAN_MAX_NUM) return 0;  // 资源池已满

    /* ---------- 在BSP层注册 ---------- */
    if (BSP_CAN_Register(id, hcan) != 1) return 0;   // BSP注册失败

    /* ---------- 写入APP资源池 ---------- */
    APP_CAN_t *port = &app_can_pool[app_can_count];
    port->id     = id;

    app_can_count++;
    return 1;
}

/*==============================================================================
 * 滤波器配置
 *============================================================================*/
/**
 * @brief 配置CAN滤波器
 * @param id 端口ID
 * @param cfg 滤波器配置参数
 * @retval 1: 配置成功
 * @retval 0: 配置失败
 *
 * @note 委托 BSP_CAN_FilterConfig 完成硬件配置
 */
uint8_t APP_CAN_FilterConfig(uint8_t id, const BSP_CAN_FilterConfig_t *cfg)
{
    /* ---------- 查找端口 ---------- */
    APP_CAN_t *port = APP_CAN_GetById(id);
    if (!port) return 0;

    /* ---------- 委托BSP层配置 ---------- */
    return BSP_CAN_FilterConfig(port->id, cfg);
}

/*==============================================================================
 * 启动函数
 *============================================================================*/
/**
 * @brief 启动CAN通信
 * @param id 端口ID
 * @retval 1: 启动成功
 * @retval 0: 启动失败
 *
 * @note 委托 BSP_CAN_Start 启动硬件并使能FIFO中断
 */
uint8_t APP_CAN_Start(uint8_t id)
{
    /* ---------- 查找端口 ---------- */
    APP_CAN_t *port = APP_CAN_GetById(id);
    if (!port) return 0;

    /* ---------- 委托BSP层启动 ---------- */
    return BSP_CAN_Start(port->id);
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
 * @retval 1: 发送成功
 * @retval 0: 发送失败
 *
 * @note 委托 BSP_CAN_SendMsg 发送，非阻塞
 */
uint8_t APP_CAN_SendMsg(uint8_t id, uint32_t can_id, uint8_t is_extid, uint8_t is_remote, uint8_t *data, uint8_t len)
{
    /* ---------- 查找端口 ---------- */
    APP_CAN_t *port = APP_CAN_GetById(id);
    if (!port) return 0;

    /* ---------- 委托BSP层发送 ---------- */
    return BSP_CAN_SendMsg(port->id, can_id, is_extid, is_remote, data, len);
}
