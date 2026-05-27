/**
 * @file codec.c
 * @brief 数据包解包打包
 *        负责包头检测、长度检查、校验和校验、半包粘包处理
 *        数据包结构：0xFF 0xAA 数据长度 数据 校验和
 * @author 李嘉图
 * @date 2026-5-4
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
#include "main.h"
#include "string.h"
#include "codec.h"

/*==============================================================================
 * 解包函数
 *============================================================================*/
/**
 * @brief 解包器：从缓冲区中解析一个完整数据包（支持粘包/半包）
 * @param buf       输入缓冲区
 * @param buf_len   当前缓冲区有效数据长度
 * @param consumed  输出：本次消耗的字节数
 * @param pkt       输出：解析出的数据包
 * @retval 2：成功解析一个包，且后续仍可能存在完整包
 * @retval 1：成功解析一个包，且后续没有完整包
 * @retval 0：未解析出完整包（可能是半包）
 * @retval -1：参数非法
 */
int Codec_ParseRxPacket(const uint8_t *buf, uint16_t buf_len,
                         uint16_t *consumed, Codec_Packet_t *pkt)
{
    if (!buf || buf_len == 0 || !consumed || !pkt)
        return -1;

    uint16_t processed = 0;

    while (buf_len - processed >= MIN_PACKET_LEN)
    {
        // 1. 查找包头
        if (buf[processed] != PACKET_HEAD1 ||
            buf[processed + 1] != PACKET_HEAD2)
        {
            processed++;
            continue;
        }

        // 2. 长度字段
        uint8_t packetLen = buf[processed + 2];

        // 长度非法（防止死循环 + 防攻击）
        if (packetLen < MIN_PACKET_LEN || packetLen > MAX_PACKET_LEN)
        {
            processed += 2;
            continue;
        }

        // 3. 半包判断
        if (buf_len - processed < packetLen)
        {
            *consumed = processed;
            return 0;
        }

        // 4. 校验
        uint8_t sum = 0;
        for (uint8_t i = 0; i < packetLen - 1; i++)
        {
            sum += buf[processed + i];
        }

        if (sum != buf[processed + packetLen - 1])
        {
            // 校验失败：直接跳过整个包长度（比+1更快恢复同步）
            processed += packetLen;
            continue;
        }

        // 5. 填充 Codec_Packet_t（零拷贝思路：这里只复制payload）
        pkt->len = packetLen;
        pkt->cmd = buf[processed + 3];
        pkt->id  = 0; // 如果协议里没有id，这里保留

        if (packetLen-5 > 0)
        {
            memcpy(pkt->payload, &buf[processed + 4], packetLen - 5);
        }

        // 6. 消耗
        processed += packetLen;
        *consumed = processed;

        // 7. 判断是否还有完整包
        if (buf_len - processed >= MIN_PACKET_LEN)
            return 2;  // 还有包
        else
            return 1;  // 最后一个包
    }

    *consumed = processed;
    return 0;
}

/*==============================================================================
 * 打包函数
 *============================================================================*/
/**
 * @brief 打包器：构造一个待发送的数据包结构体
 * @param id   来源/目标编号（上层标识，帧内不含）
 * @param cmd  命令字
 * @param data 数据内容指针（len == 0 时可为 NULL）
 * @param len  数据长度
 * @return 成功：pkt.len == len
 * @return 失败：pkt.len == -1
 */
Codec_Packet_t Codec_BuildTxPacket(uint8_t id, uint8_t cmd, const uint8_t *data, uint8_t len)
{
    Codec_Packet_t pkt = {0};

    // 参数合法性检查
    if ((len > 0 && data == NULL) || len > MAX_PACKET_LEN || len < MIN_PACKET_LEN)
    {
        pkt.len = -1;
        return pkt;   // 失败返回零值结构体，len = -1
    }

    uint8_t chk = 0;
    chk += id;
    chk += PACKET_HEAD1;
    chk += PACKET_HEAD2;
    chk += cmd;
    chk += len;
    for (uint8_t i = 0; i < len-5; i++)
    {
        chk += data[i];
    }

    pkt.id  = id;
    pkt.head1 = PACKET_HEAD1;
    pkt.head2 = PACKET_HEAD2;
    pkt.cmd = cmd;
    pkt.len = len;
    if (len > 0) memcpy(pkt.payload, data, len);
    pkt.chk = chk;

    return pkt;
}