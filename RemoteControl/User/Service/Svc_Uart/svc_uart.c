/**
 * @file svc_uart.c
 * @brief 串口链路服务实现：字节流解包、帧队列、发送泵
 * @author 李嘉图
 * @date 2026-10-31
 *
 * @note 接收路径：
 *         BSP 中断 ──队列──▶ Svc_Uart_Rx_Poll ──逐字节──▶ Svc_Proto_Feed
 *                                   │                         │
 *                                   └── 成帧 ──GFIFO_Put──▶ 帧队列
 *         CMD 任务 ──Svc_Uart_Frame_Get──▶ APP 命令表
 *
 *       发送路径：
 *         APP ──Svc_Uart_Send──▶ Svc_Proto_Build ──▶ UART_TX_Q（RTOS 队列）
 *         TX 任务 ──Svc_Uart_Tx_Pump──▶ BSP_UART_Tx_Start ──DMA──▶ TxCplt
 *
 *       为什么接收用 GFIFO、发送用 RTOS 队列：
 *         接收是字节流，需要"任意长度、零拷贝、溢出可查"，GFIFO 正合适；
 *         发送是整帧转发，队列本身就能阻塞等待，且 UART_TX_Q 已由 CubeMX
 *         建好（16 × 130 字节），再用一套 FIFO 等于白占一倍的 RAM。
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include <stdint.h>
#include <string.h>
#include "main.h"
#include "common.h"
#include "gfifo.h"
#include "bsp_uart.h"
#include "svc_proto.h"
#include "svc_uart.h"

/*==============================================================================
 * 类型定义
 *============================================================================*/
/**
 * @brief 端口上下文
 */
typedef struct {
    const bsp_uart_t  *port;    /* 对应的 BSP 端口句柄 */
    svc_proto_parser_t parser;  /* 本端口的解包状态机 */
    svc_uart_stat_t    stat;    /* 本端口的收发统计 */
    uint32_t           last_tick; /* 最近一次收到字节的时刻（ms） */
} svc_uart_port_t;

/*==============================================================================
 * 内部变量
 *============================================================================*/
/** @brief 端口上下文表 */
static svc_uart_port_t s_port[BSP_UART_PORT_COUNT];

/** @brief 帧队列存储：必须静态且按元素对齐 */
static GFIFO_ALIGN(8) uint8_t s_frame_buf[SVC_UART_FRAME_DEPTH *
                                           sizeof(svc_proto_frame_t)];
/** @brief 帧队列句柄 */
static gfifo_t s_frame_fifo;

/** @brief 发送暂存帧：只有 TX 任务会碰，放静态区避免占用 512 字节的小栈 */
static svc_uart_tx_frame_t s_tx_frame;

/*==============================================================================
 * 内部函数
 *============================================================================*/
/**
 * @brief 按端口编号查上下文
 * @param id 端口编号（1 起）
 * @retval 非 NULL: 端口上下文
 * @retval NULL: 编号不存在
 */
static svc_uart_port_t *Svc_Uart_Find(uint8_t id)
{
    for (uint8_t i = 0; i < BSP_UART_PORT_COUNT; i++)
    {
        if (BSP_UART_Id(s_port[i].port) == id)
            return &s_port[i];
    }
    return NULL;
}

/**
 * @brief 把一帧推进帧队列
 * @param port  端口上下文，不可为 NULL
 * @param frame 待入队的帧，不可为 NULL
 * @retval 1: 已入队
 * @retval 0: 仍未入队（队列异常）
 * @note 队列满时丢最旧的一帧：控制指令宁可要新的，
 *       也不要因为积压让狗执行几秒前的动作。
 */
static uint8_t Svc_Uart_Push_Frame(svc_uart_port_t *port,
                                   const svc_proto_frame_t *frame)
{
    if (GFIFO_Put(&s_frame_fifo, frame) == GFIFO_OK)
    {
        port->stat.rx_frames++;
        return 1u;
    }

    /* ---------- 队列满：丢最旧，再放新的 ---------- */
    (void)GFIFO_Drop(&s_frame_fifo, 1u);
    port->stat.rx_dropped++;

    if (GFIFO_Put(&s_frame_fifo, frame) == GFIFO_OK)
    {
        port->stat.rx_frames++;
        return 1u;
    }
    return 0u;
}

/**
 * @brief 排空一个端口的字节队列并解包
 * @param port   端口上下文，不可为 NULL
 * @param now_ms 当前系统节拍（ms）
 * @retval 1: 本次至少产出一帧
 * @retval 0: 没有产出新帧
 * @note 用 Peek_Span 零拷贝取连续区间，再逐字节喂状态机；
 *       分片到达、队列回绕都不影响解包结果。
 *       开头先判半帧超时：长度字节收到一半就不再来的话，
 *       解包器会一直等下去（假帧头带一个"看起来合法"的长度时尤其明显），
 *       所以超过 SVC_UART_FRAME_TIMEOUT_MS 就丢掉半帧重新开始。
 */
static uint8_t Svc_Uart_Drain(svc_uart_port_t *port, uint32_t now_ms)
{
    svc_proto_frame_t frame;
    const uint8_t    *span;
    uint32_t          n;
    uint32_t          bytes_before = port->stat.rx_bytes;
    uint8_t           got = 0u;

    /* ---------- 半帧超时：丢掉挂住的残帧 ---------- */
    if ((port->parser.hist_len > 0u) &&
        ((uint32_t)(now_ms - port->last_tick) > SVC_UART_FRAME_TIMEOUT_MS))
    {
        Svc_Proto_Init(&port->parser);
        port->stat.rx_timeout++;
    }

    for (;;)
    {
        n    = 0u;
        span = BSP_UART_Rx_Peek(port->port, &n);
        if (!span || (n == 0u))
            break;

        for (uint32_t i = 0u; i < n; i++)
        {
            int ret = Svc_Proto_Feed(&port->parser, span[i], &frame);

            if (ret > 0)
            {
                frame.id = BSP_UART_Id(port->port);
                if (Svc_Uart_Push_Frame(port, &frame))
                    got = 1u;
            }
            else if (ret < 0)
            {
                port->stat.rx_checksum++;
            }
            else
            {
                /* 0：半包，继续喂 */
            }
        }

        BSP_UART_Rx_Skip(port->port, n);
        port->stat.rx_bytes += n;
    }

    /* 只有真的收到字节才刷新时间戳：别的端口有数据不该推迟本端口的超时 */
    if (port->stat.rx_bytes != bytes_before)
        port->last_tick = now_ms;

    port->stat.rx_overflow = BSP_UART_Rx_Overflow(port->port);
    return got;
}

/*==============================================================================
 * 中断回调（由 BSP 在中断上下文调用）
 *============================================================================*/
/**
 * @brief 收到新字节：投递端口号唤醒 RX 任务
 * @param port 串口句柄
 * @retval 无
 * @note 中断上下文：只投递一个字节，队列满就丢掉这次通知，
 *       数据仍在 BSP 队列里，下一次中断会再唤醒。
 */
static void Svc_Uart_Rx_Isr(const bsp_uart_t *port)
{
    uint8_t id = BSP_UART_Id(port);

    (void)osMessageQueuePut(UART_RX_QHandle, &id, 0u, 0u);
}

/**
 * @brief 发送完成：释放 TX 完成信号量
 * @param port 串口句柄
 * @retval 无
 * @note 中断上下文。先由 BSP 清忙标志，再在这里放信号量，
 *       任务被唤醒时 TX 一定已经空闲。
 */
static void Svc_Uart_Tx_Done_Isr(const bsp_uart_t *port)
{
    (void)port;
    (void)osSemaphoreRelease(UART_TX_CSHandle);
}

/*==============================================================================
 * 初始化
 *============================================================================*/
/**
 * @brief 初始化链路服务并启动所有端口
 * @retval 0: 成功
 * @retval -1: BSP 初始化、队列初始化或 DMA 启动失败
 */
int Svc_Uart_Init(void)
{
    if (BSP_UART_Init() != 0)
        return -1;

    if (GFIFO_Init(&s_frame_fifo, s_frame_buf, sizeof(s_frame_buf),
                   sizeof(svc_proto_frame_t), GFIFO_FLAG_NONE) != GFIFO_OK)
        return -1;

    for (uint8_t i = 0; i < BSP_UART_PORT_COUNT; i++)
    {
        s_port[i].port = BSP_UART_Get(i);
        if (!s_port[i].port)
            return -1;

        Svc_Proto_Init(&s_port[i].parser);
        memset(&s_port[i].stat, 0, sizeof(s_port[i].stat));
        s_port[i].last_tick = 0u;
    }

    /* 先挂回调再开 DMA：否则开起来的第一批字节只会更新计数，不会唤醒任务 */
    BSP_UART_Set_Rx_Notify(Svc_Uart_Rx_Isr);
    BSP_UART_Set_Tx_Done(Svc_Uart_Tx_Done_Isr);

    for (uint8_t i = 0; i < BSP_UART_PORT_COUNT; i++)
    {
        if (BSP_UART_Start(s_port[i].port) != 0)
            return -1;
    }

    return 0;
}

/*==============================================================================
 * 接收
 *============================================================================*/
/**
 * @brief 等一次接收通知并排空所有端口
 * @param now_ms 当前系统节拍（ms）
 * @retval 无
 * @note 队列等待带 SVC_UART_RX_POLL_MS 超时：慢速小包不会触发半满/全满
 *       中断，只能靠这个周期把 DMA 里的增量搬进队列。
 *       两个端口都搬、都解，通知只当作"可能更快"的提示。
 */
void Svc_Uart_Rx_Poll(uint32_t now_ms)
{
    uint8_t id;
    uint8_t got = 0u;

    /* 超时返回也继续往下走：这可能就是唯一的搬运时机 */
    (void)osMessageQueueGet(UART_RX_QHandle, &id, NULL, SVC_UART_RX_POLL_MS);

    for (uint8_t i = 0; i < BSP_UART_PORT_COUNT; i++)
    {
        /* 补搬一次 DMA 增量（中断里搬过就不会再搬，幂等） */
        (void)BSP_UART_Rx_Poll(s_port[i].port);

        if (Svc_Uart_Drain(&s_port[i], now_ms))
            got = 1u;
    }

    /* 有帧才通知 CMD 任务，避免空转 */
    if (got)
        (void)osSemaphoreRelease(UART_RX_BSHandle);
}

/**
 * @brief 取一帧已解出的数据（阻塞）
 * @param frame 输出缓冲，不可为 NULL
 * @retval 0: 取到一帧
 * @retval -1: 参数非法
 */
int Svc_Uart_Frame_Get(svc_proto_frame_t *frame)
{
    if (!frame)
        return -1;

    for (;;)
    {
        if (GFIFO_Get(&s_frame_fifo, frame) == GFIFO_OK)
            return 0;

        /* 队列空：等"有帧"通知。信号量可能先于队列被消耗，
           取不到就再等一次，不会丢帧也不会空转。 */
        if (osSemaphoreAcquire(UART_RX_BSHandle, osWaitForever) != osOK)
            return -1;
    }
}

/*==============================================================================
 * 发送
 *============================================================================*/
/**
 * @brief 打包并提交一帧发送
 * @param id      目标端口编号
 * @param cmd     命令字
 * @param payload 载荷，len 为 0 时可为 NULL
 * @param len     载荷长度
 * @retval 0: 已提交
 * @retval -1: 参数非法或队列满
 */
int Svc_Uart_Send(uint8_t id, uint8_t cmd, const uint8_t *payload, uint8_t len)
{
    svc_uart_port_t *ctx = Svc_Uart_Find(id);
    svc_uart_tx_frame_t frame;
    uint16_t full_len;

    full_len = Svc_Proto_Build(cmd, payload, len, frame.data, sizeof(frame.data));
    if (full_len == 0u)
        return -1;

    frame.id  = id;
    frame.len = (uint8_t)full_len;

    if (osMessageQueuePut(UART_TX_QHandle, &frame, 0u, 0u) != osOK)
    {
        if (ctx)
            ctx->stat.tx_dropped++;
        return -1;
    }

    if (ctx)
        ctx->stat.tx_frames++;

    return 0;
}

/**
 * @brief 取一帧并交给 DMA 发出，等发送完成（阻塞）
 * @retval 无
 */
void Svc_Uart_Tx_Pump(void)
{
    svc_uart_port_t *ctx;

    if (osMessageQueueGet(UART_TX_QHandle, &s_tx_frame, NULL,
                          osWaitForever) != osOK)
        return;

    ctx = Svc_Uart_Find(s_tx_frame.id);
    if (!ctx)
        return;

    /* 清掉可能残留的完成信号，否则这一帧会误以为"立刻就发完了" */
    (void)osSemaphoreAcquire(UART_TX_CSHandle, 0u);

    if (BSP_UART_Tx_Start(ctx->port, s_tx_frame.data, s_tx_frame.len) != 0)
    {
        ctx->stat.tx_timeout++;
        return;
    }

    if (osSemaphoreAcquire(UART_TX_CSHandle, SVC_UART_TX_WAIT_MS) != osOK)
        ctx->stat.tx_timeout++;
}

/*==============================================================================
 * 诊断
 *============================================================================*/
/**
 * @brief 取指定端口的统计
 * @param id 端口编号（1 起）
 * @retval 非 NULL: 统计结构（只读）
 * @retval NULL: 端口不存在
 */
const svc_uart_stat_t *Svc_Uart_Stat(uint8_t id)
{
    svc_uart_port_t *ctx = Svc_Uart_Find(id);

    return ctx ? &ctx->stat : NULL;
}
