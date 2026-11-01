/**
 * @file bsp_uart.c
 * @brief 串口硬件抽象实现：DMA 增量入队、GFIFO 字节流、DMA 发送
 * @author Ricardo
 * @date 2026-10-31
 *
 * @note 数据通路：
 *         DMA(循环) ──半满/全满中断 + 任务轮询──▶ 计算增量
 *                    ──GFIFO_Write──▶ 字节队列 ──Peek/Skip──▶ 解包器
 *         Svc ──BSP_UART_Tx_Start──▶ DMA ──TxCplt 中断──▶ 通知
 *
 *       为什么用"增量 + 字节队列"而不是"记位置 + 回读环形缓冲"：
 *         - 位置回读把环形缓冲的绕圈逻辑摊给了上层，绕圈处最易漏字节；
 *         - 中断里只做一次 memcpy，开销与到达字节数成正比；
 *         - 队列满时丢弃最旧的字节，且有计数可查，不会静默失联。
 *
 *       为什么不靠空闲中断（IDLE）收帧：
 *         H7 与 F1 的 HAL 对 ReceiveToIdle 的支持不一致（本项目 H7 的 HAL
 *         根本没有这个接口），而慢速小包又不会触发半满/全满。
 *         所以正确性交给"Svc 侧每 2 ms 轮询一次增量"，中断只用来降低突发
 *         数据的延迟——两条路径调的是同一个搬运函数，重复调用安全。
 *
 *       DMA 缓冲区与队列缓冲区都在 .bss（AXI SRAM 0x2400_0000），
 *       DMA1 可直接访问，不需要额外的链接段。
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include <stdint.h>
#include <string.h>
#include "main.h"
#include "usart.h"
#include "gfifo.h"
#include "bsp_uart.h"

/*==============================================================================
 * 宏定义与常量
 *============================================================================*/
/** @brief DMA 单次搬运缓冲区大小（字节），决定中断最大间隔 */
#define BSP_UART_DMA_BUF_SIZE   128
/** @brief 每个端口的接收队列容量（字节），放得下 10 帧最长报文 */
#define BSP_UART_RX_FIFO_SIZE   512

/*==============================================================================
 * 类型定义
 *============================================================================*/
/**
 * @brief 端口运行时状态
 * @note 句柄存配置，可写状态全部留在这里：句柄因此能保持 const。
 */
typedef struct {
    gfifo_t           rx;                             /* 字节流队列 */
    uint8_t           dma_buf[BSP_UART_DMA_BUF_SIZE]; /* DMA 循环缓冲区 */
    uint8_t           rx_buf[BSP_UART_RX_FIFO_SIZE];  /* 队列存储 */
    volatile uint16_t dma_last;                       /* 上次入队到的 DMA 偏移 */
    volatile uint8_t  tx_busy;                        /* 1: DMA 发送进行中 */
    volatile uint32_t rx_drop;                        /* 队列满丢弃的累计字节数 */
    volatile uint32_t err_count;                      /* 接收错误次数 */
} bsp_uart_rt_t;

/**
 * @brief 串口句柄（布局只出现在本文件）
 */
struct bsp_uart {
    void          *hal;   /* UART_HandleTypeDef *，强转集中在 BSP_UART_Hal_Of */
    uint8_t        id;    /* 逻辑编号，1 起 */
    bsp_uart_rt_t *rt;    /* 指向静态运行时状态 */
};

/*==============================================================================
 * 内部变量
 *============================================================================*/
/** @brief UART4 的运行时状态 */
static bsp_uart_rt_t s_rt_uart4;
/** @brief UART5 的运行时状态 */
static bsp_uart_rt_t s_rt_uart5;

/** @brief UART4 句柄：id 1 → huart4 */
const bsp_uart_t bsp_uart4 = { .hal = &huart4, .id = 1u, .rt = &s_rt_uart4 };
/** @brief UART5 句柄：id 2 → huart5 */
const bsp_uart_t bsp_uart5 = { .hal = &huart5, .id = 2u, .rt = &s_rt_uart5 };

/** @brief 端口实例表：所有按序号/编号的查找都走这张表 */
static const bsp_uart_t *const s_port_table[BSP_UART_PORT_COUNT] = {
    &bsp_uart4,
    &bsp_uart5,
};

/** @brief 收到新字节的通知回调（ISR 上下文） */
static bsp_uart_notify_t s_rx_notify = NULL;
/** @brief 发送完成的通知回调（ISR 上下文） */
static bsp_uart_notify_t s_tx_done = NULL;

/*==============================================================================
 * 内部函数
 *============================================================================*/
/**
 * @brief 取 HAL 句柄
 * @param port 串口句柄，可为 NULL
 * @retval 非 NULL: UART_HandleTypeDef 指针
 * @retval NULL: 句柄为空
 * @note void * → UART_HandleTypeDef * 的强转只允许出现在本函数。
 */
static UART_HandleTypeDef *BSP_UART_Hal_Of(const bsp_uart_t *port)
{
    return port ? (UART_HandleTypeDef *)port->hal : NULL;
}

/**
 * @brief 按 HAL 句柄反查端口
 * @param huart HAL UART 句柄
 * @retval 非 NULL: 对应端口
 * @retval NULL: 不是本层管理的端口
 */
static const bsp_uart_t *BSP_UART_Find_By_Hal(const UART_HandleTypeDef *huart)
{
    if (!huart)
        return NULL;

    for (uint8_t i = 0; i < BSP_UART_PORT_COUNT; i++)
    {
        if (s_port_table[i]->hal == (const void *)huart)
            return s_port_table[i];
    }
    return NULL;
}

/**
 * @brief 把 DMA 自上次入队以来的新字节搬进队列（中断上下文）
 * @param port 串口句柄，不可为 NULL
 * @retval 1: 本次有新字节入队
 * @retval 0: 没有新字节
 * @note 增量 = 当前写入偏移 − 上次记录偏移（按缓冲区大小回绕）。
 *       队列满时丢弃放不下的部分并累加 rx_drop，指针照样推进，
 *       否则下次会重复搬运同一段数据。
 */
static uint8_t BSP_UART_Rx_Harvest(const bsp_uart_t *port)
{
    UART_HandleTypeDef *hal = BSP_UART_Hal_Of(port);
    bsp_uart_rt_t      *rt  = port->rt;
    uint32_t            total;
    uint32_t            put;
    uint16_t            pos;
    uint16_t            last;

    if (!hal || !hal->hdmarx)
        return 0u;

    pos  = (uint16_t)(BSP_UART_DMA_BUF_SIZE - __HAL_DMA_GET_COUNTER(hal->hdmarx));
    last = rt->dma_last;

    if (pos >= BSP_UART_DMA_BUF_SIZE)   /* 计数器异常时的防御 */
        pos = 0u;
    if (pos == last)
        return 0u;

    if (pos > last)
    {
        /* ---------- 未回绕：一段连续数据 ---------- */
        total = (uint32_t)(pos - last);
        put   = GFIFO_Write(&rt->rx, &rt->dma_buf[last], total);
    }
    else
    {
        /* ---------- 回绕：尾段 + 头段 ---------- */
        uint32_t tail = (uint32_t)(BSP_UART_DMA_BUF_SIZE - last);

        total = tail + (uint32_t)pos;
        put   = GFIFO_Write(&rt->rx, &rt->dma_buf[last], tail);
        if (put == tail)
            put += GFIFO_Write(&rt->rx, &rt->dma_buf[0], (uint32_t)pos);
    }

    if (put < total)
        rt->rx_drop += (total - put);

    rt->dma_last = pos;
    return (put > 0u) ? 1u : 0u;
}

/*==============================================================================
 * 初始化与启动
 *============================================================================*/
/**
 * @brief 初始化 BSP 串口层
 * @retval 0: 成功
 * @retval -1: 队列初始化失败
 */
int BSP_UART_Init(void)
{
    for (uint8_t i = 0; i < BSP_UART_PORT_COUNT; i++)
    {
        const bsp_uart_t *port = s_port_table[i];
        bsp_uart_rt_t    *rt   = port->rt;

        memset(rt, 0, sizeof(*rt));

        if (GFIFO_Init(&rt->rx, rt->rx_buf, sizeof(rt->rx_buf), 1u,
                       GFIFO_FLAG_NONE) != GFIFO_OK)
            return -1;
    }

    s_rx_notify = NULL;
    s_tx_done   = NULL;
    return 0;
}

/**
 * @brief 启动指定端口的 DMA 循环接收
 * @param port 串口句柄，不可为 NULL
 * @retval 0: 已启动
 * @retval -1: 参数非法、无 DMA 通道或 HAL 启动失败
 * @note 用标准 DMA 接收接口而不是 ReceiveToIdle：后者在不同 HAL 版本里
 *       有无不一，而本层的正确性由状态轮询保证，不依赖空闲中断。
 */
int BSP_UART_Start(const bsp_uart_t *port)
{
    UART_HandleTypeDef *hal = BSP_UART_Hal_Of(port);

    if (!hal || !hal->hdmarx)
        return -1;

    /* 停掉可能残留的旧传输，位置计数同时归零 */
    (void)HAL_UART_DMAStop(hal);
    port->rt->dma_last = 0u;

    /* CubeMX 生成的是 NORMAL 模式，这里改成 CIRCULAR */
    hal->hdmarx->Init.Mode = DMA_CIRCULAR;
    if (HAL_DMA_Init(hal->hdmarx) != HAL_OK)
        return -1;

    if (HAL_UART_Receive_DMA(hal, port->rt->dma_buf,
                             BSP_UART_DMA_BUF_SIZE) != HAL_OK)
        return -1;

    /* 半满中断：只影响延迟，不承担正确性 */
    __HAL_DMA_ENABLE_IT(hal->hdmarx, DMA_IT_HT);
    return 0;
}

/*==============================================================================
 * 端口查询
 *============================================================================*/
/**
 * @brief 取端口数量
 * @retval 端口总数
 */
uint8_t BSP_UART_Count(void)
{
    return (uint8_t)BSP_UART_PORT_COUNT;
}

/**
 * @brief 按序号取端口句柄
 * @param index 序号
 * @retval 非 NULL: 端口句柄
 * @retval NULL: 序号越界
 */
const bsp_uart_t *BSP_UART_Get(uint8_t index)
{
    return (index < BSP_UART_PORT_COUNT) ? s_port_table[index] : NULL;
}

/**
 * @brief 取端口的逻辑编号
 * @param port 串口句柄，可为 NULL
 * @retval 1~BSP_UART_PORT_COUNT: 逻辑编号
 * @retval 0: 句柄为空
 */
uint8_t BSP_UART_Id(const bsp_uart_t *port)
{
    return port ? port->id : 0u;
}

/**
 * @brief 按逻辑编号查端口句柄
 * @param id 逻辑编号
 * @retval 非 NULL: 端口句柄
 * @retval NULL: 编号不存在
 */
const bsp_uart_t *BSP_UART_Find(uint8_t id)
{
    for (uint8_t i = 0; i < BSP_UART_PORT_COUNT; i++)
    {
        if (s_port_table[i]->id == id)
            return s_port_table[i];
    }
    return NULL;
}

/*==============================================================================
 * 事件注册
 *============================================================================*/
/**
 * @brief 注册"收到新字节"通知
 * @param fn 回调函数，NULL 表示取消注册
 * @retval 无
 */
void BSP_UART_Set_Rx_Notify(bsp_uart_notify_t fn)
{
    s_rx_notify = fn;
}

/**
 * @brief 注册"DMA 发送完成"通知
 * @param fn 回调函数，NULL 表示取消注册
 * @retval 无
 */
void BSP_UART_Set_Tx_Done(bsp_uart_notify_t fn)
{
    s_tx_done = fn;
}

/*==============================================================================
 * 接收
 *============================================================================*/
/**
 * @brief 读出并丢弃队列里的字节
 * @param port 串口句柄，不可为 NULL
 * @param dst  目标缓冲区，不可为 NULL
 * @param len  最多读取的字节数
 * @retval 实际读出的字节数
 */
uint32_t BSP_UART_Rx_Read(const bsp_uart_t *port, uint8_t *dst, uint32_t len)
{
    if (!port || !dst)
        return 0u;

    return GFIFO_Read(&port->rt->rx, dst, len);
}

/**
 * @brief 零拷贝查看队首连续可读区间
 * @param[in]  port 串口句柄，不可为 NULL
 * @param[out] len  连续可读字节数，可为 NULL
 * @retval 非 NULL: 队列内部只读指针
 * @retval NULL: 队列为空或参数非法
 */
const uint8_t *BSP_UART_Rx_Peek(const bsp_uart_t *port, uint32_t *len)
{
    const void *span = NULL;
    uint32_t    n    = 0u;

    if (!port)
    {
        if (len)
            *len = 0u;
        return NULL;
    }

    n = GFIFO_Peek_Span(&port->rt->rx, &span);
    if (len)
        *len = n;

    return (n > 0u) ? (const uint8_t *)span : NULL;
}

/**
 * @brief 推进读指针，释放已处理的数据
 * @param port 串口句柄，不可为 NULL
 * @param len  已处理的字节数
 * @retval 无
 */
void BSP_UART_Rx_Skip(const bsp_uart_t *port, uint32_t len)
{
    if (!port || len == 0u)
        return;

    (void)GFIFO_Skip(&port->rt->rx, len);
}

/**
 * @brief 补一次"把 DMA 新字节搬进队列"
 * @param port 串口句柄，不可为 NULL
 * @retval 1: 本次搬进了新字节
 * @retval 0: 没有新字节或句柄非法
 * @note 中断里也会调用同一个搬运函数，两边都会读改 rt->dma_last。
 *       任务侧必须关中断再搬，否则中断插在"已算出增量、还没更新位置"
 *       之间，双方会把同一段字节写两遍。临界区只有一次 ≤128 字节的
 *       memcpy（约 1 µs），不影响中断响应。
 */
uint8_t BSP_UART_Rx_Poll(const bsp_uart_t *port)
{
    uint32_t primask;
    uint8_t  moved;

    if (!port || !port->rt)
        return 0u;

    primask = __get_PRIMASK();
    __disable_irq();
    moved = BSP_UART_Rx_Harvest(port);
    __set_PRIMASK(primask);

    return moved;
}

/**
 * @brief 查询队列中待读字节数
 * @param port 串口句柄，可为 NULL
 * @retval 待读字节数
 */
uint32_t BSP_UART_Rx_Count(const bsp_uart_t *port)
{
    return port ? GFIFO_Used(&port->rt->rx) : 0u;
}

/**
 * @brief 查询因队列满而丢弃的字节累计数
 * @param port 串口句柄，可为 NULL
 * @retval 累计丢弃字节数
 */
uint32_t BSP_UART_Rx_Overflow(const bsp_uart_t *port)
{
    return port ? port->rt->rx_drop : 0u;
}

/*==============================================================================
 * 发送
 *============================================================================*/
/**
 * @brief 启动一次 DMA 发送（非阻塞）
 * @param port 串口句柄，不可为 NULL
 * @param buf  待发送缓冲区，不可为 NULL
 * @param len  发送长度
 * @retval 0: 已启动
 * @retval -1: 参数非法或上次发送未完成
 * @retval -2: HAL 启动失败
 */
int BSP_UART_Tx_Start(const bsp_uart_t *port, const uint8_t *buf, uint16_t len)
{
    UART_HandleTypeDef *hal = BSP_UART_Hal_Of(port);

    if (!hal || !buf || len == 0u || !port->rt)
        return -1;
    if (port->rt->tx_busy)
        return -1;

    port->rt->tx_busy = 1u;
    if (HAL_UART_Transmit_DMA(hal, (uint8_t *)buf, len) != HAL_OK)
    {
        port->rt->tx_busy = 0u;
        return -2;
    }
    return 0;
}

/**
 * @brief 查询 DMA 发送是否进行中
 * @param port 串口句柄，可为 NULL
 * @retval 1: 发送中
 * @retval 0: 空闲或句柄非法
 */
uint8_t BSP_UART_Tx_Is_Busy(const bsp_uart_t *port)
{
    return (port && port->rt) ? port->rt->tx_busy : 0u;
}

/**
 * @brief 阻塞式发送（轮询，不占用 DMA 通道）
 * @param port    串口句柄，不可为 NULL
 * @param buf     待发送缓冲区，不可为 NULL
 * @param len     发送长度
 * @param timeout 超时时间（ms）
 * @retval 0: 全部发出
 * @retval -1: 参数非法或超时
 */
int BSP_UART_Send(const bsp_uart_t *port, const uint8_t *buf, uint16_t len,
                  uint32_t timeout)
{
    UART_HandleTypeDef *hal = BSP_UART_Hal_Of(port);

    if (!hal || !buf || len == 0u)
        return -1;

    return (HAL_UART_Transmit(hal, (uint8_t *)buf, len, timeout) == HAL_OK) ? 0 : -1;
}

/*==============================================================================
 * 中断回调
 *
 * 接收侧两个入口（半满、全满）走同一个搬运函数。
 * 因为增量是按 DMA 计数器算的，重复触发也不会重复入队，
 * 所以不需要区分事件类型，也不需要清任何标志。
 *============================================================================*/
/**
 * @brief HAL 串口 DMA 半满回调
 * @param huart 触发中断的 UART 句柄
 * @retval 无
 * @note 中断上下文：只搬运与通知，不解析、不阻塞。
 */
void HAL_UART_RxHalfCpltCallback(UART_HandleTypeDef *huart)
{
    const bsp_uart_t *port = BSP_UART_Find_By_Hal(huart);

    if (!port)
        return;

    if (BSP_UART_Rx_Harvest(port) && s_rx_notify)
        s_rx_notify(port);
}

/**
 * @brief HAL 串口 DMA 全满回调（循环模式下即缓冲区绕回一圈）
 * @param huart 触发中断的 UART 句柄
 * @retval 无
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    const bsp_uart_t *port = BSP_UART_Find_By_Hal(huart);

    if (!port)
        return;

    if (BSP_UART_Rx_Harvest(port) && s_rx_notify)
        s_rx_notify(port);
}

/**
 * @brief HAL 串口发送完成回调
 * @param huart UART 硬件句柄
 * @retval 无
 * @note 先清忙标志再通知：任务被唤醒时看到的一定是"空闲"。
 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    const bsp_uart_t *port = BSP_UART_Find_By_Hal(huart);

    if (!port)
        return;

    port->rt->tx_busy = 0u;
    if (s_tx_done)
        s_tx_done(port);
}

/**
 * @brief HAL 串口错误回调
 * @param huart UART 硬件句柄
 * @retval 无
 * @note 溢出/噪声错误会让 HAL 结束接收（RxState 回到 READY），
 *       不重启的话这个端口从此再也收不到数据——现场表现为"串口死了"。
 *       所以这里计数后立刻重启接收。
 */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    const bsp_uart_t *port = BSP_UART_Find_By_Hal(huart);

    if (!port)
        return;

    port->rt->err_count++;

    if (huart->RxState == HAL_UART_STATE_READY)
        (void)BSP_UART_Start(port);
}
