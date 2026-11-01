/**
 * @file svc_proto.c
 * @brief 串口帧协议实现：带重放的流式解包 + 打包
 * @author 李嘉图
 * @date 2026-10-31
 *
 * @note 解包分三段：找帧头 1 → 找帧头 2 → 收满一帧再验校验。
 *       之所以"收满再验"而不是边收边算：校验失败时要把这一帧的原始
 *       字节重放一遍，才能发现藏在里面的真帧。代价是每端口多 128 字节，
 *       换来的是"一个假帧头不会吞掉后面 128 字节"。
 *
 *       重放只做一层：重放中再失败就复位收工。协议帧最长 128 字节，
 *       一层重放已经能覆盖"噪声里夹着真帧"这个实际场景，
 *       再深的递归只会让错误路径更慢、更难解释。
 *
 *       校验和覆盖帧头，与旧版 codec 的线缆格式完全一致，
 *       两端固件可以分批升级，不会出现"协议不兼容"。
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include <stdint.h>
#include <string.h>
#include "svc_proto.h"

/*==============================================================================
 * 宏定义与常量
 *============================================================================*/
/** @brief 解包状态 */
enum {
    SVC_PROTO_ST_HEAD1 = 0,  /* 等帧头 1 */
    SVC_PROTO_ST_HEAD2,      /* 等帧头 2 */
    SVC_PROTO_ST_COLLECT     /* 收长度、收满一帧 */
};

/*==============================================================================
 * 内部函数
 *============================================================================*/
/**
 * @brief 单步推进状态机（不含重放）
 * @param parser 解包器，不可为 NULL
 * @param byte   本次字节
 * @param out    成帧时的输出缓冲，不可为 NULL
 * @retval 1: 解出一帧
 * @retval 0: 继续
 * @retval -1: 校验失败；hist 中保留整帧原始字节，供调用方重放
 */
static int Svc_Proto_Step(svc_proto_parser_t *parser, uint8_t byte,
                          svc_proto_frame_t *out)
{
    switch (parser->state)
    {
    /* ---------- 1. 找帧头 1 ---------- */
    case SVC_PROTO_ST_HEAD1:
        if (byte == SVC_PROTO_HEAD1)
        {
            parser->hist[0]  = byte;
            parser->hist_len = 1u;
            parser->state    = (uint8_t)SVC_PROTO_ST_HEAD2;
        }
        break;

    /* ---------- 2. 找帧头 2 ---------- */
    case SVC_PROTO_ST_HEAD2:
        if (byte == SVC_PROTO_HEAD2)
        {
            parser->hist[1]  = byte;
            parser->hist_len = 2u;
            parser->state    = (uint8_t)SVC_PROTO_ST_COLLECT;
        }
        else if (byte == SVC_PROTO_HEAD1)
        {
            parser->hist[0]  = byte;      /* 还可能是帧头，别丢 */
            parser->hist_len = 1u;
        }
        else
        {
            parser->state = (uint8_t)SVC_PROTO_ST_HEAD1;
        }
        break;

    /* ---------- 3. 收长度、收满一帧 ---------- */
    case SVC_PROTO_ST_COLLECT:
        if (parser->hist_len == 2u)
        {
            /* 长度字节 */
            if ((byte < SVC_PROTO_OVERHEAD) || (byte > SVC_PROTO_MAX_FRAME))
            {
                /* 长度非法：本字节可能就是新帧头，按帧头再判一次 */
                parser->state    = (uint8_t)SVC_PROTO_ST_HEAD1;
                parser->hist_len = 0u;
                if (byte == SVC_PROTO_HEAD1)
                {
                    parser->hist[0]  = byte;
                    parser->hist_len = 1u;
                    parser->state    = (uint8_t)SVC_PROTO_ST_HEAD2;
                }
                break;
            }
            parser->full_len = byte;
            parser->hist[parser->hist_len++] = byte;
            break;
        }

        parser->hist[parser->hist_len++] = byte;
        if (parser->hist_len >= parser->full_len)
        {
            /* ---------- 帧齐了：验校验 ---------- */
            uint8_t sum = 0u;

            for (uint8_t i = 0u; i < (uint8_t)(parser->full_len - 1u); i++)
                sum = (uint8_t)(sum + parser->hist[i]);

            if (sum != parser->hist[parser->full_len - 1u])
            {
                parser->state = (uint8_t)SVC_PROTO_ST_HEAD1;
                return -1;      /* hist 保留整帧，交给 Feed 重放 */
            }

            out->cmd = parser->hist[3];
            out->len = (uint8_t)(parser->full_len - SVC_PROTO_OVERHEAD);
            if (out->len > 0u)
                memcpy(out->payload, &parser->hist[4], out->len);

            parser->state    = (uint8_t)SVC_PROTO_ST_HEAD1;
            parser->hist_len = 0u;
            return 1;
        }
        break;

    default:
        parser->state    = (uint8_t)SVC_PROTO_ST_HEAD1;
        parser->hist_len = 0u;
        break;
    }

    return 0;
}

/*==============================================================================
 * 解包
 *============================================================================*/
/**
 * @brief 复位解包状态机
 * @param parser 解包器，可为 NULL
 * @retval 无
 */
void Svc_Proto_Init(svc_proto_parser_t *parser)
{
    if (!parser)
        return;

    memset(parser, 0, sizeof(*parser));
    parser->state = (uint8_t)SVC_PROTO_ST_HEAD1;
}

/**
 * @brief 喂入一个字节
 * @param parser 解包器，不可为 NULL
 * @param byte   收到的字节
 * @param out    解出一帧时的输出缓冲，不可为 NULL
 * @retval 1: 解出一帧
 * @retval 0: 已消费，尚未成帧
 * @retval -1: 校验和错误
 */
int Svc_Proto_Feed(svc_proto_parser_t *parser, uint8_t byte,
                   svc_proto_frame_t *out)
{
    uint8_t candidate[SVC_PROTO_MAX_FRAME];
    uint8_t len;
    int     ret;

    if (!parser || !out)
        return 0;

    ret = Svc_Proto_Step(parser, byte, out);
    if (ret >= 0)
        return ret;

    /* ---------- 校验失败：把这一帧从第 2 个字节起重放 ---------- */
    len = parser->hist_len;
    if (len > SVC_PROTO_MAX_FRAME)
        len = SVC_PROTO_MAX_FRAME;
    memcpy(candidate, parser->hist, len);

    Svc_Proto_Init(parser);

    for (uint8_t i = 1u; i < len; i++)
    {
        int replay = Svc_Proto_Step(parser, candidate[i], out);

        if (replay > 0)
            return 1;               /* 假帧头后面藏着真帧，交出去 */
        if (replay < 0)
        {
            Svc_Proto_Init(parser); /* 重放也坏：不再递归，复位收工 */
            return 0;
        }
    }

    return -1;
}

/*==============================================================================
 * 打包
 *============================================================================*/
/**
 * @brief 计算一帧的校验和
 * @param frame    帧首地址，不可为 NULL
 * @param full_len 帧全长
 * @retval 校验和
 * @retval 0: 参数非法
 */
uint8_t Svc_Proto_Checksum(const uint8_t *frame, uint8_t full_len)
{
    uint8_t sum = 0u;

    if (!frame || (full_len < SVC_PROTO_OVERHEAD))
        return 0u;

    for (uint8_t i = 0u; i < (uint8_t)(full_len - 1u); i++)
        sum = (uint8_t)(sum + frame[i]);

    return sum;
}

/**
 * @brief 组装一帧待发送数据
 * @param cmd     命令字
 * @param payload 载荷，len 为 0 时可为 NULL
 * @param len     载荷长度
 * @param out     输出缓冲区，不可为 NULL
 * @param cap     输出缓冲区容量
 * @retval >0: 帧总长
 * @retval 0: 参数非法或缓冲区不够
 */
uint16_t Svc_Proto_Build(uint8_t cmd, const uint8_t *payload, uint8_t len,
                         uint8_t *out, uint16_t cap)
{
    uint16_t full_len;

    if (!out || (len > SVC_PROTO_MAX_PAYLOAD))
        return 0u;
    if ((len > 0u) && !payload)
        return 0u;

    full_len = (uint16_t)len + SVC_PROTO_OVERHEAD;
    if (cap < full_len)
        return 0u;

    out[0] = SVC_PROTO_HEAD1;
    out[1] = SVC_PROTO_HEAD2;
    out[2] = (uint8_t)full_len;
    out[3] = cmd;

    if (len > 0u)
        memcpy(&out[4], payload, len);

    out[full_len - 1u] = Svc_Proto_Checksum(out, (uint8_t)full_len);
    return full_len;
}
