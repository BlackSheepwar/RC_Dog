/**
 * @file bsp_uart.h
 * @brief UART硬件接口
 * @author 李嘉图
 * @date 2026-06-20
 *
 * @note 硬件映射表（id → huart）在此文件中静态定义，
 *       上层通过 id 调用，无需知道具体 UART 外设。
 *       映射表为编译期常量，运行时不修改。
 *
 *       遵循 CAN 接口风格：BSP 层仅负责硬件配置与数据收发，
 *       不做任何缓冲管理，不持有软件 FIFO。
 *
 *       DMA 循环缓冲区由 APP 层分配，通过 ConfigDMARx 注入 BSP。
 *       三个中断入口（IDLE/HT/TC）统一通知任务，不拷贝数据。
 */

#ifndef __BSP_UART_H__
#define __BSP_UART_H__

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include <stdint.h>
// 功能包含
#include "usart.h"

/*==============================================================================
 * 宏定义与常量
 *============================================================================*/
#define BSP_UART_MAX_NUM        1           // UART 实例最大数量

/*==============================================================================
 * 初始化函数
 *============================================================================*/
/**
 * @brief 初始化 BSP UART 层
 * @note 复位运行时 DMA 上下文状态。
 *       映射表为编译期常量，无需注册。
 */
void BSP_UART_Init(void);

/**
 * @brief 根据 UART 句柄获取实例 ID
 * @param huart UART 硬件句柄
 * @retval 0~BSP_UART_MAX_NUM-1: 对应实例 ID
 * @retval 0xFF: 未找到
 */
uint8_t BSP_UART_GetIdByHuart(UART_HandleTypeDef *huart);

/*==============================================================================
 * DMA 接收配置
 *============================================================================*/
/**
 * @brief 配置并启动 DMA 循环接收
 * @param id    UART 实例 ID
 * @param buf   DMA 接收缓冲区（APP 层提供，BSP 只配置地址）
 * @param size  缓冲区大小（字节）
 * @note 启动后 DMA 以循环模式持续写入，永不停止。
 *       同时使能 HT/TC/IDLE 中断，触发后通知任务取数据。
 *       重复调用安全，先停止旧 DMA 再重启。
 */
void BSP_UART_ConfigDMARx(uint8_t id, uint8_t *buf, uint16_t size);

/**
 * @brief 获取当前 DMA 写入位置
 * @param id UART 实例 ID
 * @return 当前 DMA 在循环缓冲区中的写入偏移（0 ~ size-1）
 * @note 通过 DMA 的 Counter 寄存器计算，开销仅 2 次总线访问。
 */
uint16_t BSP_UART_GetDMAPos(uint8_t id);

/*==============================================================================
 * 发送函数
 *============================================================================*/
/**
 * @brief 通过 DMA 发送数据（非阻塞）
 * @param id  UART 实例 ID
 * @param buf 待发送数据指针
 * @param len 发送长度
 * @note 调用前需确保前一次 DMA 发送已完成（通过 BSP_UART_IsTxBusy 检查）。
 *       CRC 与帧格式由上层 Codec 层处理，BSP 不关心内容。
 */
void BSP_UART_SendDMA(uint8_t id, const uint8_t *buf, uint16_t len);

/**
 * @brief 查询 DMA 发送是否忙
 * @param id UART 实例 ID
 * @retval 1: 发送中
 * @retval 0: 空闲
 */
uint8_t BSP_UART_IsTxBusy(uint8_t id);

/*==============================================================================
 * TX 完成回调（用于上层双缓冲等扩展功能）
 *============================================================================*/
/** @brief TX 完成回调函数类型 */
typedef void (*BSP_UART_TxCpltFn_t)(uint8_t id);

/**
 * @brief 注册 TX 完成回调
 * @param fn 回调函数（NULL = 取消注册）
 * @note 在 HAL_UART_TxCpltCallback ISR 末尾被调用，
 *       函数内需保持 ISR 级别的约束（不阻塞、不操作互斥量）。
 *       典型用途：双缓冲 DMA 切换、释放 TX 信号量等。
 */
void BSP_UART_SetTxCpltFn(BSP_UART_TxCpltFn_t fn);

#endif /* __BSP_UART_H__ */
