/**
 * @file codec.h
 * @brief 串口数据包编解码
 *        协议：FF AA LEN CMD [DATA] CHK
 * @author 李嘉图
 * @date 2026-6-26
 *
 * @note byte[2] 直接存储包全长（LEN）。
 *       data_len = LEN - 5，范围 0~123（无负载时 data_len=0, LEN=5）
 *       LEN 范围 5~128
 *       校验和 = HEAD1 + HEAD2 + LEN + CMD + PAYLOAD（含帧头）
 */

#ifndef __CODEC_H__
#define __CODEC_H__
/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include <stdint.h>

/*==============================================================================
 * 宏定义与常量
 *============================================================================*/
#define PACKET_HEAD1   0xFF         /**< 包头1 */
#define PACKET_HEAD2   0xAA         /**< 包头2 */
#define MIN_PACKET_LEN 5            /**< 最小包全长（无负载时 = 5） */
#define MAX_PACKET_LEN 128          /**< 最大包全长 */

/** @brief 最大数据负载长度 */
#define MAX_DATA_LEN   (MAX_PACKET_LEN - 5)  /* 123 */

/*==============================================================================
 * 数据包结构体
 *============================================================================*/
typedef struct
{
    uint8_t id;                             /**< 串口 ID（上层标识，帧内不含） */
    uint8_t head1;                          /**< 包头1 (0xFF) */
    uint8_t head2;                          /**< 包头2 (0xAA) */
    uint8_t cmd;                            /**< 命令字 */
    uint8_t len;                            /**< 包全长 = data_len + 5 */
    uint8_t payload[MAX_DATA_LEN];          /**< 数据负载 */
    uint8_t chk;                            /**< 校验和 = HEAD1 + HEAD2 + LEN + cmd + payload */
} Codec_Packet_t;

/*==============================================================================
 * 解包函数
 *============================================================================*/
/**
 * @brief 解包器：从缓冲区中解析一个完整数据包（支持粘包/半包）
 * @param buf       输入缓冲区
 * @param buf_len   当前缓冲区有效长度
 * @param consumed  输出：本次消耗的字节数
 * @param pkt       输出：解析出的数据包（pkt.len = 全长）
 * @retval  2：成功解析一个包，且后续仍有完整包
 * @retval  1：成功解析一个包，后续无包
 * @retval  0：半包，需等更多数据
 * @retval -1：参数非法
 */
int Codec_ParseRxPacket(const uint8_t *buf, uint16_t buf_len,
                         uint16_t *consumed, Codec_Packet_t *pkt);

/*==============================================================================
 * 打包函数
 *============================================================================*/
/**
 * @brief 打包器：构造一个待发送的数据包结构体
 * @param id       来源/目标编号（上层标识）
 * @param cmd      命令字
 * @param data     payload 指针（data_len==0 时可为 NULL）
 * @param data_len payload 长度（0~123，0=无负载）
 * @return 成功：pkt.len = data_len + 5
 * @return 失败：pkt.len == (uint8_t)-1
 */
Codec_Packet_t Codec_BuildTxPacket(uint8_t id, uint8_t cmd,
                                    const uint8_t *data, uint8_t data_len);

#endif
