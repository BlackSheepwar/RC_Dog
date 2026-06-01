/**
 * @file codec.h
 * @brief 串口数据包解析处理任务
 *        负责包头检测、长度检查、校验和校验、半包粘包处理
 *        并将完整数据包上报给APP层处理
 * @author 李嘉图
 * @date 2026-5-4
 */

#ifndef __CODEC_H__
#define __CODEC_H__

/*==============================================================================
 * 宏定义与常量
 *============================================================================*/
#define PACKET_HEAD1   0xFF         // 包头1
#define PACKET_HEAD2   0xAA         // 包头2
#define MIN_PACKET_LEN 5            // 最小包长
#define MAX_PACKET_LEN 128          // 最大包长

/*==============================================================================
 * 数据包结构体
 *============================================================================*/
// 数据包结构体
typedef struct
{
    uint8_t id;                             // 串口id
    uint8_t head1;                          // 包头1
    uint8_t head2;                          // 包头2
    uint8_t cmd;                            // 命令字
    uint8_t len;                            // 数据长度
    uint8_t payload[MAX_PACKET_LEN - 5];    // 数据内容
    uint8_t chk;                            // 校验和
} Codec_Packet_t;

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
                         uint16_t *consumed, Codec_Packet_t *pkt);

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
Codec_Packet_t Codec_BuildTxPacket(uint8_t id, uint8_t cmd, const uint8_t *data, uint8_t len);

#endif
