/**
 * @file test_svc_proto.c
 * @brief Svc_Proto 解包器的主机端单元测试
 * @author 李嘉图
 * @date 2026-10-31
 *
 * @note 只测协议本身：打包/解包的对称性、粘包、半包、错校验、噪声重同步。
 *       用主机 gcc 编译，与固件编同一份 svc_proto.c（不复制源码）。
 *
 *       在库根目录（RC_Dog/）执行：
 *         gcc -std=c11 -Wall -Wextra -Werror -I DogCore/User/Service/Svc_Proto \
 *             tests/service/test_svc_proto.c \
 *             DogCore/User/Service/Svc_Proto/svc_proto.c -o build/test_svc_proto
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "svc_proto.h"

/*==============================================================================
 * 内部变量
 *============================================================================*/
static int s_pass;   /**< 通过的断言数 */
static int s_fail;   /**< 失败的断言数 */

/*==============================================================================
 * 断言
 *============================================================================*/
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
 * 用例
 *============================================================================*/
/**
 * @brief 用例 1：打包后再逐字节解包，内容必须一致
 * @retval 无
 */
static void test_roundtrip(void)
{
    uint8_t           frame[SVC_PROTO_MAX_FRAME];
    svc_proto_parser_t parser;
    svc_proto_frame_t  out;
    const uint8_t      payload[4] = { 0x11, 0x22, 0x33, 0x44 };
    uint16_t           full;

    printf("用例 1 打包/解包对称\n");

    full = Svc_Proto_Build(0x41, payload, 4, frame, sizeof(frame));
    CHECK(full == 9, "帧全长 = 载荷 4 + 5");
    CHECK(frame[0] == 0xFF && frame[1] == 0xAA, "帧头正确");
    CHECK(frame[2] == 9, "长度字节 = 全长");

    Svc_Proto_Init(&parser);
    for (uint16_t i = 0; i < full; i++)
    {
        int ret = Svc_Proto_Feed(&parser, frame[i], &out);
        if (i + 1 < full)
            CHECK(ret == 0, "半包期间返回 0");
        else
        {
            CHECK(ret == 1, "最后一字节返回 1");
            CHECK(out.cmd == 0x41, "命令字还原");
            CHECK(out.len == 4, "载荷长度还原");
            CHECK(memcmp(out.payload, payload, 4) == 0, "载荷内容还原");
        }
    }
}

/**
 * @brief 用例 2：两帧粘在一起，必须连续解出两帧
 * @retval 无
 */
static void test_sticky(void)
{
    uint8_t            buf[2 * SVC_PROTO_MAX_FRAME];
    svc_proto_parser_t parser;
    svc_proto_frame_t  out;
    uint16_t           n1;
    uint16_t           n2;
    int                hits = 0;

    printf("用例 2 粘包\n");

    n1 = Svc_Proto_Build(0x01, NULL, 0, buf, sizeof(buf));
    n2 = Svc_Proto_Build(0x02, (const uint8_t *)"AB", 2, buf + n1, sizeof(buf) - n1);

    Svc_Proto_Init(&parser);
    for (uint16_t i = 0; i < (uint16_t)(n1 + n2); i++)
    {
        if (Svc_Proto_Feed(&parser, buf[i], &out) == 1)
        {
            hits++;
            if (hits == 1)
                CHECK(out.cmd == 0x01 && out.len == 0, "第一帧正确");
            else
                CHECK(out.cmd == 0x02 && out.len == 2 && out.payload[1] == 'B',
                      "第二帧正确");
        }
    }
    CHECK(hits == 2, "共解出 2 帧");
}

/**
 * @brief 用例 3：校验和错误必须丢弃，且之后能重新同步
 * @retval 无
 */
static void test_bad_checksum(void)
{
    uint8_t            frame[SVC_PROTO_MAX_FRAME];
    svc_proto_parser_t parser;
    svc_proto_frame_t  out;
    uint16_t           full;
    int                ret = 0;

    printf("用例 3 校验错误\n");

    full = Svc_Proto_Build(0x21, (const uint8_t *)"xy", 2, frame, sizeof(frame));
    frame[full - 1] ^= 0x5A;             /* 故意改坏校验和 */

    Svc_Proto_Init(&parser);
    for (uint16_t i = 0; i < full; i++)
        ret = Svc_Proto_Feed(&parser, frame[i], &out);
    CHECK(ret == -1, "错校验返回 -1");

    /* 紧接着再发一帧好数据，必须还能解出来 */
    full = Svc_Proto_Build(0x22, (const uint8_t *)"z", 1, frame, sizeof(frame));
    ret  = 0;
    for (uint16_t i = 0; i < full; i++)
        ret = Svc_Proto_Feed(&parser, frame[i], &out);
    CHECK(ret == 1 && out.cmd == 0x22, "错帧之后仍能重新同步");
}

/**
 * @brief 用例 4a：前导噪声里的长度非法，必须立刻对齐到真帧
 * @retval 无
 */
static void test_resync(void)
{
    const uint8_t noise[] = { 0x00, 0xFF, 0xFF, 0xAA, 0x01, 0xFF, 0xAA };
    uint8_t            frame[SVC_PROTO_MAX_FRAME];
    svc_proto_parser_t parser;
    svc_proto_frame_t  out;
    uint16_t           full;
    int                ret = 0;
    uint16_t           i;

    printf("用例 4a 前导噪声重同步\n");

    full = Svc_Proto_Build(0x31, (const uint8_t *)"ok", 2, frame, sizeof(frame));

    Svc_Proto_Init(&parser);
    for (i = 0; i < sizeof(noise); i++)
        ret = Svc_Proto_Feed(&parser, noise[i], &out);
    CHECK(ret == 0, "噪声期间不产生帧");

    for (i = 0; i < full; i++)
        ret = Svc_Proto_Feed(&parser, frame[i], &out);
    CHECK(ret == 1 && out.cmd == 0x31, "噪声之后解出真帧");
}

/**
 * @brief 用例 4b：假帧头带合法长度、校验失败，真帧在后面仍要解出
 * @retval 无
 */
static void test_false_header(void)
{
    uint8_t            bad[SVC_PROTO_MAX_FRAME];
    uint8_t            good[SVC_PROTO_MAX_FRAME];
    svc_proto_parser_t parser;
    svc_proto_frame_t  out;
    uint16_t           n_bad;
    uint16_t           n_good;
    int                ret  = 0;
    int                hits = 0;

    printf("用例 4b 假帧头（长度合法、校验错）\n");

    /* 造一帧 64 字节、校验错的数据：帧头合法但内容不是真帧 */
    n_bad = 64;
    bad[0] = 0xFF;
    bad[1] = 0xAA;
    bad[2] = (uint8_t)n_bad;
    for (uint16_t i = 3; i < n_bad; i++)
        bad[i] = (uint8_t)(0x30 + i);
    bad[n_bad - 1] = 0x00;              /* 校验和一定不对 */

    n_good = Svc_Proto_Build(0x77, (const uint8_t *)"tail", 4, good, sizeof(good));

    Svc_Proto_Init(&parser);
    for (uint16_t i = 0; i < n_bad; i++)
        ret = Svc_Proto_Feed(&parser, bad[i], &out);
    CHECK(ret == -1, "假帧满 64 字节后报校验错");

    for (uint16_t i = 0; i < n_good; i++)
        if (Svc_Proto_Feed(&parser, good[i], &out) == 1)
            hits++;
    CHECK(hits == 1 && out.cmd == 0x77, "假帧之后的真帧不受影响");
}

/**
 * @brief 用例 4c：真帧落在假帧跨度内，靠重放捞回来
 * @retval 无
 */
static void test_replay_inside(void)
{
    uint8_t            outer[SVC_PROTO_MAX_FRAME];
    uint8_t            inner[16];
    svc_proto_parser_t parser;
    svc_proto_frame_t  out;
    uint16_t           n_inner;
    int                ret  = 0;
    int                hits = 0;

    printf("用例 4c 假帧跨度内藏真帧（重放）\n");

    /* 外层：FF AA 0A + [内层真帧 5 字节] + 2 字节填充 + 错校验 = 10 字节 */
    n_inner = Svc_Proto_Build(0x66, NULL, 0, inner, sizeof(inner));
    CHECK(n_inner == SVC_PROTO_OVERHEAD, "内层帧 5 字节");

    outer[0] = 0xFF;
    outer[1] = 0xAA;
    outer[2] = 10;
    memcpy(&outer[3], inner, n_inner);
    outer[8]  = 0x00;
    outer[9]  = 0x00;                   /* 外层校验一定不对 */

    Svc_Proto_Init(&parser);
    for (uint16_t i = 0; i < 10u; i++)
    {
        ret = Svc_Proto_Feed(&parser, outer[i], &out);
        if (ret == 1)
        {
            hits++;
            CHECK(out.cmd == 0x66, "重放捞出内层真帧");
        }
    }
    CHECK(hits == 1, "恰好捞出 1 帧");
}

/**
 * @brief 用例 5：边界长度
 * @retval 无
 */
static void test_bounds(void)
{
    uint8_t            frame[SVC_PROTO_MAX_FRAME];
    uint8_t            big[SVC_PROTO_MAX_PAYLOAD];
    svc_proto_parser_t parser;
    svc_proto_frame_t  out;
    uint16_t           full;

    printf("用例 5 边界\n");

    /* 空载荷 */
    full = Svc_Proto_Build(0x00, NULL, 0, frame, sizeof(frame));
    CHECK(full == SVC_PROTO_OVERHEAD, "空载荷帧长 = 5");

    /* 最大载荷 */
    for (uint8_t i = 0; i < SVC_PROTO_MAX_PAYLOAD; i++)
        big[i] = i;
    full = Svc_Proto_Build(0x55, big, SVC_PROTO_MAX_PAYLOAD, frame, sizeof(frame));
    CHECK(full == SVC_PROTO_MAX_FRAME, "最大载荷帧长 = 128");

    Svc_Proto_Init(&parser);
    for (uint16_t i = 0; i < full; i++)
    {
        int ret = Svc_Proto_Feed(&parser, frame[i], &out);
        if (i + 1 == full)
            CHECK(ret == 1 && out.len == SVC_PROTO_MAX_PAYLOAD &&
                  memcmp(out.payload, big, SVC_PROTO_MAX_PAYLOAD) == 0,
                  "最大帧内容完整");
    }

    /* 缓冲区不足 */
    CHECK(Svc_Proto_Build(0x01, big, 4, frame, 8) == 0, "缓冲区不足返回 0");
}

/**
 * @brief 用例 6：任意分片（含逐字节）不影响结果
 * @retval 无
 */
static void test_split(void)
{
    uint8_t            stream[3 * SVC_PROTO_MAX_FRAME];
    svc_proto_parser_t parser;
    svc_proto_frame_t  out;
    uint16_t           total = 0;
    int                hits  = 0;

    printf("用例 6 任意分片\n");

    total += Svc_Proto_Build(0x10, (const uint8_t *)"alpha", 5,
                             stream + total, sizeof(stream) - total);
    total += Svc_Proto_Build(0x20, NULL, 0, stream + total,
                             sizeof(stream) - total);
    total += Svc_Proto_Build(0x30, (const uint8_t *)"omega", 5,
                             stream + total, sizeof(stream) - total);

    Svc_Proto_Init(&parser);
    for (uint16_t i = 0; i < total; i++)
        if (Svc_Proto_Feed(&parser, stream[i], &out) == 1)
            hits++;

    CHECK(hits == 3, "逐字节喂入仍解出 3 帧");
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
    printf("=== Svc_Proto 解包器测试 ===\n");

    test_roundtrip();
    test_sticky();
    test_bad_checksum();
    test_resync();
    test_false_header();
    test_replay_inside();
    test_bounds();
    test_split();

    printf("=== 断言 %d 条，失败 %d 条 ===\n", s_pass + s_fail, s_fail);
    return (s_fail == 0) ? 0 : 1;
}
