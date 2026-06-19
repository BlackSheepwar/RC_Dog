/**
 * @file app_uart.c
 * @brief 串口应用模块
 * @author 李嘉图
 * @date 2026-5-4
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
 * 数据包FIFO队列和相关工具函数
 *============================================================================*/
static APP_UART_PacketFIFO_t rxpacketFIFO = {0};   // 接收队列
static APP_UART_PacketFIFO_t txPacketFIFO = {0};   // 发送队列

/**
 * @brief  FIFO 入队（Push）
 * @param  fifo  队列实例
 * @param  pkt   指向待写入数据包（只读）
 * @return  0:  写入成功
 * @return -1:  队列已满
 * @note   本函数只修改 tail，可安全在中断中调用（单生产者场景）
 */
static int APP_UART_PacketFIFO_Push(APP_UART_PacketFIFO_t *fifo, const Codec_Packet_t *pkt)
{
    if (!fifo || !pkt)
        return -1;

    uint8_t next_tail = (fifo->tail + 1) % PACKET_FIFO_SIZE;
    if (next_tail == fifo->head)   // 队列满（tail + 1 追上 head）
        return -1;

    fifo->buffer[fifo->tail] = *pkt;
    fifo->tail = next_tail;
    return 0;
}

/**
 * @brief  FIFO 出队（Pop）
 * @param  fifo  队列实例
 * @param  pkt   指向用于存放读出数据包的缓冲区
 * @return  0:  写入成功
 * @return -1:  队列已满
 * @note   本函数只修改 head，与 Push 无共享写变量，无需临界区
 */
static int APP_UART_PacketFIFO_Pop(APP_UART_PacketFIFO_t *fifo, Codec_Packet_t *pkt)
{
    if (!fifo || !pkt)
        return -1;

    if (fifo->head == fifo->tail)   // 队列空
        return -1;

    *pkt = fifo->buffer[fifo->head];
    fifo->head = (fifo->head + 1) % PACKET_FIFO_SIZE;
    return 0;
}

/*==============================================================================
 * 数据包解析静态池
 *============================================================================*/
static APP_UART_t app_port_pool[UART_MAX_PORTS];
static uint8_t app_port_count = 0;

/**
 * @brief 通过 ID 查找解包实例（轮询）
 * @param id 串口编号
 * @return 非NULL：找到返回指针
 * @return NULL：未找到
 */
static APP_UART_t *app_find_port_by_id(uint8_t id)
{
    for (uint8_t i = 0; i < app_port_count; i++) {
        if (app_port_pool[i].id == id) {
            return &app_port_pool[i];
        }
    }
    return NULL;
}

/*==============================================================================
 * 初始化函数
 *============================================================================*/
/**
 * @brief APP初始化
 * @note 串口注册
 */
void APP_UART_Init(void)
{
    /* 1. 初始化底层串口 */
    BSP_UART_Init();

    /* 2. 清空FIFO（防止复位残留） */
    rxpacketFIFO.head = 0;
    rxpacketFIFO.tail = 0;

    txPacketFIFO.head = 0;
    txPacketFIFO.tail = 0;

    /* 3. 清空端口池 */
    memset(app_port_pool, 0, sizeof(app_port_pool));
    app_port_count = 0;
}

/**
 * @brief 注册一个 APP 串口解析实例
 * @param id    串口编号（需与 BSP 层一致）
 * @param huart UART 硬件句柄（DMA句柄从 huart->hdmarx 自动获取）
 * @retval 1：注册成功
 * @retval 0：注册失败
 */
uint8_t APP_UART_RegisterPort(uint8_t id, UART_HandleTypeDef *huart)
{
    // 检查池是否已满
    if (app_port_count >= UART_MAX_PORTS) return 0;
    // 检查 id 是否已被注册
    if (app_find_port_by_id(id) != NULL) return 0;

    APP_UART_t *port = &app_port_pool[app_port_count];
    port->id     = id;
    port->rx_len = 0;
    memset(port->rx_buf, 0, RX_BUF_MAX);
    // last_rx_time 会在首次收到数据时更新，这里可设 0
    port->last_rx_time = 0;

    // 在底层 BSP 注册
    BSP_UART_RegisterPort(id, huart);

    app_port_count++;
    return 1;
}

/*==============================================================================
 * 数据包消费函数
 *============================================================================*/
/**
 * @brief 数据包处理
 * @note 每次调用仅处理队列头的一个数据包
 * @retval 1：成功消费一个数据包
 * @retval 0：没有数据包可消费
 */
uint8_t APP_UART_SendRxPacket(void)
{
    Codec_Packet_t pkt;
    // 调用 FIFO_Pop 函数完成出队
    if (APP_UART_PacketFIFO_Pop(&rxpacketFIFO, &pkt) != 0) return 0; // 队列空
    APP_UART_Cmd(pkt.cmd, pkt.payload, pkt.len);
    return 1;
}

 /**
 * @brief 判断是否发生接收超时
 * @param last 上次接收时间
 * @retval 1：超时
 * @retval 0：未超时
 */
static inline uint8_t APP_UART_IsTimeout(uint32_t last)
{
    return (osKernelGetTickCount() - last) > UART_RX_TIMEOUT_MS;
}

/**
 * @brief 解包并推入接收 FIFO 队列（基于单包解析器）
 * @param id 串口编号
 */
void APP_UART_BuildRxPacket(uint8_t id)
{
    /* 1. 查找端口实例 */
    APP_UART_t *app = app_find_port_by_id(id);
    if (app == NULL) {
        return;
    }

    /* 2. 超时处理（半包丢弃） */
    if (app->rx_len > 0 && APP_UART_IsTimeout(app->last_rx_time)) {
        app->rx_len = 0;
    }

    /* 3. 从 BSP 读取数据 */
    uint8_t tmp_buf[UART_BUF_SIZE];
    while (1) {
        uint16_t space = RX_BUF_MAX - app->rx_len;
        if (space == 0) break;

        uint16_t to_read = (space < UART_BUF_SIZE) ? space : UART_BUF_SIZE;
        uint16_t n = BSP_UART_ReadRawData(id, tmp_buf, to_read);
        if (n == 0) break;

        memcpy(app->rx_buf + app->rx_len, tmp_buf, n);
        app->rx_len += n;
        app->last_rx_time = osKernelGetTickCount();
    }

    /* 4. 循环解析 */
    uint16_t total_consumed = 0;
    while (1)
    {
        Codec_Packet_t pkt;
        uint16_t consumed = 0;

        int ret = Codec_ParseRxPacket(app->rx_buf + total_consumed,
                                       app->rx_len - total_consumed,
                                       &consumed,
                                       &pkt);

        if (ret == -1) {
            break;
        }

        if (ret == 0) {
            // 没有完整包（可能是半包）
            total_consumed += consumed;
            break;
        }

        /* 推入FIFO */
        pkt.id = id;

        if (APP_UART_PacketFIFO_Push(&rxpacketFIFO, &pkt) != 0) {
            // FIFO满 → 停止解析（不能继续消费）
            break;
        }

        total_consumed += consumed;

        if (ret == 1) {
            // 最后一个包
            break;
        }

        // ret == 2 → 继续解析，但不 memmove
    }

    /* ===== 统一 memmove（只执行一次）===== */
    if (total_consumed > 0 && total_consumed <= app->rx_len)
    {
        memmove(app->rx_buf,
                app->rx_buf + total_consumed,
                app->rx_len - total_consumed);

        app->rx_len -= total_consumed;
    }
}
/*==============================================================================
 * 发送数据包函数
 *============================================================================*/
 /**
 * @brief 数据包发送调度器
 */
void APP_UART_TxScheduler(uint8_t id)
{
    if (BSP_UART_ReadTxBusy(id) == 0)
    {
        osSemaphoreRelease(UART_TX_BSHandle);
    }
}

 /**
 * @brief 发送 FIFO 队列的数据包，一次发送一个数据包
 * @note  从 txPacketFIFO 中取出一个包，组装成完整帧并通过底层发送
 */
void APP_UART_SendTxPacket(void)
{
    Codec_Packet_t pkt;
    if (APP_UART_PacketFIFO_Pop(&txPacketFIFO, &pkt) != 0)
        return;

    uint8_t txbuf[UART_BUF_SIZE];

    /* 帧格式: HEAD1 HEAD2 LEN CMD [PAYLOAD] CHK
     * pkt.len = 总帧长（含帧头） = payload_len + 5
     * 线缆 LEN 字段 = payload_len + 3
     * 最大 payload_len = MAX_PACKET_LEN - 5 = 123 → txbuf 最大 128 字节 */
    uint8_t wire_len = (uint8_t)(pkt.len - 2);
    uint8_t pay_len  = (uint8_t)(pkt.len - 5);

    txbuf[0] = PACKET_HEAD1;
    txbuf[1] = PACKET_HEAD2;
    txbuf[2] = wire_len;
    txbuf[3] = pkt.cmd;
    if (pay_len > 0)
        memcpy(&txbuf[4], pkt.payload, pay_len);
    txbuf[4 + pay_len] = pkt.chk;

    BSP_UART_WriteTxBusy(pkt.id);
    BSP_UART_Send(pkt.id, txbuf, pkt.len);   // 总发送字节数 = pkt.len
}


/**
 * @brief 打包并推入发送 FIFO 队列
 * @param id   来源/目标编号
 * @param cmd  命令字
 * @param data 数据内容指针（len == 0 时可为 NULL）
 * @param len  数据长度
 * @retval 1：成功打包并入队
 * @retval 0：失败（参数非法、队列满等）
 */
uint8_t APP_UART_BuildTxPacket(uint8_t id, uint8_t cmd, const uint8_t *data, uint8_t len)
{
    // 1. 调用打包函数构造数据包
    Codec_Packet_t pkt = Codec_BuildTxPacket(id, cmd, data, len+5);
    if (pkt.len == -1)
    {
        return 0;   // 打包失败（参数非法或长度溢出）
    }

    // 2. 压入发送 FIFO
    if (APP_UART_PacketFIFO_Push(&txPacketFIFO, &pkt) != 0) {
        return 0;   // 队列满
    }

    // 3. 启用调度器
    APP_UART_TxScheduler(id);

    return 1;       // 成功
}
