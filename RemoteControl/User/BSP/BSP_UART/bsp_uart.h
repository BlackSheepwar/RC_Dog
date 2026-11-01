/**
 * @file bsp_uart.h
 * @brief 串口硬件抽象：DMA 收发 + 字节流队列
 * @author 李嘉图
 * @date 2026-10-31
 *
 * @note 本层只做两件事：把 DMA 收到的字节搬进队列、把队列里的字节交给 DMA。
 *       不知道帧格式，不知道谁在发，也不做任何业务判断。
 *
 *       接收用 DMA 循环模式 + 增量搬运：中断里（半满/全满）按 DMA 计数器
 *       增量把新字节压进 GFIFO，任务侧每 2 ms 再补一次搬运。
 *       不依赖空闲中断（IDLE）：慢速小包本来就触发不了半满/全满，
 *       正确性交给周期性补搬，中断只负责让突发数据更快到达。
 *
 *       单生产者单消费者，队列无锁；句柄为不透明类型，
 *       端口与外设映射只出现在 bsp_uart.c。
 */

#ifndef __BSP_UART_H__
#define __BSP_UART_H__

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include <stdint.h>

/*==============================================================================
 * 宏定义与常量
 *============================================================================*/
/** @brief 本板串口总数（与 bsp_uart.c 的实例表必须一致） */
#define BSP_UART_PORT_COUNT     1

/** @brief 阻塞发送的默认超时（ms） */
#define BSP_UART_SEND_TIMEOUT   100

/*==============================================================================
 * 类型定义
 *============================================================================*/
/**
 * @brief 串口句柄（不透明，布局在 bsp_uart.c）
 */
typedef struct bsp_uart bsp_uart_t;

/**
 * @brief 端口事件回调（在中断上下文被调用）
 * @note 回调里只允许做"置标志 / 释放信号量 / 投递消息"这类短操作，
 *       禁止阻塞、禁止打日志、禁止再调本层的收发函数。
 */
typedef void (*bsp_uart_notify_t)(const bsp_uart_t *port);

/*==============================================================================
 * 端口实例
 *
 * 只有这个对象是 extern 的：上层拿指针，不拿内容。
 *============================================================================*/
/** @brief UART1 实例（逻辑编号 1，与狗主控的链路） */
extern const bsp_uart_t bsp_uart1;

/*==============================================================================
 * 初始化与启动
 *============================================================================*/
/**
 * @brief 初始化 BSP 串口层，为每个端口建立字节流队列
 * @retval 0: 成功
 * @retval -1: 队列建立失败（缓冲区或参数非法）
 * @note 只清状态、不启动 DMA；重复调用等价于重新初始化（会丢未读数据）。
 *       必须先于 BSP_UART_Start 调用。
 */
int BSP_UART_Init(void);

/**
 * @brief 启动指定端口的 DMA 循环接收
 * @param port 串口句柄，不可为 NULL
 * @retval 0: 已启动
 * @retval -1: 句柄为空、无 DMA 通道或 HAL 启动失败
 * @note DMA 以循环模式常驻，空闲/半满/全满中断都会把新增字节搬进队列。
 *       重复调用安全：先停旧 DMA 再重启，队列内容保留。
 */
int BSP_UART_Start(const bsp_uart_t *port);

/*==============================================================================
 * 端口查询
 *============================================================================*/
/**
 * @brief 取端口数量
 * @retval 端口总数（当前为 BSP_UART_PORT_COUNT）
 */
uint8_t BSP_UART_Count(void);

/**
 * @brief 按序号取端口句柄
 * @param index 序号，0 ~ BSP_UART_Count()-1
 * @retval 非 NULL: 端口句柄
 * @retval NULL: 序号越界
 */
const bsp_uart_t *BSP_UART_Get(uint8_t index);

/**
 * @brief 取端口的逻辑编号
 * @param port 串口句柄，可为 NULL
 * @retval 1~BSP_UART_PORT_COUNT: 逻辑编号
 * @retval 0: 句柄为空
 */
uint8_t BSP_UART_Id(const bsp_uart_t *port);

/**
 * @brief 按逻辑编号查端口句柄
 * @param id 逻辑编号（1 起）
 * @retval 非 NULL: 端口句柄
 * @retval NULL: 编号不存在
 */
const bsp_uart_t *BSP_UART_Find(uint8_t id);

/*==============================================================================
 * 事件注册
 *============================================================================*/
/**
 * @brief 注册"收到新字节"通知
 * @param fn 回调函数，NULL 表示取消注册
 * @retval 无
 * @note 回调在 UART 中断里被调用，每次搬入新字节后触发一次。
 */
void BSP_UART_Set_Rx_Notify(bsp_uart_notify_t fn);

/**
 * @brief 注册"DMA 发送完成"通知
 * @param fn 回调函数，NULL 表示取消注册
 * @retval 无
 * @note 回调在 HAL_UART_TxCpltCallback 里被调用，可用于释放信号量。
 */
void BSP_UART_Set_Tx_Done(bsp_uart_notify_t fn);

/*==============================================================================
 * 接收
 *============================================================================*/
/**
 * @brief 读出并丢弃队列里的字节
 * @param port 串口句柄，不可为 NULL
 * @param dst  目标缓冲区，不可为 NULL
 * @param len  最多读取的字节数
 * @retval 实际读出的字节数（0 表示队列为空）
 * @note 任务上下文调用；中断里只写，这里只读。
 */
uint32_t BSP_UART_Rx_Read(const bsp_uart_t *port, uint8_t *dst, uint32_t len);

/**
 * @brief 零拷贝查看队首连续可读区间
 * @param[in]  port 串口句柄，不可为 NULL
 * @param[out] len  连续可读字节数，可为 NULL
 * @retval 非 NULL: 指向队列内部的只读指针，有效期到下一次 Rx_Skip/Rx_Read
 * @retval NULL: 队列为空或参数非法
 * @note 用于"喂给解包状态机"这类逐字节消费场景，
 *       处理完必须调用 BSP_UART_Rx_Skip 推进读指针，否则数据不会被释放。
 */
const uint8_t *BSP_UART_Rx_Peek(const bsp_uart_t *port, uint32_t *len);

/**
 * @brief 推进读指针，释放已处理的数据
 * @param port 串口句柄，不可为 NULL
 * @param len  已处理的字节数
 * @retval 无
 * @note len 必须来自同一次 BSP_UART_Rx_Peek 返回的区间。
 */
void BSP_UART_Rx_Skip(const bsp_uart_t *port, uint32_t len);

/**
 * @brief 补一次"把 DMA 新字节搬进队列"
 * @param port 串口句柄，不可为 NULL
 * @retval 1: 本次搬进了新字节
 * @retval 0: 没有新字节或句柄非法
 * @note 任务上下文调用。中断里的搬运是同一个函数，所以重复调用绝对安全
 *       （增量按 DMA 计数器算，搬过就不会再搬）。
 *       慢速小包（几毫秒才来一帧）不会触发半满/全满中断，靠这一句兜底。
 */
uint8_t BSP_UART_Rx_Poll(const bsp_uart_t *port);

/**
 * @brief 查询队列中待读字节数
 * @param port 串口句柄，可为 NULL
 * @retval 待读字节数（0 表示已排空或句柄非法）
 */
uint32_t BSP_UART_Rx_Count(const bsp_uart_t *port);

/**
 * @brief 查询因队列满而丢弃的字节累计数
 * @param port 串口句柄，可为 NULL
 * @retval 累计丢弃字节数，0 表示从未溢出
 * @note 该值只增不减，用于现场判断"是丢数据还是没数据"。
 */
uint32_t BSP_UART_Rx_Overflow(const bsp_uart_t *port);

/*==============================================================================
 * 发送
 *============================================================================*/
/**
 * @brief 启动一次 DMA 发送（非阻塞）
 * @param port 串口句柄，不可为 NULL
 * @param buf  待发送缓冲区，不可为 NULL
 * @param len  发送长度，必须 > 0
 * @retval 0: 已启动，完成后触发 Tx_Done 通知
 * @retval -1: 参数非法或上次发送未完成
 * @retval -2: HAL 启动失败
 * @note buf 必须一直有效到发送完成（DMA 直接读该地址），
 *       不要在栈上开临时数组后立刻返回。
 */
int BSP_UART_Tx_Start(const bsp_uart_t *port, const uint8_t *buf, uint16_t len);

/**
 * @brief 查询 DMA 发送是否进行中
 * @param port 串口句柄，可为 NULL
 * @retval 1: 发送中
 * @retval 0: 空闲或句柄非法
 */
uint8_t BSP_UART_Tx_Is_Busy(const bsp_uart_t *port);

/**
 * @brief 阻塞式发送（轮询，不占用 DMA 通道）
 * @param port    串口句柄，不可为 NULL
 * @param buf     待发送缓冲区，不可为 NULL
 * @param len     发送长度
 * @param timeout 超时时间（ms）
 * @retval 0: 全部发出
 * @retval -1: 参数非法或超时
 * @note 只给调试打印这类低频场景用；长度越大阻塞越久。
 */
int BSP_UART_Send(const bsp_uart_t *port, const uint8_t *buf, uint16_t len,
                  uint32_t timeout);

#endif /* __BSP_UART_H__ */
