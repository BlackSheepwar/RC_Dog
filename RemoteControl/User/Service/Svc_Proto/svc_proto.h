/**
 * @file svc_proto.h
 * @brief 串口帧协议：FF AA LEN CMD [PAYLOAD] CHK 的打包与流式解包
 * @author 李嘉图
 * @date 2026-10-31
 *
 * @note 帧格式（byte[2] 存帧全长，含帧头帧尾）：
 *         [0]      0xFF      帧头 1
 *         [1]      0xAA      帧头 2
 *         [2]      LEN       帧全长 = 载荷长度 + 5，范围 5~128
 *         [3]      CMD       命令字
 *         [4..LEN-2]        PAYLOAD（LEN - 5 字节）
 *         [LEN-1]  CHK       校验和 = byte[0] + ... + byte[LEN-2]
 *
 *       解包用流式状态机而不是"整块缓冲 + 反复扫描"：
 *         - 收到的字节逐个喂进去，不需要等一整帧到齐；
 *         - 不需要 memmove 搬运残留数据，粘包/半包天然成立；
 *         - 队列回绕、分包到达都不影响结果。
 *
 *       本模块是纯函数，不碰硬件、不看时间，可在 PC 上单独编译测试。
 */

#ifndef __Svc_Proto_H__
#define __Svc_Proto_H__

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include <stdint.h>

/*==============================================================================
 * 宏定义与常量
 *============================================================================*/
#define SVC_PROTO_HEAD1         0xFF   /* 帧头 1 */
#define SVC_PROTO_HEAD2         0xAA   /* 帧头 2 */
#define SVC_PROTO_OVERHEAD      5      /* 帧头2 + 长度 + 命令 + 校验 = 5 字节 */
#define SVC_PROTO_MAX_FRAME     128    /* 帧全长上限 */
#define SVC_PROTO_MAX_PAYLOAD   (SVC_PROTO_MAX_FRAME - SVC_PROTO_OVERHEAD) /* 123 */

/*==============================================================================
 * 类型定义
 *============================================================================*/
/**
 * @brief 一帧解析结果
 * @note 值类型：调用方按值持有或放进队列，布局稳定。
 *       id 由链路层填，帧里没有这个字段。
 */
typedef struct {
    uint8_t id;                              /* 来源端口编号，1 起 */
    uint8_t cmd;                             /* 命令字 */
    uint8_t len;                             /* 载荷长度，0 ~ 123 */
    uint8_t payload[SVC_PROTO_MAX_PAYLOAD];  /* 载荷 */
} svc_proto_frame_t;

/**
 * @brief 解包状态机
 * @note 调用方按值持有（通常放在链路层的端口上下文里）。
 *       字段是内部实现，外部只负责初始化和喂字节。
 *       hist 保存"当前候选帧"的原始字节：校验失败时靠它把后面的字节
 *       重新当作可能的帧头扫一遍，避免一个假帧头把紧随其后的真帧吃掉。
 */
typedef struct {
    uint8_t state;     /* 当前状态（SVC_PROTO_ST_xxx） */
    uint8_t full_len;  /* 本帧全长 */
    uint8_t hist_len;  /* 已收字节数 */
    uint8_t hist[SVC_PROTO_MAX_FRAME];  /* 候选帧原始字节 */
} svc_proto_parser_t;

/*==============================================================================
 * 解包
 *============================================================================*/
/**
 * @brief 复位解包状态机
 * @param parser 解包器，可为 NULL
 * @retval 无
 * @note 上电、链路重连、长时间无数据后都应调用一次。
 */
void Svc_Proto_Init(svc_proto_parser_t *parser);

/**
 * @brief 喂入一个字节
 * @param parser 解包器，不可为 NULL
 * @param byte   收到的字节
 * @param out    解出一帧时的输出缓冲，不可为 NULL
 * @retval 1: 解出一帧，out 有效（id 未填，由链路层补）
 * @retval 0: 已消费，尚未成帧
 * @retval -1: 校验和错误，本帧丢弃，状态机已复位
 * @note 逐字节调用；粘包会连续返回多次 1，半包返回 0。
 *       校验失败时内部会把本帧从第 2 个字节起重放一遍（防止假帧头
 *       吞掉后面的真帧），因此返回 -1 表示"这一帧坏了"，之后
 *       紧接着的字节仍会被正常解析。
 */
int Svc_Proto_Feed(svc_proto_parser_t *parser, uint8_t byte,
                   svc_proto_frame_t *out);

/*==============================================================================
 * 打包
 *============================================================================*/
/**
 * @brief 组装一帧待发送数据
 * @param cmd     命令字
 * @param payload 载荷，len 为 0 时可为 NULL
 * @param len     载荷长度，0 ~ SVC_PROTO_MAX_PAYLOAD
 * @param out     输出缓冲区，不可为 NULL
 * @param cap     输出缓冲区容量
 * @retval >0: 写入的帧总长（含帧头帧尾）
 * @retval 0: 参数非法或缓冲区不够
 * @note 帧格式与 Svc_Proto_Feed 严格对称，校验和覆盖 byte[0..len-2]。
 */
uint16_t Svc_Proto_Build(uint8_t cmd, const uint8_t *payload, uint8_t len,
                         uint8_t *out, uint16_t cap);

/**
 * @brief 计算一帧的校验和
 * @param frame    帧首地址，不可为 NULL
 * @param full_len 帧全长（含帧头帧尾）
 * @retval 校验和：byte[0] ~ byte[full_len-2] 的累加和低 8 位
 * @retval 0: 参数非法（full_len 小于最小帧长）
 */
uint8_t Svc_Proto_Checksum(const uint8_t *frame, uint8_t full_len);

#endif /* __Svc_Proto_H__ */
