/**
 * @file app_uart.c
 * @brief 串口应用模块
 * @author 李嘉图
 * @date 2026-06-20
 *
 * @note 架构说明：
 *       接收：DMA 循环缓冲（APP 所有）→ 拼包 → Codec 解析
 *             → 共享数据池（字节）→ 描述符 FIFO（6B/项）
 *       发送：Build → 编码为线缆格式 → RTOS 消息队列
 *             → TX 任务 → DMA 发送
 *
 *       中断路径零拷贝，所有数据处理在任务上下文完成。
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 标准库
#include <stdint.h>
#include <string.h>
// 固定包含
#include "main.h"
#include "app_uart.h"
#include "common.h"
// 功能包含
#include "app_uart_cmd.h"

/*==============================================================================
 * 端口上下文池
 *
 * @note APP_UART_PORT_IDS 为编译期常量，定义哪些串口需要初始化。
 *       新增串口只需在此数组加一项，同时在 bsp_uart.c 映射表加对应项。
 *============================================================================*/

/** @brief APP 层管理的串口 ID 列表（运行时不受理热注册） */
static const uint8_t APP_UART_PORT_IDS[] = { 1 };

static APP_UART_Port_t app_port_pool[ARRAY_SIZE(APP_UART_PORT_IDS)];
static uint8_t app_port_count = 0;

/*==============================================================================
 * 双缓冲发送上下文
 *
 * 每端口两个 DMA 缓冲区（ping-pong），
 * 硬件发 buf[A] 的同时任务可预装 buf[B]，
 * TX 完成 ISR 自动切到预装缓冲，消除帧间间隙。
 *============================================================================*/

/** @brief 每端口双缓冲 TX 状态 */
typedef struct {
    uint8_t  buf[2][APP_TX_BUF_SIZE];   // 两个 DMA 发送缓冲区
    uint8_t  len[2];                     // 各缓冲区数据长度
    uint8_t  active;                     // 当前 DMA 正在发送的缓冲区号（0/1）
    uint8_t  preloaded;                  // 1 = 非活跃缓冲区已有预装数据
    volatile uint8_t in_progress;        // 1 = 有 DMA 发送正在进行
} APP_TxDualBuf_t;

/** @brief 双缓冲上下文池 */
static APP_TxDualBuf_t tx_dual_pool[ARRAY_SIZE(APP_UART_PORT_IDS)];

/*==============================================================================
 * 共享接收数据池
 *
 * 环形字节池：所有串口的完整包 payload 按顺序写入，
 * 处理完后顺序读出释放。wr 和 rd 均为顺序推进+回绕。
 *============================================================================*/
static uint8_t  app_rx_pool[APP_RX_POOL_SIZE];
static uint16_t app_rx_pool_wr = 0;     // 写指针（入池位置）
static uint16_t app_rx_pool_rd = 0;     // 读指针（已处理位置）

/**
 * @brief 计算池中可用写入空间
 * @return 空闲字节数
 */
static uint16_t app_rx_pool_free(void)
{
    if (app_rx_pool_wr >= app_rx_pool_rd)
        return APP_RX_POOL_SIZE - (app_rx_pool_wr - app_rx_pool_rd);
    else
        return app_rx_pool_rd - app_rx_pool_wr;
}

/**
 * @brief 向共享池写入数据（环形处理回绕）
 * @param data  源数据
 * @param len   数据长度
 * @return 写入起始偏移，写失败返回 0xFFFF
 */
static uint16_t app_rx_pool_write(const uint8_t *data, uint16_t len)
{
    if (len > app_rx_pool_free() - 1)
        return 0xFFFF;  /* 空间不足 */

    uint16_t pos = app_rx_pool_wr;
    uint16_t first = APP_RX_POOL_SIZE - app_rx_pool_wr;

    if (first >= len)
    {
        memcpy(&app_rx_pool[app_rx_pool_wr], data, len);
    }
    else
    {
        memcpy(&app_rx_pool[app_rx_pool_wr], data, first);
        memcpy(&app_rx_pool[0], &data[first], len - first);
    }

    app_rx_pool_wr = (app_rx_pool_wr + len) % APP_RX_POOL_SIZE;
    return pos;
}

/**
 * @brief 从共享池读取数据（环形处理回绕）
 * @param offset 起始偏移
 * @param dst    目标缓冲区
 * @param len    读取长度
 */
static void app_rx_pool_read(uint16_t offset, uint8_t *dst, uint16_t len)
{
    uint16_t first = APP_RX_POOL_SIZE - offset;

    if (first >= len)
    {
        memcpy(dst, &app_rx_pool[offset], len);
    }
    else
    {
        memcpy(dst, &app_rx_pool[offset], first);
        memcpy(dst + first, &app_rx_pool[0], len - first);
    }
}

/**
 * @brief 推进池读指针（释放已处理数据）
 * @param len 释放字节数
 */
static void app_rx_pool_advance(uint16_t len)
{
    app_rx_pool_rd = (app_rx_pool_rd + len) % APP_RX_POOL_SIZE;
}

/*==============================================================================
 * 描述符 FIFO 操作（共享，跨所有串口）
 *============================================================================*/
static APP_RxDescFIFO_t rx_desc_fifo = {0};

/**
 * @brief 描述符入队
 * @param desc 描述符指针
 * @retval 0: 成功
 * @retval -1: 队列满
 */
static int rx_desc_push(const APP_RxDesc_t *desc)
{
    uint8_t next = (rx_desc_fifo.tail + 1) % APP_RX_DESC_MAX;
    if (next == rx_desc_fifo.head)
        return -1;

    rx_desc_fifo.buffer[rx_desc_fifo.tail] = *desc;
    rx_desc_fifo.tail = next;
    return 0;
}

/**
 * @brief 描述符出队
 * @param desc 输出缓冲区
 * @retval 0: 成功
 * @retval -1: 队列空
 */
static int rx_desc_pop(APP_RxDesc_t *desc)
{
    if (rx_desc_fifo.head == rx_desc_fifo.tail)
        return -1;

    *desc = rx_desc_fifo.buffer[rx_desc_fifo.head];
    rx_desc_fifo.head = (rx_desc_fifo.head + 1) % APP_RX_DESC_MAX;
    return 0;
}

/*==============================================================================
 * 内部函数
 *============================================================================*/
/**
 * @brief 查找端口实例
 * @param id 串口编号
 * @return 端口指针，NULL 表示未找到
 */
static APP_UART_Port_t *app_find_port(uint8_t id)
{
    for (uint8_t i = 0; i < app_port_count; i++)
    {
        if (app_port_pool[i].id == id)
            return &app_port_pool[i];
    }
    return NULL;
}

/**
 * @brief 查找端口在池中的索引
 * @param id 串口编号
 * @return 索引，0xFF 表示未找到
 */
static uint8_t app_port_index(uint8_t id)
{
    for (uint8_t i = 0; i < app_port_count; i++)
    {
        if (app_port_pool[i].id == id)
            return i;
    }
    return 0xFF;
}

/**
 * @brief 判断接收超时
 * @param last 最后接收时间戳
 * @retval 1: 超时
 * @retval 0: 未超时
 */
static inline uint8_t app_is_timeout(uint32_t last)
{
    return (osKernelGetTickCount() - last) > APP_RX_TIMEOUT_MS;
}

/**
 * @brief 从 DMA 循环缓冲区读取新数据（直接写入目标位置）
 * @param port    端口实例
 * @param dst     目标缓冲区（可直接传 rx_buf + rx_len）
 * @param max_len 目标缓冲区剩余空间
 * @return 实际读取字节数
 * @note 自动处理回绕，最多读 max_len 字节。
 *       rx_last_pos 更新方式：读到多少就推进多少。
 */
static uint16_t app_read_dma_data(APP_UART_Port_t *port, uint8_t *dst, uint16_t max_len)
{
    uint16_t cur_pos = BSP_UART_GetDMAPos(port->id);
    uint16_t last = port->rx_last_pos;

    if (cur_pos == last || max_len == 0)
        return 0;

    /* 可用数据量（DMA 已写入但未读） */
    uint16_t avail;
    if (cur_pos > last)
        avail = cur_pos - last;
    else
        avail = (APP_RX_DMA_BUF_SIZE - last) + cur_pos;

    if (avail > max_len)
        avail = max_len;

    /* 直接拷贝，处理回绕 */
    if (cur_pos > last)
    {
        /* 顺序：一次拷贝 */
        memcpy(dst, &port->rx_dma_buf[last], avail);
    }
    else
    {
        /* 回绕：分两段 */
        uint16_t part1 = APP_RX_DMA_BUF_SIZE - last;
        if (part1 > avail) part1 = avail;
        memcpy(dst, &port->rx_dma_buf[last], part1);
        if (avail > part1)
            memcpy(dst + part1, &port->rx_dma_buf[0], avail - part1);
    }

    /* 读到多少推进多少（不是跳到 cur_pos，避免丢掉衔接处的数据） */
    port->rx_last_pos = (last + avail) % APP_RX_DMA_BUF_SIZE;
    return avail;
}

/*==============================================================================
 * 前向声明
 *============================================================================*/
/** @brief TX 完成 ISR 回调（实现在双缓冲段落） */
static void on_uart_tx_complete(uint8_t id);

/*==============================================================================
 * 初始化函数
 *============================================================================*/
/**
 * @brief APP 层初始化
 * @note 初始化 BSP、所有端口上下文，配置 DMA 循环接收。
 *       端口 ID 由内部编译期数组 APP_UART_PORT_IDS 定义。
 */
void APP_UART_Init(void)
{
    /* 1. 初始化 BSP */
    BSP_UART_Init();

    /* 消耗 CubeMX 生成的初始计数值（初始=1→0，适配 ISR 信号模式） */
    osSemaphoreAcquire(UART_RX_BSHandle, 0);

    /* 2. 清空端口池 */
    memset(app_port_pool, 0, sizeof(app_port_pool));
    app_port_count = 0;

    /* 3. 清空共享池和描述符 FIFO */
    app_rx_pool_wr = 0;
    app_rx_pool_rd = 0;
    rx_desc_fifo.head = 0;
    rx_desc_fifo.tail = 0;

    /* 4. 初始化所有端口并启动 DMA 循环接收 */
    for (uint8_t i = 0; i < ARRAY_SIZE(APP_UART_PORT_IDS); i++)
    {
        uint8_t id = APP_UART_PORT_IDS[i];

        APP_UART_Port_t *port = &app_port_pool[app_port_count];
        port->id = id;
        port->rx_len = 0;
        port->rx_last_pos = 0;
        port->last_rx_time = 0;

        /* 配置 BSP DMA 循环接收（注入 APP 的缓冲区） */
        BSP_UART_ConfigDMARx(id, port->rx_dma_buf, APP_RX_DMA_BUF_SIZE);

        app_port_count++;
    }

    /* 5. 注册 TX 完成回调（ISR → 双缓冲切换） */
    BSP_UART_SetTxCpltFn(on_uart_tx_complete);
}

/*==============================================================================
 * 接收处理
 *============================================================================*/
/**
 * @brief 从 DMA 循环缓冲读取新数据并解析
 * @param id 串口编号
 *
 * @note 处理流程：
 *       1. 从 DMA 缓冲读取新字节（处理回绕）
 *       2. 拼入 rx_buf（处理超时半包丢弃）
 *       3. Codec 解析完整包
 *       4. 完整包 payload → 共享数据池
 *       5. 描述符 → FIFO
 */
void APP_UART_ProcessRxData(uint8_t id)
{
    APP_UART_Port_t *port = app_find_port(id);
    if (!port)
        return;

    /* 1. 超时半包丢弃 */
    if (port->rx_len > 0 && app_is_timeout(port->last_rx_time))
        port->rx_len = 0;

    /* 2. 从 DMA 直接读入 rx_buf（无临时缓冲，省栈） */
    {
        uint16_t space = APP_RX_BUF_MAX - port->rx_len;
        if (space == 0)
            return;

        uint16_t n = app_read_dma_data(port, port->rx_buf + port->rx_len, space);
        if (n == 0)
            return;     /* 无新数据 */

        port->rx_len += n;
        port->last_rx_time = osKernelGetTickCount();
    }

    /* 3. 循环解析完整包 */
    uint16_t total_consumed = 0;

    while (1)
    {
        Codec_Packet_t pkt;
        uint16_t consumed = 0;

        int ret = Codec_ParseRxPacket(
            port->rx_buf + total_consumed,
            port->rx_len - total_consumed,
            &consumed, &pkt);

        if (ret == -1)
            break;      /* 参数非法 */

        if (ret == 0)
        {
            /* 半包：保留剩余数据等待后续 */
            total_consumed += consumed;
            break;
        }

        /* 完整包：payload 写入共享池 */
        pkt.id = id;
        uint16_t data_len = (pkt.len > 5) ? (pkt.len - 5) : 0;   /* 数据长度 */

        uint16_t pool_off = app_rx_pool_write(pkt.payload, data_len);
        if (pool_off == 0xFFFF)
        {
            /* 池满 → 停止解析 */
            break;
        }

        /* 描述符入 FIFO */
        APP_RxDesc_t desc;
        desc.offset = pool_off;
        desc.len    = data_len;
        desc.cmd    = pkt.cmd;
        desc.id     = id;

        if (rx_desc_push(&desc) != 0)
        {
            /* FIFO 满 → 无法消费，回滚池写入 */
            /* 简单处理：直接停止解析 */
            break;
        }

        total_consumed += consumed;

        if (ret == 1)
            break;      /* 最后一个包 */
        /* ret == 2 → 继续解析 */
    }

    /* 4. memmove 剩余数据到 rx_buf 头部 */
    if (total_consumed > 0 && total_consumed <= port->rx_len)
    {
        memmove(port->rx_buf,
                port->rx_buf + total_consumed,
                port->rx_len - total_consumed);
        port->rx_len -= total_consumed;
    }
}

/**
 * @brief 处理一个接收数据包
 * @retval 1: 成功处理一个包
 * @retval 0: 无包可处理
 */
uint8_t APP_UART_SendRxPacket(void)
{
    APP_RxDesc_t desc;

    if (rx_desc_pop(&desc) != 0)
        return 0;

    /* 从共享池读取 payload */
    uint8_t pool_data[APP_TX_BUF_SIZE];
    uint16_t data_len = desc.len;           /* 数据负载长度 */

    if (data_len > 0)
    {
        if (data_len > APP_TX_BUF_SIZE)
            data_len = APP_TX_BUF_SIZE;
        app_rx_pool_read(desc.offset, pool_data, data_len);
    }

    /* 推进池读指针释放空间 */
    app_rx_pool_advance(data_len);

    /* 调用命令处理 */
    APP_UART_Cmd(desc.cmd, (data_len > 0) ? pool_data : NULL, data_len);

    return 1;
}

/*==============================================================================
 * 双缓冲发送
 *============================================================================*/
/**
 * @brief TX 完成 ISR 回调（由 BSP 注册，在中断上下文中调用）
 * @param id 已完成发送的串口 ID
 * @note 若有预装数据则立即切缓冲并启动下一轮 DMA，
 *       然后发标记帧通知 TX 任务去装填刚释放的缓冲。
 */
static void on_uart_tx_complete(uint8_t id)
{
    uint8_t idx = app_port_index(id);
    if (idx == 0xFF) return;

    APP_TxDualBuf_t *db = &tx_dual_pool[idx];
    db->in_progress = 0;

    if (db->preloaded)
    {
        /* 切到预装缓冲，立即启动 DMA */
        db->active = 1 - db->active;
        db->preloaded = 0;
        db->in_progress = 1;
        BSP_UART_SendDMA(id, db->buf[db->active], db->len[db->active]);
    }

    /* 发标记帧唤醒 TX 任务（让其尝试装填刚释放的缓冲） */
    APP_TxFrame_t marker;
    marker.id  = id;
    marker.len = 0;
    osMessageQueuePut(UART_TX_QHandle, &marker, 0, 0);
}

/**
 * @brief 双缓冲发送：尝试发送或预装一帧
 * @param frame 待发送帧（函数内部不会修改）
 * @param idx   端口在 tx_dual_pool 中的索引
 * @retval 1: 已发送或预装成功
 * @retval 0: 双缓冲均忙，未处理
 */
static uint8_t app_dual_try_send(const APP_TxFrame_t *frame, uint8_t idx)
{
    APP_TxDualBuf_t *db = &tx_dual_pool[idx];

    if (!db->in_progress)
    {
        /* DMA 空闲 → 直接发送 */
        db->active = 0;
        db->len[0] = frame->len;
        memcpy(db->buf[0], frame->data, frame->len);
        db->in_progress = 1;
        BSP_UART_SendDMA(frame->id, db->buf[0], frame->len);
        return 1;
    }

    if (!db->preloaded)
    {
        /* DMA 正在发送，预装另一缓冲 */
        uint8_t pre = 1 - db->active;
        db->len[pre] = frame->len;
        memcpy(db->buf[pre], frame->data, frame->len);
        db->preloaded = 1;
        return 1;
    }

    return 0;   /* 双缓冲均忙 */
}

/**
 * @brief TX 完成处理（由 TX 任务收到标记帧后调用）
 * @param id 已完成发送的串口 ID
 * @note 尝试从队列取帧装填刚释放的缓冲，
 *       实现 ISR 切缓冲后任务立即预装下一帧。
 */
void APP_UART_OnTxComplete(uint8_t id)
{
    uint8_t idx = app_port_index(id);
    if (idx == 0xFF) return;

    /* 尝试从队列取帧 → 装填空闲缓冲 */
    APP_TxFrame_t next;
    while (osMessageQueueGet(UART_TX_QHandle, &next, NULL, 0) == osOK)
    {
        if (next.len == 0)
            continue;                   /* 跳过其他标记帧 */

        if (app_dual_try_send(&next, idx))
            return;                     /* 成功装填本次释放的缓冲 */

        /* 装填失败（可能目标端口不同），放回等下次 */
        osMessageQueuePut(UART_TX_QHandle, &next, 0, 0);
        return;
    }
}

/**
 * @brief 双缓冲发送一帧（TX 任务入口）
 * @param frame 待发送帧
 * @note 若 DMA 空闲则立即发，若忙但单缓冲空闲则预装，
 *       若双缓冲均忙则放回队列末尾等下次通知。
 */
void APP_UART_TrySendDual(const APP_TxFrame_t *frame)
{
    uint8_t idx = app_port_index(frame->id);
    if (idx == 0xFF)
        return;

    if (app_dual_try_send(frame, idx))
        return;     /* 已发送或预装成功 */

    /* 双缓冲均忙 → 放回队尾 */
    osMessageQueuePut(UART_TX_QHandle, frame, 0, 0);
    osDelay(1);
}

/*==============================================================================
 * 发送函数
 *============================================================================*/
/**
 * @brief 打包并推入发送队列
 * @param id       目标串口 ID
 * @param cmd      命令字
 * @param data     payload 指针（data_len == 0 时可为 NULL）
 * @param data_len 数据负载长度（0~123）
 * @retval 1: 成功入队
 * @retval 0: 失败
 */
uint8_t APP_UART_BuildTxPacket(uint8_t id, uint8_t cmd,
                                const uint8_t *data, uint8_t data_len)
{
    /* 1. Codec 打包（传入 data_len，内部拼全长） */
    Codec_Packet_t pkt = Codec_BuildTxPacket(id, cmd, data, data_len);
    if (pkt.len == (uint8_t)-1)
        return 0;

    uint8_t full_len = pkt.len;             /* 包全长 = data_len + 5 */

    APP_TxFrame_t frame;
    frame.id  = id;
    frame.len = full_len;

    frame.data[0] = PACKET_HEAD1;
    frame.data[1] = PACKET_HEAD2;
    frame.data[2] = full_len;                   /* 包全长 */
    frame.data[3] = pkt.cmd;

    if (data_len > 0)
        memcpy(&frame.data[4], pkt.payload, data_len);

    frame.data[4 + data_len] = pkt.chk;

    if (osMessageQueuePut(UART_TX_QHandle, &frame, 0, 0) != osOK)
        return 0;

    return 1;
}

/**
 * @brief 发送一帧数据
 * @param frame 已编码的帧
 * @note TX 任务使用此函数代替直接调用 BSP_SendDMA
 */
void APP_UART_SendFrame(const APP_TxFrame_t *frame)
{
    if (!frame || frame->len == 0)
        return;
    BSP_UART_SendDMA(frame->id, frame->data, frame->len);
}
