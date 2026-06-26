/**
 * @file codec.c
 * @brief 数据包解包打包
 *        负责包头检测、长度检查、校验和校验、半包粘包处理
 * @author 李嘉图
 * @date 2026-6-26
 *
 * @note 协议格式 v3（byte[2] 存包全长）：
 *       [0] 0xFF         — 包头1
 *       [1] 0xAA         — 包头2
 *       [2] LEN          — 包全长（含帧头帧尾，最小 5）
 *       [3] CMD          — 命令字
 *       [4..LEN-2]       — PAYLOAD（LEN - 5 字节）
 *       [LEN-1]          — CHK   （校验和 = HEAD1 + HEAD2 + LEN + CMD + PAYLOAD）
 *
 *       校验和覆盖 byte[0] 到 byte[LEN-2]（含帧头）。
 *       各 MCU 统一移植此文件，byte[2] 始终为包全长。
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
#include <stdint.h>
#include <string.h>
#include "main.h"
#include "codec.h"

/*==============================================================================
 * 解包函数
 *============================================================================*/
int Codec_ParseRxPacket(const uint8_t *buf, uint16_t buf_len,
                         uint16_t *consumed, Codec_Packet_t *pkt)
{
    if (!buf || buf_len == 0 || !consumed || !pkt)
        return -1;

    uint16_t processed = 0;

    while (buf_len - processed >= MIN_PACKET_LEN)
    {
        /* 1. 查找包头 */
        if (buf[processed] != PACKET_HEAD1 ||
            buf[processed + 1] != PACKET_HEAD2)
        {
            processed++;
            continue;
        }

        /* 2. 读取包全长（byte[2]），数据长度 = 全长 - 5 */
        uint8_t full_len = buf[processed + 2];
        uint8_t data_len = full_len - 5;        /* 数据负载长度 */

        if (full_len < MIN_PACKET_LEN || full_len > MAX_PACKET_LEN)
        {
            processed += 2;
            continue;
        }

        if (buf_len - processed < full_len)
        {
            *consumed = processed;
            return 0;
        }

        /* 3. 校验和（从 byte[0] 累加到 byte[LEN-2]，含帧头 FF AA） */
        uint8_t sum = 0;
        for (uint8_t i = 0; i < full_len - 1; i++)
        {
            sum += buf[processed + i];
        }

        if (sum != buf[processed + full_len - 1])
        {
            processed += full_len;
            continue;
        }

        /* 4. 填充 Codec_Packet_t */
        pkt->len = full_len;                    /* 包全长 */
        pkt->cmd = buf[processed + 3];

        if (data_len > 0)
        {
            memcpy(pkt->payload, &buf[processed + 4], data_len);
        }

        /* 5. 消耗 */
        processed += full_len;
        *consumed = processed;

        if (buf_len - processed >= MIN_PACKET_LEN)
            return 2;
        else
            return 1;
    }

    *consumed = processed;
    return 0;
}

/*==============================================================================
 * 打包函数
 *============================================================================*/
Codec_Packet_t Codec_BuildTxPacket(uint8_t id, uint8_t cmd,
                                    const uint8_t *data, uint8_t data_len)
{
    Codec_Packet_t pkt = {0};

    /* 全长 = 数据长度 + 帧头帧尾（5 字节） */
    uint8_t full_len = data_len + 5;

    if (full_len < MIN_PACKET_LEN || full_len > MAX_PACKET_LEN)
    {
        pkt.len = (uint8_t)-1;
        return pkt;
    }

    if (data_len > 0 && data == NULL)
    {
        pkt.len = (uint8_t)-1;
        return pkt;
    }

    /* 校验和 = HEAD1 + HEAD2 + LEN + CMD + PAYLOAD */
    uint8_t chk = PACKET_HEAD1 + PACKET_HEAD2 + full_len + cmd;
    for (uint8_t i = 0; i < data_len; i++)
    {
        chk += data[i];
    }

    pkt.id    = id;
    pkt.head1 = PACKET_HEAD1;
    pkt.head2 = PACKET_HEAD2;
    pkt.cmd   = cmd;
    pkt.len   = full_len;       /* 包全长 */
    if (data_len > 0)
        memcpy(pkt.payload, data, data_len);
    pkt.chk   = chk;

    return pkt;
}
