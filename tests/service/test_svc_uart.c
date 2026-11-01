/**
 * @file test_svc_uart.c
 * @brief Svc_Uart 链路层的主机端端到端测试
 * @author 李嘉图
 * @date 2026-10-31
 *
 * @note 用假 BSP（内部仍是真的 GFIFO）+ 假 OS 顶替硬件，
 *       验证"字节进队列 → 解包 → 帧队列 → 取出"与"打包 → 发送队列 → 启动 DMA"
 *       两条路径。被测的是 Service 层真正跑的逻辑，不是它的复制品。
 *
 *       在库根目录（RC_Dog/）执行：
 *         gcc -std=c11 -Wall -Wextra -Werror
 *             -I tests/shim -I DogCore/User/Service/Svc_Uart
 *             -I DogCore/User/Service/Svc_Proto -I DogCore/User/BSP/BSP_UART
 *             -I DogCore/User/Common/Common_Utils -I DogCore/User/Third_Party/GFIFO
 *             tests/service/test_svc_uart.c
 *             DogCore/User/Service/Svc_Uart/svc_uart.c
 *             DogCore/User/Service/Svc_Proto/svc_proto.c
 *             DogCore/User/Third_Party/GFIFO/gfifo.c
 *             -o build/test_svc_uart
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "cmsis_os2.h"
#include "gfifo.h"
#include "bsp_uart.h"
#include "svc_proto.h"
#include "svc_uart.h"

/*==============================================================================
 * 断言
 *============================================================================*/
static int s_pass;   /**< 通过的断言数 */
static int s_fail;   /**< 失败的断言数 */

/**
 * @brief 记录一条断言结果
 * @param cond 断言条件
 * @param what 断言描述
 * @retval 无
 */
#define CHECK(cond, what)                                                    \
    do {                                                                     \
        if (cond) {                                                          \
            s_pass++;                                                        \
        } else {                                                             \
            s_fail++;                                                        \
            printf("  [FAIL] %s (第 %d 行)\n", (what), __LINE__);            \
        }                                                                    \
    } while (0)

/*==============================================================================
 * 假 OS
 *============================================================================*/
#define FAKE_Q_DEPTH    8u

/** @brief 假消息队列：元素是 130 字节的发送帧 */
typedef struct {
    svc_uart_tx_frame_t item[FAKE_Q_DEPTH];
    uint8_t head;
    uint8_t tail;
} fake_queue_t;

/** @brief 假端口号队列（RX 通知用，元素 1 字节） */
typedef struct {
    uint8_t item[FAKE_Q_DEPTH];
    uint8_t head;
    uint8_t tail;
} fake_byte_queue_t;

static fake_queue_t      s_tx_q;         /**< 对应 UART_TX_Q */
static fake_byte_queue_t s_rx_q;         /**< 对应 UART_RX_Q */
static uint32_t          s_rx_bs_count;  /**< 对应 UART_RX_BS 的计数 */
static uint32_t          s_tx_cs_count;  /**< 对应 UART_TX_CS 的计数 */
static uint32_t          s_tick;         /**< 假系统节拍 */

/* 头文件 main.h 里 extern 的四个句柄：给它们一个地址即可 */
osMessageQueueId_t UART_RX_QHandle = &s_rx_q;
osMessageQueueId_t UART_TX_QHandle = &s_tx_q;
osSemaphoreId_t    UART_RX_BSHandle = &s_rx_bs_count;
osSemaphoreId_t    UART_TX_CSHandle = &s_tx_cs_count;

/**
 * @brief 假的消息队列写入
 * @param mq_id   队列句柄
 * @param msg_ptr 数据
 * @param msg_prio 优先级（忽略）
 * @param timeout 超时（忽略）
 * @retval osOK 或 osErrorResource
 */
osStatus_t osMessageQueuePut(osMessageQueueId_t mq_id, const void *msg_ptr,
                             uint8_t msg_prio, uint32_t timeout)
{
    (void)msg_prio;
    (void)timeout;

    if (mq_id == (osMessageQueueId_t)&s_tx_q)
    {
        uint8_t next = (uint8_t)((s_tx_q.tail + 1u) % FAKE_Q_DEPTH);

        if (next == s_tx_q.head)
            return osErrorResource;
        s_tx_q.item[s_tx_q.tail] = *(const svc_uart_tx_frame_t *)msg_ptr;
        s_tx_q.tail = next;
        return osOK;
    }

    if (mq_id == (osMessageQueueId_t)&s_rx_q)
    {
        uint8_t next = (uint8_t)((s_rx_q.tail + 1u) % FAKE_Q_DEPTH);

        if (next == s_rx_q.head)
            return osErrorResource;
        s_rx_q.item[s_rx_q.tail] = *(const uint8_t *)msg_ptr;
        s_rx_q.tail = next;
        return osOK;
    }

    return osErrorParameter;
}

/**
 * @brief 假的消息队列读取
 * @param mq_id   队列句柄
 * @param msg_ptr 输出
 * @param msg_prio 优先级输出（忽略）
 * @param timeout 超时：0 表示不等待
 * @retval osOK 或 osErrorTimeout
 */
osStatus_t osMessageQueueGet(osMessageQueueId_t mq_id, void *msg_ptr,
                             uint8_t *msg_prio, uint32_t timeout)
{
    (void)msg_prio;
    (void)timeout;      /* 假 OS 不做真正阻塞，空即超时 */

    if (mq_id == (osMessageQueueId_t)&s_tx_q)
    {
        if (s_tx_q.head == s_tx_q.tail)
            return osErrorTimeout;
        *(svc_uart_tx_frame_t *)msg_ptr = s_tx_q.item[s_tx_q.head];
        s_tx_q.head = (uint8_t)((s_tx_q.head + 1u) % FAKE_Q_DEPTH);
        return osOK;
    }

    if (mq_id == (osMessageQueueId_t)&s_rx_q)
    {
        if (s_rx_q.head == s_rx_q.tail)
            return osErrorTimeout;
        *(uint8_t *)msg_ptr = s_rx_q.item[s_rx_q.head];
        s_rx_q.head = (uint8_t)((s_rx_q.head + 1u) % FAKE_Q_DEPTH);
        return osOK;
    }

    return osErrorParameter;
}

/**
 * @brief 假的信号量获取
 * @param sem_id  信号量句柄
 * @param timeout 超时（忽略）
 * @retval osOK 或 osErrorResource
 */
osStatus_t osSemaphoreAcquire(osSemaphoreId_t sem_id, uint32_t timeout)
{
    (void)timeout;

    if (sem_id == (osSemaphoreId_t)&s_rx_bs_count)
    {
        if (s_rx_bs_count == 0u)
            return osErrorResource;
        s_rx_bs_count--;
        return osOK;
    }

    if (sem_id == (osSemaphoreId_t)&s_tx_cs_count)
    {
        if (s_tx_cs_count == 0u)
            return osErrorResource;
        s_tx_cs_count--;
        return osOK;
    }

    return osErrorParameter;
}

/**
 * @brief 假的信号量释放
 * @param sem_id 信号量句柄
 * @retval osOK
 */
osStatus_t osSemaphoreRelease(osSemaphoreId_t sem_id)
{
    if (sem_id == (osSemaphoreId_t)&s_rx_bs_count)
        s_rx_bs_count = 1u;
    else if (sem_id == (osSemaphoreId_t)&s_tx_cs_count)
        s_tx_cs_count++;

    return osOK;
}

/**
 * @brief 假的系统节拍
 * @retval 当前假节拍
 */
uint32_t osKernelGetTickCount(void)
{
    return s_tick;
}

/*==============================================================================
 * 假 BSP
 *
 * 端口只有一个（bsp_uart4，编号 1）。字节队列用真的 GFIFO，
 * 这样测试覆盖到的就是固件里同一份环形缓冲实现。
 *============================================================================*/
#define FAKE_RX_FIFO_SIZE   512

typedef struct {
    gfifo_t          rx;
    uint8_t          buf[FAKE_RX_FIFO_SIZE];
    bsp_uart_notify_t rx_notify;
    bsp_uart_notify_t tx_done;
    uint8_t          tx_started;      /**< 最近一次启动 DMA 的字节数 */
    uint8_t          tx_buf[160];     /**< 最近一次交给 DMA 的字节 */
    uint32_t         rx_drop;
} fake_port_t;

static fake_port_t s_port;

/**
 * @brief 假端口句柄
 * @note bsp_uart_t 是不透明类型，测试里没法（也不该）实例化；
 *       Service 只是把它原样传回来，所以拿一个真实地址冒充即可。
 */
#define FAKE_PORT   ((const bsp_uart_t *)&s_port)

/**
 * @brief 初始化假端口
 * @retval 0
 */
int BSP_UART_Init(void)
{
    memset(&s_port, 0, sizeof(s_port));
    (void)GFIFO_Init(&s_port.rx, s_port.buf, sizeof(s_port.buf), 1u,
                     GFIFO_FLAG_NONE);
    return 0;
}

/**
 * @brief 启动假端口（无 DMA）
 * @param port 端口句柄
 * @retval 0
 */
int BSP_UART_Start(const bsp_uart_t *port)
{
    (void)port;
    return 0;
}

/**
 * @brief 端口数量
 * @retval 1
 */
uint8_t BSP_UART_Count(void)
{
    return 1u;
}

/**
 * @brief 取端口句柄
 * @param index 序号
 * @retval 句柄或 NULL
 */
const bsp_uart_t *BSP_UART_Get(uint8_t index)
{
    return (index == 0u) ? FAKE_PORT : NULL;
}

/**
 * @brief 取端口编号
 * @param port 端口句柄
 * @retval 1
 */
uint8_t BSP_UART_Id(const bsp_uart_t *port)
{
    (void)port;
    return 1u;
}

/**
 * @brief 假轮询：把测试预置的字节当作"刚从 DMA 搬进来"
 * @param port 端口句柄
 * @retval 无新字节时 0
 */
uint8_t BSP_UART_Rx_Poll(const bsp_uart_t *port)
{
    (void)port;
    return 0u;
}

/**
 * @brief 零拷贝取队首区间
 * @param port 端口句柄
 * @param len  输出长度
 * @retval 指针或 NULL
 */
const uint8_t *BSP_UART_Rx_Peek(const bsp_uart_t *port, uint32_t *len)
{
    const void *span = NULL;
    uint32_t    n;

    (void)port;
    n = GFIFO_Peek_Span(&s_port.rx, &span);
    if (len)
        *len = n;
    return (n > 0u) ? (const uint8_t *)span : NULL;
}

/**
 * @brief 推进读指针
 * @param port 端口句柄
 * @param len  字节数
 * @retval 无
 */
void BSP_UART_Rx_Skip(const bsp_uart_t *port, uint32_t len)
{
    (void)port;
    (void)GFIFO_Skip(&s_port.rx, len);
}

/**
 * @brief 已用字节数
 * @param port 端口句柄
 * @retval 字节数
 */
uint32_t BSP_UART_Rx_Count(const bsp_uart_t *port)
{
    (void)port;
    return GFIFO_Used(&s_port.rx);
}

/**
 * @brief 溢出字节数
 * @param port 端口句柄
 * @retval 字节数
 */
uint32_t BSP_UART_Rx_Overflow(const bsp_uart_t *port)
{
    (void)port;
    return s_port.rx_drop;
}

/**
 * @brief 注册接收通知
 * @param fn 回调
 * @retval 无
 */
void BSP_UART_Set_Rx_Notify(bsp_uart_notify_t fn)
{
    s_port.rx_notify = fn;
}

/**
 * @brief 注册发送完成通知
 * @param fn 回调
 * @retval 无
 */
void BSP_UART_Set_Tx_Done(bsp_uart_notify_t fn)
{
    s_port.tx_done = fn;
}

/**
 * @brief 假启动 DMA 发送：记下字节并立刻回报完成
 * @param port 端口句柄
 * @param buf  数据
 * @param len  长度
 * @retval 0
 */
int BSP_UART_Tx_Start(const bsp_uart_t *port, const uint8_t *buf, uint16_t len)
{
    (void)port;

    if (!buf || len == 0u || len > sizeof(s_port.tx_buf))
        return -1;

    memcpy(s_port.tx_buf, buf, len);
    s_port.tx_started = (uint8_t)len;

    if (s_port.tx_done)
        s_port.tx_done(FAKE_PORT);   /* 模拟 TxCplt 中断 */

    return 0;
}

/**
 * @brief 假忙标志
 * @param port 端口句柄
 * @retval 0
 */
uint8_t BSP_UART_Tx_Is_Busy(const bsp_uart_t *port)
{
    (void)port;
    return 0u;
}

/*==============================================================================
 * 测试辅助
 *============================================================================*/
/**
 * @brief 把字节塞进假端口的接收队列，并触发一次"有数据"通知
 * @param data 数据
 * @param len  长度
 * @retval 无
 */
static void feed(const uint8_t *data, uint32_t len)
{
    (void)GFIFO_Write(&s_port.rx, data, len);

    if (s_port.rx_notify)
        s_port.rx_notify(FAKE_PORT);
}

/**
 * @brief 清空帧队列与信号量
 * @retval 无
 */
static void reset_queues(void)
{
    s_rx_q.head = s_rx_q.tail = 0u;
    s_tx_q.head = s_tx_q.tail = 0u;
    s_rx_bs_count = 1u;          /* CubeMX 生成的初始值 */
    s_tx_cs_count = 0u;
    s_tick = 0u;
}

/*==============================================================================
 * 用例
 *============================================================================*/
/**
 * @brief 用例 1：一帧字节进来，取出来的帧必须一模一样
 * @retval 无
 */
static void test_rx_single(void)
{
    uint8_t           frame[32];
    uint16_t          n;
    svc_proto_frame_t out;

    printf("用例 1 单帧接收\n");

    reset_queues();
    n = Svc_Proto_Build(0x21, (const uint8_t *)"ping", 4, frame, sizeof(frame));
    feed(frame, n);

    Svc_Uart_Rx_Poll(0u);

    CHECK(Svc_Uart_Frame_Get(&out) == 0, "取到一帧");
    CHECK(out.cmd == 0x21 && out.len == 4, "命令字与长度正确");
    CHECK(memcmp(out.payload, "ping", 4) == 0, "载荷正确");
    CHECK(out.id == 1, "帧里带了端口编号");

    /* 统计也应更新 */
    const svc_uart_stat_t *st = Svc_Uart_Stat(1);
    CHECK(st && st->rx_frames == 1 && st->rx_bytes == n, "统计正确");
}

/**
 * @brief 用例 2：两帧粘在一起，一次处理必须出两帧
 * @retval 无
 */
static void test_rx_sticky(void)
{
    uint8_t           buf[64];
    uint16_t          n1;
    uint16_t          n2;
    svc_proto_frame_t out;

    printf("用例 2 粘包\n");

    reset_queues();
    n1 = Svc_Proto_Build(0x01, NULL, 0, buf, sizeof(buf));
    n2 = Svc_Proto_Build(0x02, (const uint8_t *)"ab", 2, buf + n1,
                         sizeof(buf) - n1);
    feed(buf, (uint32_t)(n1 + n2));

    Svc_Uart_Rx_Poll(0u);

    CHECK(Svc_Uart_Frame_Get(&out) == 0 && out.cmd == 0x01, "第一帧");
    CHECK(Svc_Uart_Frame_Get(&out) == 0 && out.cmd == 0x02, "第二帧");
}

/**
 * @brief 用例 3：发送路径把线缆格式原样交给 DMA
 * @retval 无
 */
static void test_tx(void)
{
    uint8_t expect[32];
    uint16_t n;
    uint8_t i;

    printf("用例 3 发送\n");

    reset_queues();
    CHECK(Svc_Uart_Send(1, 0x41, (const uint8_t *)"XY", 2) == 0, "提交成功");

    Svc_Uart_Tx_Pump();

    n = Svc_Proto_Build(0x41, (const uint8_t *)"XY", 2, expect, sizeof(expect));
    CHECK(s_port.tx_started == n, "启动的字节数与打包长度一致");
    for (i = 0; i < n; i++)
        CHECK(s_port.tx_buf[i] == expect[i], "线缆字节逐位一致");
}

/**
 * @brief 用例 4：半帧挂太久要被丢掉，之后一帧仍能正常解出
 * @retval 无
 */
static void test_frame_timeout(void)
{
    uint8_t           frame[32];
    uint16_t          n;
    svc_proto_frame_t out;

    printf("用例 4 半帧超时\n");

    reset_queues();
    n = Svc_Proto_Build(0x55, (const uint8_t *)"xy", 2, frame, sizeof(frame));

    /* 只喂帧头两个字节就撒手 */
    feed(frame, 2u);
    Svc_Uart_Rx_Poll(0u);

    /* 100 ms 后来了一整帧：残帧应先被丢弃，再正常解出新帧 */
    feed(frame, n);
    Svc_Uart_Rx_Poll(100u);

    CHECK(Svc_Uart_Frame_Get(&out) == 0, "取到一帧");
    CHECK(out.cmd == 0x55 && out.len == 2, "内容正确");

    const svc_uart_stat_t *st = Svc_Uart_Stat(1);
    CHECK(st && st->rx_timeout == 1, "记录了一次半帧超时");
}

/*==============================================================================
 * 入口
 *============================================================================*/
/**
 * @brief 测试入口
 * @retval 0: 全部通过
 * @retval 1: 有失败
 */
int main(void)
{
    printf("=== Svc_Uart 链路层测试 ===\n");

    (void)Svc_Uart_Init();

    test_rx_single();
    test_rx_sticky();
    test_tx();
    test_frame_timeout();

    printf("=== 断言 %d 条，失败 %d 条 ===\n", s_pass + s_fail, s_fail);
    return (s_fail == 0) ? 0 : 1;
}
