/**
 * @file gfifo.c
 * @brief GFIFO 通用环形缓冲库实现：字节流 / 定长结构体 / 变长消息
 * @author Ricardo
 * @date 2026-10-31
 *
 * @note 本文件不含任何平台相关代码：不 include HAL、不 include CMSIS、
 *       不 include 任何 RTOS 头文件。所有平台差异都经 gfifo_port.h 的宏表达，
 *       所以同一份源码可以在 Cortex-M、RISC-V、x86、MSP430 上原样编译。
 *
 * @note 核心不变式（读代码前必须先接受这三条，否则会觉得很多地方"没必要"）：
 *       1) head >= tail 永真（按无符号减法理解，回绕自动正确）；
 *       2) used = (gfifo_idx_t)(head - tail)，且 used <= size 恒成立；
 *       3) head 只由生产者写，tail 只由消费者写，rsv 只由生产者写。
 *       只要这三条成立，SPSC 场景下不加锁也不会读到半条数据——因为
 *       "数据先写、head 后更新"的顺序由 GFIFO_BARRIER() 保证。
 *
 * @note 关于 rsv（预留计数）：它让"预约了但还没写完"的空间对外不可见。
 *       计算空闲量时用 size - used - rsv，读者看到的 used 只包含已提交数据，
 *       所以 DMA 正在灌数据的那段时间里，消费者完全不会碰到那块内存。
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
/* 固定包含 */
#include <stdint.h>
#include <stddef.h>
#include "gfifo.h"
/* 功能包含 */
#if GFIFO_CFG_USE_LIBC
#include <string.h>
#endif

/*==============================================================================
 * 宏定义与常量
 *============================================================================*/

/**
 * @brief 变长消息长度头的字节数，从配置项转成本地短名
 */
#define GFIFO_MSG_HDR ((uint32_t)GFIFO_CFG_MSG_HDR_BYTES)

/**
 * @brief 单条消息的最大长度，由长度头宽度决定
 *
 * @note 1 字节头最大 255，2 字节头最大 65535，4 字节头理论上是 4 GiB - 1，
 *       但实际还会被 FIFO 容量限制，所以这里给一个不会溢出的保守上限。
 */
#if (GFIFO_CFG_MSG_HDR_BYTES == 1)
#define GFIFO_MSG_MAX_LEN 0xFFu
#elif (GFIFO_CFG_MSG_HDR_BYTES == 2)
#define GFIFO_MSG_MAX_LEN 0xFFFFu
#else
#define GFIFO_MSG_MAX_LEN 0xFFFFFFFFu
#endif

/*==============================================================================
 * 内部函数
 *============================================================================*/

/**
 * @brief 内存拷贝，按配置走 libc 或库内字节循环
 * @param dst 目标地址，不可为 NULL，至少可写 len 字节
 * @param src 源地址，不可为 NULL，至少可读 len 字节
 * @param len 字节数，允许为 0
 * @retval 无
 * @note 两段地址不允许重叠：本库的所有拷贝都发生在"缓冲区"与"调用方内存"
 *       之间，或者是缓冲区内部互不重叠的两段，不存在 memmove 的需求。
 */
static void GFIFO_Mem_Copy(void *dst, const void *src, uint32_t len)
{
#if GFIFO_CFG_USE_LIBC
    if (len > 0u) {
        (void)memcpy(dst, src, (size_t)len);
    }
#else
    uint8_t       *d = (uint8_t *)dst;
    const uint8_t *s = (const uint8_t *)src;

    while (len > 0u) {
        *d++ = *s++;
        len--;
    }
#endif
}

#if GFIFO_CFG_CLEAR_ON_INIT
/**
 * @brief 内存填充，按配置走 libc 或库内字节循环
 * @param dst   目标地址，不可为 NULL，至少可写 len 字节
 * @param value 填充值
 * @param len   字节数，允许为 0
 * @retval 无
 * @note 只在 GFIFO_CFG_CLEAR_ON_INIT = 1 时编入：这是它唯一的用途，
 *       关掉清零策略后整个函数都该消失，否则 -Werror 会因为
 *       "定义但未使用" 直接编译失败。
 */
static void GFIFO_Mem_Set(void *dst, uint8_t value, uint32_t len)
{
#if GFIFO_CFG_USE_LIBC
    if (len > 0u) {
        (void)memset(dst, (int)value, (size_t)len);
    }
#else
    uint8_t *d = (uint8_t *)dst;

    while (len > 0u) {
        *d++ = value;
        len--;
    }
#endif
}
#endif /* GFIFO_CFG_CLEAR_ON_INIT */

/**
 * @brief 判断句柄是否可用（非空、魔数正确、缓冲区有效）
 * @param f 待检查的句柄，允许为 NULL
 * @retval 1 可用，后续操作可以安全解引用
 * @retval 0 不可用
 * @note NULL 检查在任何配置下都执行，因为它的成本就是一次比较，而收益是
 *       把"传了空指针"从 HardFault 变成确定的错误码。其余检查受
 *       GFIFO_CFG_ENABLE_PARAM_CHECK 控制。
 */
static uint8_t GFIFO_Is_Valid(const gfifo_t *f)
{
    if (f == NULL) {
        return 0u;
    }

#if GFIFO_CFG_ENABLE_PARAM_CHECK
    if (f->magic != (uint16_t)GFIFO_MAGIC) {
        return 0u;
    }
    if (f->buf == NULL) {
        return 0u;
    }
    if (f->size == 0u) {
        return 0u;
    }
#endif

    /* elem_size 为 0 会让后续的 "len / elem_size" 直接除零，一条比较换一条命，
       所以这条检查不受 GFIFO_CFG_ENABLE_PARAM_CHECK 控制，永远执行 */
    if (f->elem_size == 0u) {
        return 0u;
    }

    return 1u;
}

/**
 * @brief 进入临界区（仅在开启并发支持且该 FIFO 置了 GFIFO_FLAG_CRITICAL 时生效）
 * @param f 句柄指针，不可为 NULL
 * @retval 无
 * @note 与 GFIFO_Unlock 必须成对，且中间不得 return，否则会把中断永久关掉。
 *       本文件里所有加锁函数的写法都是"单一出口 + 先算后赋"，就是为了守住
 *       这条纪律。
 */
static void GFIFO_Lock(gfifo_t *f)
{
#if GFIFO_CFG_ENABLE_CRITICAL
    if ((f->flags & GFIFO_FLAG_CRITICAL) != 0u) {
        GFIFO_ENTER_CRITICAL(f);
    }
#else
    GFIFO_UNUSED(f);
#endif
}

/**
 * @brief 退出临界区，与 GFIFO_Lock 严格配对
 * @param f 句柄指针，不可为 NULL
 * @retval 无
 */
static void GFIFO_Unlock(gfifo_t *f)
{
#if GFIFO_CFG_ENABLE_CRITICAL
    if ((f->flags & GFIFO_FLAG_CRITICAL) != 0u) {
        GFIFO_EXIT_CRITICAL(f);
    }
#else
    GFIFO_UNUSED(f);
#endif
}

/**
 * @brief 把逻辑索引换算成缓冲区内的物理下标
 * @param f   句柄指针，不可为 NULL
 * @param idx 逻辑索引（累计计数器的值）
 * @retval 物理下标，范围 [0, size)
 * @note 两条路径：容量是 2 的幂时用掩码，一条 AND 指令；否则用取模。
 *       分支判断的是"这个 FIFO 的容量属性"，不是数据，预测器命中率 100%，
 *       所以即便走取模路径也没有额外的分支惩罚。
 */
static gfifo_idx_t GFIFO_Pos(const gfifo_t *f, gfifo_idx_t idx)
{
#if GFIFO_CFG_POW2_ONLY
    return (gfifo_idx_t)(idx & f->mask);
#else
    if ((f->flags & GFIFO_FLAG_POW2) != 0u) {
        return (gfifo_idx_t)(idx & f->mask);
    }
    return (gfifo_idx_t)(idx % f->size);
#endif
}

/**
 * @brief 已占用字节数（不加锁，调用方必须已在临界区内或确认是 SPSC）
 * @param f 句柄指针，不可为 NULL
 * @retval 已用字节数
 * @note 依赖无符号减法的回绕特性：head 与 tail 都单调递增并自然溢出，
 *       但 (head - tail) 在模 2^N 意义下恒等于真实差值，只要真实差值
 *       小于 2^N 就不会出错。容量被限制在 2^N - 1 以内，所以成立。
 */
static uint32_t GFIFO_Used_Raw(const gfifo_t *f)
{
    return (uint32_t)(gfifo_idx_t)(f->head - f->tail);
}

/**
 * @brief 剩余可写字节数（不加锁），已扣除未提交的预约区
 * @param f 句柄指针，不可为 NULL
 * @retval 剩余可写字节数
 * @note 三重保护：used 已经超过 size（计数器被踩）返回 0；rsv 把剩余量吃光
 *       返回 0；正常情况返回差值。绝不返回负数或巨大值。
 */
static uint32_t GFIFO_Free_Raw(const gfifo_t *f)
{
    uint32_t used  = GFIFO_Used_Raw(f);
    uint32_t total = (uint32_t)f->size;
    uint32_t rsv   = (uint32_t)f->rsv;

    if (used >= total) {
        return 0u;
    }
    if ((total - used) <= rsv) {
        return 0u;
    }
    return total - used - rsv;
}

/**
 * @brief 已存放的完整元素个数（不加锁）
 * @param f 句柄指针，不可为 NULL
 * @retval 元素个数
 */
static uint32_t GFIFO_Count_Raw(const gfifo_t *f)
{
    uint32_t elem = (uint32_t)f->elem_size;

    if (elem == 0u) {
        return 0u;
    }
    return GFIFO_Used_Raw(f) / elem;
}

/**
 * @brief 从逻辑位置 pos 起向缓冲区写入 len 字节，自动处理回绕
 * @param f   句柄指针，不可为 NULL
 * @param pos 起始逻辑索引
 * @param src 源数据，不可为 NULL
 * @param len 字节数，必须不超过当前空闲量（调用方保证）
 * @retval 无
 * @note 拆成最多两段拷贝：第一段写到缓冲区末尾，第二段从缓冲区开头续上。
 *       这是环形缓冲唯一需要"分两段"的地方，其余逻辑都基于逻辑索引，
 *       看不见物理边界。
 */
static void GFIFO_Write_At_Raw(gfifo_t *f, gfifo_idx_t pos,
                               const void *src, uint32_t len)
{
    const uint8_t *p     = (const uint8_t *)src;
    gfifo_idx_t    wpos  = GFIFO_Pos(f, pos);
    uint32_t       first = (uint32_t)(f->size - wpos);

    if (first > len) {
        first = len;
    }

    GFIFO_Mem_Copy(&f->buf[wpos], p, first);

    if (len > first) {
        GFIFO_Mem_Copy(f->buf, p + first, len - first);
    }
}

/**
 * @brief 从逻辑位置 pos 起读出 len 字节，自动处理回绕
 * @param f   句柄指针，不可为 NULL
 * @param pos 起始逻辑索引
 * @param dst 目标缓冲区，不可为 NULL
 * @param len 字节数，必须不超过当前已用量（调用方保证）
 * @retval 无
 */
static void GFIFO_Read_At_Raw(const gfifo_t *f, gfifo_idx_t pos,
                              void *dst, uint32_t len)
{
    uint8_t       *p     = (uint8_t *)dst;
    gfifo_idx_t    rpos  = GFIFO_Pos(f, pos);
    uint32_t       first = (uint32_t)(f->size - rpos);

    if (first > len) {
        first = len;
    }

    GFIFO_Mem_Copy(p, &f->buf[rpos], first);

    if (len > first) {
        GFIFO_Mem_Copy(p + first, f->buf, len - first);
    }
}

/**
 * @brief 从队首丢弃 need 字节，为覆盖写入腾空间（不加锁）
 * @param f    句柄指针，不可为 NULL
 * @param need 需要腾出的字节数
 * @retval 无
 * @note 元素模式下丢弃量向上对齐到整元素，避免出现半个结构体残留：那种状态
 *       会让后续所有 Get 永久错位，比丢一整个元素严重得多。
 * @note need >= used 时直接清空（tail 追上 head），这是最省事的边界处理，
 *       也顺带修掉了"计数器损坏导致 need 巨大"的退化情况。
 */
static void GFIFO_Discard_Raw(gfifo_t *f, uint32_t need)
{
    uint32_t used = GFIFO_Used_Raw(f);
    uint32_t elem = (uint32_t)f->elem_size;
    uint32_t drop;

    if (need >= used) {
        f->tail = f->head;
        return;
    }

    drop = need;
    if (elem > 1u) {
        drop = (uint32_t)GFIFO_ALIGN_UP(drop, elem);
    }
    if (drop > used) {
        drop = used;
    }

    f->tail = (gfifo_idx_t)(f->tail + (gfifo_idx_t)drop);
}

/**
 * @brief 更新峰值水位统计（不加锁）
 * @param f    句柄指针，不可为 NULL
 * @param used 刚刚写入完成后的已用字节数
 * @retval 无
 */
static void GFIFO_Touch_Peak_Raw(gfifo_t *f, uint32_t used)
{
#if GFIFO_CFG_ENABLE_STATS
    if (used > f->stat_peak) {
        f->stat_peak = used;
    }
#else
    GFIFO_UNUSED(f);
    GFIFO_UNUSED(used);
#endif
}

/**
 * @brief 记录一次溢出（不加锁）
 * @param f 句柄指针，不可为 NULL
 * @retval 无
 */
static void GFIFO_Note_Overflow_Raw(gfifo_t *f)
{
#if GFIFO_CFG_ENABLE_STATS
    f->stat_ovf++;
#else
    GFIFO_UNUSED(f);
#endif
}

/**
 * @brief 记录一次下溢（不加锁）
 * @param f 句柄指针，不可为 NULL
 * @retval 无
 */
static void GFIFO_Note_Underflow_Raw(gfifo_t *f)
{
#if GFIFO_CFG_ENABLE_STATS
    f->stat_unf++;
#else
    GFIFO_UNUSED(f);
#endif
}

/**
 * @brief 记录累计写入字节数（不加锁）
 * @param f   句柄指针，不可为 NULL
 * @param len 本次写入的字节数
 * @retval 无
 */
static void GFIFO_Note_In_Raw(gfifo_t *f, uint32_t len)
{
#if GFIFO_CFG_ENABLE_STATS
    f->stat_in += len;
#else
    GFIFO_UNUSED(f);
    GFIFO_UNUSED(len);
#endif
}

/**
 * @brief 记录累计读出字节数（不加锁）
 * @param f   句柄指针，不可为 NULL
 * @param len 本次消费的字节数
 * @retval 无
 */
static void GFIFO_Note_Out_Raw(gfifo_t *f, uint32_t len)
{
#if GFIFO_CFG_ENABLE_STATS
    f->stat_out += len;
#else
    GFIFO_UNUSED(f);
    GFIFO_UNUSED(len);
#endif
}

#if GFIFO_CFG_ENABLE_MSG
/**
 * @brief 把消息长度编码进长度头，固定小端序
 * @param dst 长度头缓冲区，不可为 NULL，至少可写 GFIFO_MSG_HDR 字节
 * @param len 消息体长度
 * @retval 无
 * @note 固定小端而不是主机序：同一个固件可能被烧到小端的 Cortex-M 与大端的
 *       PowerPC 上，用固定字节序可以让"存进 Flash 的抓包数据"在 PC 上回放时
 *       不需要额外转换。长度头只有 1~4 字节，编解码成本可忽略。
 */
static void GFIFO_Msg_Hdr_Write(uint8_t *dst, uint32_t len)
{
    dst[0] = (uint8_t)(len & 0xFFu);

#if (GFIFO_CFG_MSG_HDR_BYTES >= 2)
    dst[1] = (uint8_t)((len >> 8) & 0xFFu);
#endif
#if (GFIFO_CFG_MSG_HDR_BYTES >= 4)
    dst[2] = (uint8_t)((len >> 16) & 0xFFu);
    dst[3] = (uint8_t)((len >> 24) & 0xFFu);
#endif
}

/**
 * @brief 从长度头解码出消息长度，固定小端序
 * @param src 长度头缓冲区，不可为 NULL，至少可读 GFIFO_MSG_HDR 字节
 * @retval 消息体长度
 */
static uint32_t GFIFO_Msg_Hdr_Read(const uint8_t *src)
{
    uint32_t len = (uint32_t)src[0];

#if (GFIFO_CFG_MSG_HDR_BYTES >= 2)
    len |= ((uint32_t)src[1] << 8);
#endif
#if (GFIFO_CFG_MSG_HDR_BYTES >= 4)
    len |= ((uint32_t)src[2] << 16);
    len |= ((uint32_t)src[3] << 24);
#endif

    return len;
}

/**
 * @brief 读取队首消息的长度头并校验，不移动读指针（不加锁）
 * @param f   句柄指针，不可为 NULL
 * @param len 输出参数，返回消息体长度，不可为 NULL
 * @retval GFIFO_OK          读取成功
 * @retval GFIFO_ERR_EMPTY   队列里连长度头都不够
 * @retval GFIFO_ERR_CORRUPT 长度头解析出的长度超过队列现有数据量
 * @note 校验的两条判据缺一不可：既不能超过 FIFO 总容量（否则是野长度），
 *       也不能超过当前已用量（否则是"头写进去了数据没跟上"的残留）。
 *       后者在掉电重启、DMA 半途中断时真的会出现。
 */
static gfifo_err_t GFIFO_Msg_Head_Len_Raw(const gfifo_t *f, uint32_t *len)
{
    uint8_t  hdr[GFIFO_CFG_MSG_HDR_BYTES];
    uint32_t used = GFIFO_Used_Raw(f);
    uint32_t body;

    if (used < GFIFO_MSG_HDR) {
        return GFIFO_ERR_EMPTY;
    }

    GFIFO_Read_At_Raw(f, f->tail, hdr, GFIFO_MSG_HDR);
    body = GFIFO_Msg_Hdr_Read(hdr);

    if (body > (uint32_t)f->size) {
        return GFIFO_ERR_CORRUPT;
    }
    if ((body + GFIFO_MSG_HDR) > used) {
        return GFIFO_ERR_CORRUPT;
    }

    *len = body;
    return GFIFO_OK;
}

/**
 * @brief 丢弃队首的整条消息（不加锁）
 * @param f 句柄指针，不可为 NULL
 * @retval GFIFO_OK          丢弃成功
 * @retval GFIFO_ERR_EMPTY   队列为空
 * @retval GFIFO_ERR_CORRUPT 长度头损坏，已强制清空整个队列以自愈
 * @note 损坏时选择"清空整个队列"而不是"跳过若干字节"：长度头一旦不可信，
 *       后面所有消息的边界都无法确定，继续按偏移跳只会在错误的位置上
 *       产生更多假消息。清空是唯一能保证回到一致状态的做法。
 */
static gfifo_err_t GFIFO_Msg_Discard_Raw(gfifo_t *f)
{
    uint32_t body = 0u;
    gfifo_err_t ret = GFIFO_Msg_Head_Len_Raw(f, &body);

    if (ret == GFIFO_ERR_EMPTY) {
        /* 连一个完整长度头都没有：可能是掉电残留，直接回到空状态 */
        f->tail = f->head;
        return GFIFO_ERR_EMPTY;
    }
    if (ret != GFIFO_OK) {
        f->tail = f->head;
        return ret;
    }

    f->tail = (gfifo_idx_t)(f->tail + (gfifo_idx_t)(body + GFIFO_MSG_HDR));
    GFIFO_Note_Out_Raw(f, body + GFIFO_MSG_HDR);

    return GFIFO_OK;
}
#endif /* GFIFO_CFG_ENABLE_MSG */

/*==============================================================================
 * 生命周期
 *============================================================================*/

/**
 * @brief 初始化或重新初始化一个 FIFO，绑定调用方提供的存储
 * @param f         句柄指针，不可为 NULL；同一句柄可重复 Init，效果等于重置
 * @param buf       缓冲区基址，不可为 NULL，生命周期必须覆盖 FIFO 的全部使用期
 * @param size      缓冲区字节数，会被向下对齐到 elem_size 的整数倍后使用
 * @param elem_size 元素大小，1 表示纯字节流；结构体 FIFO 传 sizeof(结构体)
 * @param flags     GFIFO_FLAG_xxx 的组合，无特殊需求传 GFIFO_FLAG_NONE
 * @retval GFIFO_OK           初始化成功
 * @retval GFIFO_ERR_PARAM    f 或 buf 为 NULL；size 为 0；elem_size 为 0 或超过 65535；
 *                            flags 含未定义的位；对齐后可用容量为 0；
 *                            强制 2 的幂模式下容量不是 2 的幂
 * @retval GFIFO_ERR_TOOBIG   size 超过索引类型上限（uint16_t 索引时为 65535）
 * @note 本函数不是线程安全的，必须在任何生产者/消费者启动之前调用。
 * @note 先写字段、最后写 magic：这样即便初始化过程被中断打断，句柄也会被
 *       后续调用识别为"未初始化"，而不是半初始化状态下被误用。
 */
gfifo_err_t GFIFO_Init(gfifo_t *f, void *buf, uint32_t size,
                       uint32_t elem_size, uint16_t flags)
{
    uint32_t aligned;

    /* ---------- 参数校验 ---------- */
    if (f == NULL || buf == NULL) {
        return GFIFO_ERR_PARAM;
    }
    if (elem_size == 0u || elem_size > 0xFFFFu) {
        return GFIFO_ERR_PARAM;
    }
    if (size == 0u) {
        return GFIFO_ERR_PARAM;
    }
    if (size > (uint32_t)GFIFO_IDX_MAX) {
        return GFIFO_ERR_TOOBIG;
    }
    if ((flags & (uint16_t)(~GFIFO_FLAG_MASK_USER)) != 0u) {
        return GFIFO_ERR_PARAM;
    }

    /* ---------- 容量对齐到整元素 ---------- */
    aligned = size;
    if (elem_size > 1u) {
        aligned = (size / elem_size) * elem_size;
    }
    if (aligned == 0u) {
        return GFIFO_ERR_PARAM;
    }

#if GFIFO_CFG_POW2_ONLY
    /* 强制 2 的幂模式下，容量必须恰好是 2 的幂，否则掩码换算不成立 */
    if ((aligned & (aligned - 1u)) != 0u) {
        return GFIFO_ERR_PARAM;
    }
#endif

    /* ---------- 写字段 ---------- */
    f->buf       = (uint8_t *)buf;
    f->size      = (gfifo_idx_t)aligned;
    f->mask      = (gfifo_idx_t)(f->size - 1u);
    f->head      = (gfifo_idx_t)0;
    f->tail      = (gfifo_idx_t)0;
    f->rsv       = (gfifo_idx_t)0;
    f->elem_size = (uint16_t)elem_size;
    f->flags     = (uint16_t)(flags & GFIFO_FLAG_MASK_USER);

#if !GFIFO_CFG_POW2_ONLY
    /* 容量恰好是 2 的幂时走掩码快路径，否则走取模 */
    if ((aligned & (aligned - 1u)) == 0u) {
        f->flags |= (uint16_t)GFIFO_FLAG_POW2;
    }
#endif

#if GFIFO_CFG_ENABLE_STATS
    f->stat_in   = 0u;
    f->stat_out  = 0u;
    f->stat_peak = 0u;
    f->stat_ovf  = 0u;
    f->stat_unf  = 0u;
#endif

    /* ---------- 缓冲区清零 ---------- */
#if GFIFO_CFG_CLEAR_ON_INIT
    if ((flags & GFIFO_FLAG_NO_CLEAR) == 0u) {
        GFIFO_Mem_Set(f->buf, 0u, aligned);
    }
#endif

    /* ---------- 最后一步才置魔数 ---------- */
    f->magic = (uint16_t)GFIFO_MAGIC;

    return GFIFO_OK;
}

/**
 * @brief 清空 FIFO 里的全部数据，保留容量、元素大小、标志与统计配置
 * @param f 句柄指针，不可为 NULL
 * @retval GFIFO_OK         清空成功
 * @retval GFIFO_ERR_PARAM  f 为 NULL
 * @retval GFIFO_ERR_NOINIT 句柄未初始化
 * @note 会一并复位未提交的预留区，因此 Reserve 之后忘记 Commit 的悬挂状态
 *       可以用 Reset 兜底恢复。
 * @note 统计计数不做清零，需要清零请显式调 GFIFO_Stat_Reset()。
 */
gfifo_err_t GFIFO_Reset(gfifo_t *f)
{
    if (!GFIFO_Is_Valid(f)) {
        return (f == NULL) ? GFIFO_ERR_PARAM : GFIFO_ERR_NOINIT;
    }

    GFIFO_Lock(f);

    f->head = (gfifo_idx_t)0;
    f->tail = (gfifo_idx_t)0;
    f->rsv  = (gfifo_idx_t)0;
    GFIFO_BARRIER();

    GFIFO_Unlock(f);

    return GFIFO_OK;
}

/**
 * @brief 查询 FIFO 的实际可用字节容量
 * @param f 句柄指针，可为 NULL
 * @retval 可用字节数；f 为 NULL 或未初始化时返回 0
 */
uint32_t GFIFO_Capacity(const gfifo_t *f)
{
    if (!GFIFO_Is_Valid(f)) {
        return 0u;
    }
    return (uint32_t)f->size;
}

/**
 * @brief 查询单个元素的字节大小
 * @param f 句柄指针，可为 NULL
 * @retval 元素字节数；f 为 NULL 或未初始化时返回 0
 */
uint32_t GFIFO_Elem_Size(const gfifo_t *f)
{
    if (!GFIFO_Is_Valid(f)) {
        return 0u;
    }
    return (uint32_t)f->elem_size;
}

/**
 * @brief 查询最多能容纳多少个元素
 * @param f 句柄指针，可为 NULL
 * @retval 元素个数上限；f 为 NULL 或未初始化时返回 0
 */
uint32_t GFIFO_Elem_Capacity(const gfifo_t *f)
{
    if (!GFIFO_Is_Valid(f)) {
        return 0u;
    }
    if (f->elem_size == 0u) {
        return 0u;
    }
    return (uint32_t)f->size / (uint32_t)f->elem_size;
}

/*==============================================================================
 * 状态查询
 *============================================================================*/

/**
 * @brief 查询已占用的字节数
 * @param f 句柄指针，可为 NULL
 * @retval 已用字节数，范围 [0, GFIFO_Capacity()]；f 为 NULL 或未初始化时返回 0
 * @note 对超过容量的异常值做了夹紧：计数器被踩时返回容量而不是一个天文数字，
 *       这样上层拿它算数组下标也不会越界。返回"刚好等于容量"就是提示。
 */
uint32_t GFIFO_Used(const gfifo_t *f)
{
    uint32_t used;

    if (!GFIFO_Is_Valid(f)) {
        return 0u;
    }

    used = GFIFO_Used_Raw(f);
    if (used > (uint32_t)f->size) {
        used = (uint32_t)f->size;
    }
    return used;
}

/**
 * @brief 查询还能写入的字节数
 * @param f 句柄指针，可为 NULL
 * @retval 剩余可写字节数，已扣除未提交的预留区；f 为 NULL 或未初始化时返回 0
 */
uint32_t GFIFO_Free(const gfifo_t *f)
{
    if (!GFIFO_Is_Valid(f)) {
        return 0u;
    }
    return GFIFO_Free_Raw(f);
}

/**
 * @brief 查询已存放的完整元素个数
 * @param f 句柄指针，可为 NULL
 * @retval 元素个数；f 为 NULL 或未初始化时返回 0
 */
uint32_t GFIFO_Count(const gfifo_t *f)
{
    if (!GFIFO_Is_Valid(f)) {
        return 0u;
    }
    return GFIFO_Count_Raw(f);
}

/**
 * @brief 查询还能再放入多少个完整元素
 * @param f 句柄指针，可为 NULL
 * @retval 可写元素个数；f 为 NULL 或未初始化时返回 0
 */
uint32_t GFIFO_Space(const gfifo_t *f)
{
    if (!GFIFO_Is_Valid(f)) {
        return 0u;
    }
    if (f->elem_size == 0u) {
        return 0u;
    }
    return GFIFO_Free_Raw(f) / (uint32_t)f->elem_size;
}

/**
 * @brief 判断 FIFO 是否为空
 * @param f 句柄指针，可为 NULL
 * @retval 1 空
 * @retval 0 非空
 */
uint8_t GFIFO_Is_Empty(const gfifo_t *f)
{
    if (!GFIFO_Is_Valid(f)) {
        return 1u;
    }
    return (GFIFO_Used_Raw(f) == 0u) ? 1u : 0u;
}

/**
 * @brief 判断 FIFO 是否已满（连一个元素都放不下）
 * @param f 句柄指针，可为 NULL
 * @retval 1 满
 * @retval 0 未满
 * @note 未初始化的句柄按"满"处理：让"满了就等待"的调用方自然停在等待分支，
 *       而不是在无效句柄上空转。
 */
uint8_t GFIFO_Is_Full(const gfifo_t *f)
{
    if (!GFIFO_Is_Valid(f)) {
        return 1u;
    }
    return (GFIFO_Free_Raw(f) < (uint32_t)f->elem_size) ? 1u : 0u;
}

/*==============================================================================
 * 字节流接口
 *============================================================================*/

/**
 * @brief 写入一段字节，空间不足时按"能写多少写多少"处理
 * @param f   句柄指针，不可为 NULL
 * @param src 源数据地址，不可为 NULL
 * @param len 请求写入的字节数，必须大于 0
 * @retval >0 实际写入的字节数，可能小于 len
 * @retval 0  一个字节都没写
 * @note 元素模式下 len 会被向下对齐到 elem_size 的整数倍，回来的是对齐后的
 *       实际值，调用方按返回值推进源指针即可，不会写出半个元素。
 */
uint32_t GFIFO_Write(gfifo_t *f, const void *src, uint32_t len)
{
    uint32_t want;
    uint32_t space;
    uint32_t elem;
    uint8_t  overwritten = 0u;

    if (src == NULL || len == 0u) {
        return 0u;
    }
    if (!GFIFO_Is_Valid(f)) {
        return 0u;
    }

    /* ---------- 对齐到整元素 ---------- */
    elem = (uint32_t)f->elem_size;
    if (elem > 1u) {
        len = (len / elem) * elem;
        if (len == 0u) {
            return 0u;
        }
    }

    /* want 取"对齐之后"的请求量：元素对齐不是失败，不该记溢出 */
    want = len;

    /* ---------- 单次写入不能超过总容量 ---------- */
    if (len > (uint32_t)f->size) {
        len = (uint32_t)f->size;
    }

    GFIFO_Lock(f);

    /* ---------- 空间不足时按策略处理 ---------- */
    space = GFIFO_Free_Raw(f);
    if (len > space) {
        if ((f->flags & GFIFO_FLAG_OVERWRITE) != 0u) {
            GFIFO_Discard_Raw(f, len - space);
            space = GFIFO_Free_Raw(f);
            if (len > space) {
                len = space;
            }
            /* 只要真的丢了旧数据，就算一次溢出，哪怕最终全写进去了 */
            overwritten = 1u;
        } else {
            len = space;
        }
    }

    /* ---------- 拷贝并推进 head ---------- */
    if (len > 0u) {
        GFIFO_Write_At_Raw(f, f->head, src, len);

        /* 数据必须先于 head 对新读者可见，这是 SPSC 无锁的正确性基础 */
        GFIFO_BARRIER();
        f->head = (gfifo_idx_t)(f->head + (gfifo_idx_t)len);
        GFIFO_BARRIER();

        GFIFO_Note_In_Raw(f, len);
        GFIFO_Touch_Peak_Raw(f, GFIFO_Used_Raw(f));
    }

    if ((len < want) || (overwritten != 0u)) {
        GFIFO_Note_Overflow_Raw(f);
    }

    GFIFO_Unlock(f);

    return len;
}

/**
 * @brief 读出一段字节，数据不足时按"有多少读多少"处理
 * @param f   句柄指针，不可为 NULL
 * @param dst 目标缓冲区地址，不可为 NULL
 * @param len 请求读取的字节数，必须大于 0
 * @retval >0 实际读出的字节数，可能小于 len
 * @retval 0  一个字节都没读
 * @note 元素模式下 len 会被向下对齐到 elem_size 的整数倍。
 */
uint32_t GFIFO_Read(gfifo_t *f, void *dst, uint32_t len)
{
    uint32_t want;
    uint32_t avail;
    uint32_t elem;

    if (dst == NULL || len == 0u) {
        return 0u;
    }
    if (!GFIFO_Is_Valid(f)) {
        return 0u;
    }

    elem = (uint32_t)f->elem_size;
    if (elem > 1u) {
        len = (len / elem) * elem;
        if (len == 0u) {
            return 0u;
        }
    }

    /* want 取对齐之后的量：对齐截断不是"数据不足"，不该记下溢 */
    want = len;

    GFIFO_Lock(f);

    /* ---------- 能读多少读多少 ---------- */
    avail = GFIFO_Used_Raw(f);
    if (len > avail) {
        len = avail;
    }

    if (len > 0u) {
        GFIFO_Read_At_Raw(f, f->tail, dst, len);

        /* 数据读完之后才推进 tail：否则消费者推进了 tail 却还没读完，
           生产者可能立刻覆盖那段内存，读出来就是脏数据 */
        GFIFO_BARRIER();
        f->tail = (gfifo_idx_t)(f->tail + (gfifo_idx_t)len);
        GFIFO_BARRIER();

        GFIFO_Note_Out_Raw(f, len);
    }

    if (len < want) {
        GFIFO_Note_Underflow_Raw(f);
    }

    GFIFO_Unlock(f);

    return len;
}

/**
 * @brief 写入单个字节
 * @param f    句柄指针，不可为 NULL
 * @param byte 待写入的字节
 * @retval GFIFO_OK         写入成功
 * @retval GFIFO_ERR_PARAM  f 为 NULL，或该 FIFO 的元素大小不是 1
 * @retval GFIFO_ERR_NOINIT 句柄未初始化
 * @retval GFIFO_ERR_FULL   空间不足，未写入
 * @note 只允许在元素大小为 1 的 FIFO 上使用。元素大小大于 1 时写一个字节会
 *       把后续所有元素错位，所以直接拒绝而不是"宽容处理"——这种错误必须
 *       在开发阶段暴露出来。
 */
gfifo_err_t GFIFO_Write_Byte(gfifo_t *f, uint8_t byte)
{
    uint8_t overwritten = 0u;

    if (!GFIFO_Is_Valid(f)) {
        return (f == NULL) ? GFIFO_ERR_PARAM : GFIFO_ERR_NOINIT;
    }
    if (f->elem_size != 1u) {
        return GFIFO_ERR_PARAM;
    }

    GFIFO_Lock(f);

    if (GFIFO_Free_Raw(f) < 1u) {
        if ((f->flags & GFIFO_FLAG_OVERWRITE) != 0u) {
            GFIFO_Discard_Raw(f, 1u);
            if (GFIFO_Free_Raw(f) < 1u) {
                GFIFO_Note_Overflow_Raw(f);
                GFIFO_Unlock(f);
                return GFIFO_ERR_FULL;
            }
            overwritten = 1u;
        } else {
            GFIFO_Note_Overflow_Raw(f);
            GFIFO_Unlock(f);
            return GFIFO_ERR_FULL;
        }
    }

    GFIFO_Write_At_Raw(f, f->head, &byte, 1u);
    GFIFO_BARRIER();
    f->head = (gfifo_idx_t)(f->head + (gfifo_idx_t)1u);
    GFIFO_BARRIER();

    GFIFO_Note_In_Raw(f, 1u);
    GFIFO_Touch_Peak_Raw(f, GFIFO_Used_Raw(f));

    if (overwritten != 0u) {
        GFIFO_Note_Overflow_Raw(f);
    }

    GFIFO_Unlock(f);

    return GFIFO_OK;
}

/**
 * @brief 读出单个字节
 * @param f    句柄指针，不可为 NULL
 * @param byte 输出参数，不可为 NULL
 * @retval GFIFO_OK         读取成功
 * @retval GFIFO_ERR_PARAM  f 或 byte 为 NULL，或该 FIFO 的元素大小不是 1
 * @retval GFIFO_ERR_NOINIT 句柄未初始化
 * @retval GFIFO_ERR_EMPTY  FIFO 为空
 */
gfifo_err_t GFIFO_Read_Byte(gfifo_t *f, uint8_t *byte)
{
    if (byte == NULL) {
        return GFIFO_ERR_PARAM;
    }
    if (!GFIFO_Is_Valid(f)) {
        return (f == NULL) ? GFIFO_ERR_PARAM : GFIFO_ERR_NOINIT;
    }
    if (f->elem_size != 1u) {
        return GFIFO_ERR_PARAM;
    }

    GFIFO_Lock(f);

    if (GFIFO_Used_Raw(f) < 1u) {
        GFIFO_Note_Underflow_Raw(f);
        GFIFO_Unlock(f);
        return GFIFO_ERR_EMPTY;
    }

    GFIFO_Read_At_Raw(f, f->tail, byte, 1u);
    GFIFO_BARRIER();
    f->tail = (gfifo_idx_t)(f->tail + (gfifo_idx_t)1u);
    GFIFO_BARRIER();

    GFIFO_Note_Out_Raw(f, 1u);

    GFIFO_Unlock(f);

    return GFIFO_OK;
}

/*==============================================================================
 * 定长元素接口
 *============================================================================*/

/**
 * @brief 放入一个完整元素（结构体 FIFO 的主力接口）
 * @param f    句柄指针，不可为 NULL
 * @param elem 指向待写入元素的指针，不可为 NULL
 * @retval GFIFO_OK         写入成功
 * @retval GFIFO_ERR_PARAM  f 或 elem 为 NULL
 * @retval GFIFO_ERR_NOINIT 句柄未初始化
 * @retval GFIFO_ERR_FULL   剩余空间不足一个元素，未写入
 * @note 与 GFIFO_Write 的根本区别：本函数要么完整写入一个元素，要么一个字节
 *       都不写。元素 FIFO 必须用本函数，否则一旦发生部分写入，后续每一次
 *       Get 都会读到错位的数据，而且从现象上完全看不出是"写坏了"。
 */
gfifo_err_t GFIFO_Put(gfifo_t *f, const void *elem)
{
    uint32_t elem_size;
    uint32_t free_bytes;
    uint8_t  overwritten = 0u;

    if (elem == NULL) {
        return GFIFO_ERR_PARAM;
    }
    if (!GFIFO_Is_Valid(f)) {
        return (f == NULL) ? GFIFO_ERR_PARAM : GFIFO_ERR_NOINIT;
    }

    elem_size = (uint32_t)f->elem_size;

    GFIFO_Lock(f);

    /* ---------- 空间检查：覆盖模式先腾地方 ---------- */
    free_bytes = GFIFO_Free_Raw(f);
    if (free_bytes < elem_size) {
        if ((f->flags & GFIFO_FLAG_OVERWRITE) != 0u) {
            GFIFO_Discard_Raw(f, elem_size);
            if (GFIFO_Free_Raw(f) < elem_size) {
                GFIFO_Note_Overflow_Raw(f);
                GFIFO_Unlock(f);
                return GFIFO_ERR_FULL;
            }
            overwritten = 1u;
        } else {
            GFIFO_Note_Overflow_Raw(f);
            GFIFO_Unlock(f);
            return GFIFO_ERR_FULL;
        }
    }

    /* ---------- 整块写入 ---------- */
    GFIFO_Write_At_Raw(f, f->head, elem, elem_size);

    GFIFO_BARRIER();
    f->head = (gfifo_idx_t)(f->head + (gfifo_idx_t)elem_size);
    GFIFO_BARRIER();

    GFIFO_Note_In_Raw(f, elem_size);
    GFIFO_Touch_Peak_Raw(f, GFIFO_Used_Raw(f));

    if (overwritten != 0u) {
        GFIFO_Note_Overflow_Raw(f);
    }

    GFIFO_Unlock(f);

    return GFIFO_OK;
}

/**
 * @brief 取出一个完整元素，取出后该元素从 FIFO 中移除
 * @param f    句柄指针，不可为 NULL
 * @param elem 输出参数，至少可写 elem_size 字节，不可为 NULL
 * @retval GFIFO_OK         读取成功
 * @retval GFIFO_ERR_PARAM  f 或 elem 为 NULL
 * @retval GFIFO_ERR_NOINIT 句柄未初始化
 * @retval GFIFO_ERR_EMPTY  FIFO 为空，elem 未被修改
 */
gfifo_err_t GFIFO_Get(gfifo_t *f, void *elem)
{
    uint32_t elem_size;

    if (elem == NULL) {
        return GFIFO_ERR_PARAM;
    }
    if (!GFIFO_Is_Valid(f)) {
        return (f == NULL) ? GFIFO_ERR_PARAM : GFIFO_ERR_NOINIT;
    }

    elem_size = (uint32_t)f->elem_size;

    GFIFO_Lock(f);

    if (GFIFO_Used_Raw(f) < elem_size) {
        GFIFO_Note_Underflow_Raw(f);
        GFIFO_Unlock(f);
        return GFIFO_ERR_EMPTY;
    }

    GFIFO_Read_At_Raw(f, f->tail, elem, elem_size);

    GFIFO_BARRIER();
    f->tail = (gfifo_idx_t)(f->tail + (gfifo_idx_t)elem_size);
    GFIFO_BARRIER();

    GFIFO_Note_Out_Raw(f, elem_size);

    GFIFO_Unlock(f);

    return GFIFO_OK;
}

/**
 * @brief 查看最旧的元素但不移除（偷看）
 * @param f    句柄指针，不可为 NULL
 * @param elem 输出参数，至少可写 elem_size 字节，不可为 NULL
 * @retval GFIFO_OK         读取成功
 * @retval GFIFO_ERR_PARAM  f 或 elem 为 NULL
 * @retval GFIFO_ERR_NOINIT 句柄未初始化
 * @retval GFIFO_ERR_EMPTY  FIFO 为空，elem 未被修改
 * @note 只读不改 tail，所以在 SPSC 下 Peek 与 Get 之间可能被生产者插入新数据，
 *       但队首元素本身不会被生产者动（生产者只碰 head 一侧），所以"看一眼
 *       再决定要不要取"是安全的。
 */
gfifo_err_t GFIFO_Peek(gfifo_t *f, void *elem)
{
    uint32_t elem_size;

    if (elem == NULL) {
        return GFIFO_ERR_PARAM;
    }
    if (!GFIFO_Is_Valid(f)) {
        return (f == NULL) ? GFIFO_ERR_PARAM : GFIFO_ERR_NOINIT;
    }

    elem_size = (uint32_t)f->elem_size;

    GFIFO_Lock(f);

    if (GFIFO_Used_Raw(f) < elem_size) {
        GFIFO_Note_Underflow_Raw(f);
        GFIFO_Unlock(f);
        return GFIFO_ERR_EMPTY;
    }

    GFIFO_Read_At_Raw(f, f->tail, elem, elem_size);

    GFIFO_Unlock(f);

    return GFIFO_OK;
}

/**
 * @brief 查看指定位置的元素但不移除
 * @param f     句柄指针，不可为 NULL
 * @param index 相对最旧元素的偏移，0 表示队首
 * @param elem  输出参数，不可为 NULL
 * @retval GFIFO_OK         读取成功
 * @retval GFIFO_ERR_PARAM  f 或 elem 为 NULL
 * @retval GFIFO_ERR_NOINIT 句柄未初始化
 * @retval GFIFO_ERR_EMPTY  index 越界或 FIFO 为空
 * @note 直接由 tail + index * elem_size 换算，不遍历，复杂度 O(1)。
 *       越界判断用的是"元素个数"而不是"字节数"，所以不需要额外的除法。
 */
gfifo_err_t GFIFO_Peek_At(gfifo_t *f, uint32_t index, void *elem)
{
    uint32_t    elem_size;
    gfifo_idx_t pos;

    if (elem == NULL) {
        return GFIFO_ERR_PARAM;
    }
    if (!GFIFO_Is_Valid(f)) {
        return (f == NULL) ? GFIFO_ERR_PARAM : GFIFO_ERR_NOINIT;
    }

    elem_size = (uint32_t)f->elem_size;

    GFIFO_Lock(f);

    if (index >= GFIFO_Count_Raw(f)) {
        GFIFO_Note_Underflow_Raw(f);
        GFIFO_Unlock(f);
        return GFIFO_ERR_EMPTY;
    }

    /* index < count 保证 index * elem_size < used <= size，不会溢出 */
    pos = (gfifo_idx_t)(f->tail + (gfifo_idx_t)(index * elem_size));
    GFIFO_Read_At_Raw(f, pos, elem, elem_size);

    GFIFO_Unlock(f);

    return GFIFO_OK;
}

/**
 * @brief 丢弃最旧的若干个元素
 * @param f     句柄指针，不可为 NULL
 * @param count 要丢弃的元素个数，超过已有数量时按实际数量丢弃
 * @retval GFIFO_OK         丢弃完成
 * @retval GFIFO_ERR_PARAM  f 为 NULL
 * @retval GFIFO_ERR_NOINIT 句柄未初始化
 * @note 计入 stat_out（视为已消费），不计入下溢：主动丢弃是一种正常业务行为，
 *       不是"没数据可读"的错误。
 */
gfifo_err_t GFIFO_Drop(gfifo_t *f, uint32_t count)
{
    uint32_t avail;
    uint32_t drop;
    uint32_t bytes;

    if (!GFIFO_Is_Valid(f)) {
        return (f == NULL) ? GFIFO_ERR_PARAM : GFIFO_ERR_NOINIT;
    }
    if (count == 0u) {
        return GFIFO_OK;
    }

    GFIFO_Lock(f);

    avail = GFIFO_Count_Raw(f);
    drop  = (count > avail) ? avail : count;
    bytes = drop * (uint32_t)f->elem_size;

    f->tail = (gfifo_idx_t)(f->tail + (gfifo_idx_t)bytes);
    GFIFO_Note_Out_Raw(f, bytes);

    GFIFO_Unlock(f);

    return GFIFO_OK;
}

/*==============================================================================
 * 零拷贝接口
 *============================================================================*/

/**
 * @brief 取得队首之后的一段连续可读区域，不做任何拷贝
 * @param f   句柄指针，不可为 NULL
 * @param ptr 输出参数，返回指向 FIFO 内部缓冲的指针，不可为 NULL
 * @retval >0 第一段连续可读的字节数
 * @retval 0  FIFO 为空或参数非法，*ptr 被置为 NULL
 * @note 返回值受物理边界限制：如果队首靠近缓冲区末尾，这次只返回"到末尾为止"
 *       的那一段，剩下部分要再调一次才能拿到。所以标准用法是 while 循环。
 * @note 返回的指针在下一次写操作或下一次 Skip 之前有效，不要缓存。
 */
uint32_t GFIFO_Peek_Span(const gfifo_t *f, const void **ptr)
{
    uint32_t    used;
    gfifo_idx_t rpos;
    uint32_t    cont;

    if (ptr == NULL) {
        return 0u;
    }
    *ptr = NULL;

    if (!GFIFO_Is_Valid(f)) {
        return 0u;
    }

    used = GFIFO_Used_Raw(f);
    if (used == 0u) {
        return 0u;
    }

    rpos = GFIFO_Pos(f, f->tail);
    cont = (uint32_t)(f->size - rpos);
    if (cont > used) {
        cont = used;
    }

    *ptr = (const void *)&f->buf[rpos];

    return cont;
}

/**
 * @brief 跳过（消费掉）指定长度的数据，不拷贝内容
 * @param f   句柄指针，不可为 NULL
 * @param len 要消费的**字节数**（不是元素个数），超过已有数据时按实际数量处理
 * @retval GFIFO_OK         处理完成
 * @retval GFIFO_ERR_PARAM  f 为 NULL
 * @retval GFIFO_ERR_NOINIT 句柄未初始化
 * @note 元素模式下 len 向下对齐到整元素，避免消费半个元素造成永久错位。
 *       注意对齐是"向下"取整：24 字节元素时 Skip(20) 什么都不做。
 *       要按个数丢，用 GFIFO_Drop()，它的参数才是元素个数。
 */
gfifo_err_t GFIFO_Skip(gfifo_t *f, uint32_t len)
{
    uint32_t avail;
    uint32_t elem;

    if (!GFIFO_Is_Valid(f)) {
        return (f == NULL) ? GFIFO_ERR_PARAM : GFIFO_ERR_NOINIT;
    }
    if (len == 0u) {
        return GFIFO_OK;
    }

    elem = (uint32_t)f->elem_size;
    if (elem > 1u) {
        len = (len / elem) * elem;
        if (len == 0u) {
            return GFIFO_OK;
        }
    }

    GFIFO_Lock(f);

    avail = GFIFO_Used_Raw(f);
    if (len > avail) {
        len = avail;
    }

    f->tail = (gfifo_idx_t)(f->tail + (gfifo_idx_t)len);
    GFIFO_Note_Out_Raw(f, len);

    GFIFO_Unlock(f);

    return GFIFO_OK;
}

/**
 * @brief 预约一段写入空间，拿到可直接写的两段指针，稍后再提交
 * @param f    句柄指针，不可为 NULL
 * @param len  要预约的字节数，必须大于 0
 * @param span 输出参数，不可为 NULL
 * @retval GFIFO_OK         预约成功
 * @retval GFIFO_ERR_PARAM  f 或 span 为 NULL，或 len 为 0，或本功能被裁剪
 * @retval GFIFO_ERR_NOINIT 句柄未初始化
 * @retval GFIFO_ERR_STATE  已有未提交的预约
 * @retval GFIFO_ERR_FULL   空闲空间不足
 * @retval GFIFO_ERR_TOOBIG len 超过总容量
 * @note 预约期间这段空间被"占住"但不对外可见：rsv 会从空闲量里扣掉，读者看到
 *       的 used 不包含它。所以 DMA 正在灌数据的那几百微秒里，消费者完全不会
 *       碰到这块内存，也就不需要有任何"等 DMA 完成"的握手。
 */
gfifo_err_t GFIFO_Write_Reserve(gfifo_t *f, uint32_t len, gfifo_span_t *span)
{
#if GFIFO_CFG_ENABLE_RESERVE
    gfifo_idx_t wpos;
    uint32_t    first;
    uint32_t    second;

    if (span == NULL) {
        return GFIFO_ERR_PARAM;
    }
    span->p1 = NULL;
    span->l1 = 0u;
    span->p2 = NULL;
    span->l2 = 0u;

    if (len == 0u) {
        return GFIFO_ERR_PARAM;
    }
    if (!GFIFO_Is_Valid(f)) {
        return (f == NULL) ? GFIFO_ERR_PARAM : GFIFO_ERR_NOINIT;
    }
    if (len > (uint32_t)f->size) {
        return GFIFO_ERR_TOOBIG;
    }

    GFIFO_Lock(f);

    if (f->rsv != 0u) {
        GFIFO_Unlock(f);
        return GFIFO_ERR_STATE;
    }
    if (len > GFIFO_Free_Raw(f)) {
        GFIFO_Note_Overflow_Raw(f);
        GFIFO_Unlock(f);
        return GFIFO_ERR_FULL;
    }

    wpos   = GFIFO_Pos(f, f->head);
    first  = (uint32_t)(f->size - wpos);
    if (first > len) {
        first = len;
    }
    second = len - first;

    span->p1 = &f->buf[wpos];
    span->l1 = first;
    if (second > 0u) {
        span->p2 = f->buf;
        span->l2 = second;
    }

    /* 占住空间：此刻起 Free_Raw 会把它扣掉，但读者看不到它 */
    f->rsv = (gfifo_idx_t)len;

    GFIFO_Unlock(f);

    return GFIFO_OK;
#else
    if (span != NULL) {
        span->p1 = NULL;
        span->l1 = 0u;
        span->p2 = NULL;
        span->l2 = 0u;
    }
    GFIFO_UNUSED(f);
    GFIFO_UNUSED(len);
    return GFIFO_ERR_PARAM;
#endif
}

/**
 * @brief 提交此前预约的全部空间，让数据对消费者可见
 * @param f 句柄指针，不可为 NULL
 * @retval GFIFO_OK         提交成功
 * @retval GFIFO_ERR_PARAM  f 为 NULL，或本功能被裁剪
 * @retval GFIFO_ERR_NOINIT 句柄未初始化
 * @retval GFIFO_ERR_STATE  当前没有未提交的预约
 * @note 先屏障、后推进 head：保证 DMA 或 CPU 写入的数据先于 head 更新可见。
 *       如果平台是弱内存序（Cortex-A、部分 RISC-V），GFIFO_BARRIER 必须被
 *       覆盖成真正的数据屏障，否则消费者可能读到旧数据。
 */
gfifo_err_t GFIFO_Write_Commit(gfifo_t *f)
{
#if GFIFO_CFG_ENABLE_RESERVE
    gfifo_idx_t pending;

    if (!GFIFO_Is_Valid(f)) {
        return (f == NULL) ? GFIFO_ERR_PARAM : GFIFO_ERR_NOINIT;
    }

    GFIFO_Lock(f);

    pending = f->rsv;
    if (pending == 0u) {
        GFIFO_Unlock(f);
        return GFIFO_ERR_STATE;
    }

    GFIFO_BARRIER();
    f->head = (gfifo_idx_t)(f->head + pending);
    f->rsv  = (gfifo_idx_t)0;
    GFIFO_BARRIER();

    GFIFO_Note_In_Raw(f, (uint32_t)pending);
    GFIFO_Touch_Peak_Raw(f, GFIFO_Used_Raw(f));

    GFIFO_Unlock(f);

    return GFIFO_OK;
#else
    GFIFO_UNUSED(f);
    return GFIFO_ERR_PARAM;
#endif
}

/**
 * @brief 放弃此前预约的空间，不写入任何数据，空间立即归还
 * @param f 句柄指针，不可为 NULL
 * @retval 无
 * @note 异常路径专用（DMA 传输失败、长度算错）。正常路径用 Commit。
 */
void GFIFO_Write_Abort(gfifo_t *f)
{
#if GFIFO_CFG_ENABLE_RESERVE
    if (!GFIFO_Is_Valid(f)) {
        return;
    }

    GFIFO_Lock(f);
    f->rsv = (gfifo_idx_t)0;
    GFIFO_Unlock(f);
#else
    GFIFO_UNUSED(f);
#endif
}

/*==============================================================================
 * 变长消息接口
 *============================================================================*/

/**
 * @brief 放入一条变长消息，自动附加长度头
 * @param f   句柄指针，不可为 NULL；元素大小必须为 1
 * @param msg 消息内容地址，不可为 NULL
 * @param len 消息长度，必须大于 0
 * @retval GFIFO_OK         写入成功
 * @retval GFIFO_ERR_PARAM  f 或 msg 为 NULL；len 为 0 或超出长度头表示范围；
 *                          元素大小不是 1；本功能被裁剪
 * @retval GFIFO_ERR_NOINIT 句柄未初始化
 * @retval GFIFO_ERR_FULL   剩余空间放不下"长度头 + 消息"
 * @retval GFIFO_ERR_TOOBIG len 加长度头后超过总容量
 * @note 长度头与消息体在同一把锁内写完，之后才推进 head，所以对读者而言
 *       整条消息是原子出现的，不存在"头有了、体还没有"的中间态。
 * @note 覆盖模式下会整条整条地丢旧消息，直到放得下新消息为止。若丢光了
 *       仍放不下（说明新消息本身就超过总容量），返回 FULL 而不是硬塞。
 */
gfifo_err_t GFIFO_Msg_Put(gfifo_t *f, const void *msg, uint32_t len)
{
#if GFIFO_CFG_ENABLE_MSG
    uint8_t     hdr[GFIFO_CFG_MSG_HDR_BYTES];
    uint32_t    need;
    uint8_t     overwritten = 0u;

    if (msg == NULL || len == 0u) {
        return GFIFO_ERR_PARAM;
    }
    if (!GFIFO_Is_Valid(f)) {
        return (f == NULL) ? GFIFO_ERR_PARAM : GFIFO_ERR_NOINIT;
    }
    if (f->elem_size != 1u) {
        return GFIFO_ERR_PARAM;
    }
    if (len > GFIFO_MSG_MAX_LEN) {
        return GFIFO_ERR_PARAM;
    }

    need = len + GFIFO_MSG_HDR;
    if (need > (uint32_t)f->size) {
        return GFIFO_ERR_TOOBIG;
    }

    GFIFO_Msg_Hdr_Write(hdr, len);

    GFIFO_Lock(f);

    /* ---------- 空间不足：整条丢弃旧消息 ---------- */
    if (GFIFO_Free_Raw(f) < need) {
        if ((f->flags & GFIFO_FLAG_OVERWRITE) != 0u) {
            while (GFIFO_Free_Raw(f) < need) {
                if (GFIFO_Used_Raw(f) == 0u) {
                    break;
                }
                (void)GFIFO_Msg_Discard_Raw(f);
            }
            if (GFIFO_Free_Raw(f) < need) {
                GFIFO_Note_Overflow_Raw(f);
                GFIFO_Unlock(f);
                return GFIFO_ERR_FULL;
            }
            overwritten = 1u;
        } else {
            GFIFO_Note_Overflow_Raw(f);
            GFIFO_Unlock(f);
            return GFIFO_ERR_FULL;
        }
    }

    /* ---------- 先写头，再写体，最后一次性推进 ---------- */
    GFIFO_Write_At_Raw(f, f->head, hdr, GFIFO_MSG_HDR);
    GFIFO_Write_At_Raw(f, (gfifo_idx_t)(f->head + (gfifo_idx_t)GFIFO_MSG_HDR),
                       msg, len);

    GFIFO_BARRIER();
    f->head = (gfifo_idx_t)(f->head + (gfifo_idx_t)need);
    GFIFO_BARRIER();

    GFIFO_Note_In_Raw(f, need);
    GFIFO_Touch_Peak_Raw(f, GFIFO_Used_Raw(f));

    if (overwritten != 0u) {
        GFIFO_Note_Overflow_Raw(f);
    }

    GFIFO_Unlock(f);

    return GFIFO_OK;
#else
    GFIFO_UNUSED(f);
    GFIFO_UNUSED(msg);
    GFIFO_UNUSED(len);
    return GFIFO_ERR_PARAM;
#endif
}

/**
 * @brief 取出一条变长消息，取出后该消息从 FIFO 中移除
 * @param f       句柄指针，不可为 NULL
 * @param dst     输出缓冲区，不可为 NULL
 * @param cap     dst 的可写字节数
 * @param out_len 输出参数，写入实际消息长度，可为 NULL
 * @retval GFIFO_OK           读取成功
 * @retval GFIFO_ERR_PARAM    f 或 dst 为 NULL，或本功能被裁剪
 * @retval GFIFO_ERR_NOINIT   句柄未初始化
 * @retval GFIFO_ERR_EMPTY    FIFO 里没有消息
 * @retval GFIFO_ERR_TOOSMALL cap 小于队首消息长度，消息保持不动
 * @retval GFIFO_ERR_CORRUPT  长度头损坏
 * @note TOOSMALL 时消息原地不动，调用方可以换个更大的缓冲区重试，也可以
 *       用 GFIFO_Msg_Drop 主动丢弃。库不会替调用方丢数据。
 */
gfifo_err_t GFIFO_Msg_Get(gfifo_t *f, void *dst, uint32_t cap, uint32_t *out_len)
{
#if GFIFO_CFG_ENABLE_MSG
    uint32_t    body = 0u;
    gfifo_err_t ret;

    if (dst == NULL) {
        return GFIFO_ERR_PARAM;
    }
    if (!GFIFO_Is_Valid(f)) {
        return (f == NULL) ? GFIFO_ERR_PARAM : GFIFO_ERR_NOINIT;
    }

    GFIFO_Lock(f);

    ret = GFIFO_Msg_Head_Len_Raw(f, &body);
    if (ret != GFIFO_OK) {
        if (ret == GFIFO_ERR_CORRUPT) {
            /* 头已损坏，无法定位下一条消息，清空以回到一致状态 */
            f->tail = f->head;
        }
        GFIFO_Unlock(f);
        return ret;
    }

    if (cap < body) {
        GFIFO_Unlock(f);
        return GFIFO_ERR_TOOSMALL;
    }

    GFIFO_Read_At_Raw(f, (gfifo_idx_t)(f->tail + (gfifo_idx_t)GFIFO_MSG_HDR),
                      dst, body);

    GFIFO_BARRIER();
    f->tail = (gfifo_idx_t)(f->tail + (gfifo_idx_t)(body + GFIFO_MSG_HDR));
    GFIFO_BARRIER();

    GFIFO_Note_Out_Raw(f, body + GFIFO_MSG_HDR);

    GFIFO_Unlock(f);

    if (out_len != NULL) {
        *out_len = body;
    }

    return GFIFO_OK;
#else
    GFIFO_UNUSED(f);
    GFIFO_UNUSED(dst);
    GFIFO_UNUSED(cap);
    GFIFO_UNUSED(out_len);
    return GFIFO_ERR_PARAM;
#endif
}

/**
 * @brief 查看队首消息但不移除，返回消息体在缓冲区内的直接指针
 * @param f         句柄指针，不可为 NULL
 * @param ptr       输出参数，返回消息体起始地址（已跳过长度头），不可为 NULL
 * @param first_len 输出参数，返回第一段连续字节数，不可为 NULL
 * @param total_len 输出参数，返回整条消息长度，不可为 NULL
 * @retval GFIFO_OK         读取成功
 * @retval GFIFO_ERR_PARAM  任一指针为 NULL，或本功能被裁剪
 * @retval GFIFO_ERR_NOINIT 句柄未初始化
 * @retval GFIFO_ERR_EMPTY  FIFO 里没有消息
 * @retval GFIFO_ERR_CORRUPT 长度头损坏
 * @note *first_len == *total_len 时整条消息在内存里连续，可以直接把 *ptr 交给
 *       解析器或 DMA，实现真正的零拷贝。
 * @note *first_len < *total_len 时消息跨越了缓冲区末尾：可以先处理第一段，
 *       再用 GFIFO_Msg_Get 把整条拷出来，或者分两段各自处理。
 */
gfifo_err_t GFIFO_Msg_Peek(const gfifo_t *f, const void **ptr,
                           uint32_t *first_len, uint32_t *total_len)
{
#if GFIFO_CFG_ENABLE_MSG
    uint32_t    body = 0u;
    gfifo_err_t ret;
    gfifo_idx_t bpos;
    uint32_t    cont;

    if (ptr == NULL || first_len == NULL || total_len == NULL) {
        return GFIFO_ERR_PARAM;
    }
    *ptr       = NULL;
    *first_len = 0u;
    *total_len = 0u;

    if (!GFIFO_Is_Valid(f)) {
        return (f == NULL) ? GFIFO_ERR_PARAM : GFIFO_ERR_NOINIT;
    }

    ret = GFIFO_Msg_Head_Len_Raw(f, &body);
    if (ret != GFIFO_OK) {
        return ret;
    }

    bpos = GFIFO_Pos(f, (gfifo_idx_t)(f->tail + (gfifo_idx_t)GFIFO_MSG_HDR));
    cont = (uint32_t)(f->size - bpos);
    if (cont > body) {
        cont = body;
    }

    *ptr       = (const void *)&f->buf[bpos];
    *first_len = cont;
    *total_len = body;

    return GFIFO_OK;
#else
    GFIFO_UNUSED(f);
    GFIFO_UNUSED(ptr);
    GFIFO_UNUSED(first_len);
    GFIFO_UNUSED(total_len);
    return GFIFO_ERR_PARAM;
#endif
}

/**
 * @brief 丢弃队首消息，不拷贝内容
 * @param f 句柄指针，不可为 NULL
 * @retval GFIFO_OK         丢弃成功
 * @retval GFIFO_ERR_PARAM  f 为 NULL，或本功能被裁剪
 * @retval GFIFO_ERR_NOINIT 句柄未初始化
 * @retval GFIFO_ERR_EMPTY  FIFO 里没有消息
 * @retval GFIFO_ERR_CORRUPT 长度头损坏，已清空队列以自愈
 */
gfifo_err_t GFIFO_Msg_Drop(gfifo_t *f)
{
#if GFIFO_CFG_ENABLE_MSG
    gfifo_err_t ret;

    if (!GFIFO_Is_Valid(f)) {
        return (f == NULL) ? GFIFO_ERR_PARAM : GFIFO_ERR_NOINIT;
    }

    GFIFO_Lock(f);
    ret = GFIFO_Msg_Discard_Raw(f);
    GFIFO_Unlock(f);

    return ret;
#else
    GFIFO_UNUSED(f);
    return GFIFO_ERR_PARAM;
#endif
}

/**
 * @brief 查询队首消息的长度，不移动读指针
 * @param f   句柄指针，不可为 NULL
 * @param len 输出参数，返回消息体长度（不含长度头），不可为 NULL
 * @retval GFIFO_OK         查询成功
 * @retval GFIFO_ERR_PARAM  f 或 len 为 NULL，或本功能被裁剪
 * @retval GFIFO_ERR_NOINIT 句柄未初始化
 * @retval GFIFO_ERR_EMPTY  FIFO 里没有消息
 * @retval GFIFO_ERR_CORRUPT 长度头损坏
 * @note 先查长度再决定开多大的栈缓冲区，可以避免"缓冲区不够又不想动态分配"
 *       的尴尬，配合 GFIFO_Msg_Get 使用。
 */
gfifo_err_t GFIFO_Msg_Len(const gfifo_t *f, uint32_t *len)
{
#if GFIFO_CFG_ENABLE_MSG
    uint32_t    body = 0u;
    gfifo_err_t ret;

    if (len == NULL) {
        return GFIFO_ERR_PARAM;
    }
    *len = 0u;

    if (!GFIFO_Is_Valid(f)) {
        return (f == NULL) ? GFIFO_ERR_PARAM : GFIFO_ERR_NOINIT;
    }

    ret = GFIFO_Msg_Head_Len_Raw(f, &body);
    if (ret == GFIFO_OK) {
        *len = body;
    }

    return ret;
#else
    GFIFO_UNUSED(f);
    GFIFO_UNUSED(len);
    return GFIFO_ERR_PARAM;
#endif
}

/*==============================================================================
 * 统计接口
 *============================================================================*/

/**
 * @brief 复位全部统计数据，不影响 FIFO 里的实际数据
 * @param f 句柄指针，不可为 NULL
 * @retval 无
 */
void GFIFO_Stat_Reset(gfifo_t *f)
{
#if GFIFO_CFG_ENABLE_STATS
    if (!GFIFO_Is_Valid(f)) {
        return;
    }

    f->stat_in   = 0u;
    f->stat_out  = 0u;
    f->stat_peak = 0u;
    f->stat_ovf  = 0u;
    f->stat_unf  = 0u;
#else
    GFIFO_UNUSED(f);
#endif
}

/**
 * @brief 查询累计写入字节数，包含因覆盖模式被丢弃的部分
 * @param f 句柄指针，可为 NULL
 * @retval 累计写入字节数；统计被裁剪或句柄无效时返回 0
 */
uint32_t GFIFO_Stat_In(const gfifo_t *f)
{
#if GFIFO_CFG_ENABLE_STATS
    if (!GFIFO_Is_Valid(f)) {
        return 0u;
    }
    return f->stat_in;
#else
    GFIFO_UNUSED(f);
    return 0u;
#endif
}

/**
 * @brief 查询累计读出字节数，包含被 Drop / Skip 丢弃的部分
 * @param f 句柄指针，可为 NULL
 * @retval 累计读出字节数；统计被裁剪或句柄无效时返回 0
 */
uint32_t GFIFO_Stat_Out(const gfifo_t *f)
{
#if GFIFO_CFG_ENABLE_STATS
    if (!GFIFO_Is_Valid(f)) {
        return 0u;
    }
    return f->stat_out;
#else
    GFIFO_UNUSED(f);
    return 0u;
#endif
}

/**
 * @brief 查询历史峰值占用字节数
 * @param f 句柄指针，可为 NULL
 * @retval 峰值字节数；统计被裁剪或句柄无效时返回 0
 */
uint32_t GFIFO_Stat_Peak(const gfifo_t *f)
{
#if GFIFO_CFG_ENABLE_STATS
    if (!GFIFO_Is_Valid(f)) {
        return 0u;
    }
    return f->stat_peak;
#else
    GFIFO_UNUSED(f);
    return 0u;
#endif
}

/**
 * @brief 查询因空间不足而丢弃数据的次数
 * @param f 句柄指针，可为 NULL
 * @retval 溢出次数；统计被裁剪或句柄无效时返回 0
 */
uint32_t GFIFO_Stat_Overflow(const gfifo_t *f)
{
#if GFIFO_CFG_ENABLE_STATS
    if (!GFIFO_Is_Valid(f)) {
        return 0u;
    }
    return f->stat_ovf;
#else
    GFIFO_UNUSED(f);
    return 0u;
#endif
}

/**
 * @brief 查询因无数据可读而失败的次数
 * @param f 句柄指针，可为 NULL
 * @retval 下溢次数；统计被裁剪或句柄无效时返回 0
 */
uint32_t GFIFO_Stat_Underflow(const gfifo_t *f)
{
#if GFIFO_CFG_ENABLE_STATS
    if (!GFIFO_Is_Valid(f)) {
        return 0u;
    }
    return f->stat_unf;
#else
    GFIFO_UNUSED(f);
    return 0u;
#endif
}

/**
 * @brief 查询统计功能是否被编入
 * @retval 1 统计可用，GFIFO_Stat_xxx 返回真实值
 * @retval 0 统计被裁剪，GFIFO_Stat_xxx 恒返回 0
 */
uint8_t GFIFO_Stat_Enabled(void)
{
#if GFIFO_CFG_ENABLE_STATS
    return 1u;
#else
    return 0u;
#endif
}

/*==============================================================================
 * 诊断接口
 *============================================================================*/

/**
 * @brief 自检句柄与内部计数的一致性
 * @param f 句柄指针，可为 NULL
 * @retval GFIFO_OK          一切正常
 * @retval GFIFO_ERR_PARAM   f 为 NULL
 * @retval GFIFO_ERR_NOINIT  魔数不对，句柄没初始化或被越界写踩了
 * @retval GFIFO_ERR_CORRUPT 占用字节数超过容量、元素边界错位、预留量异常
 * @note 复杂度 O(1)，不遍历缓冲区内容，可以在主循环里周期性调用。
 * @note 四条判据分别对应四类真实故障：
 *       1) used > size —— 有指针越界写了 head 或 tail；
 *       2) used % elem_size != 0 —— 有人绕过 API 直接按字节改了计数器；
 *       3) size % elem_size != 0 —— Init 之后有人改了 elem_size 字段；
 *       4) rsv > size —— 预约后既没提交也没放弃，且计数器被踩。
 */
gfifo_err_t GFIFO_Check(const gfifo_t *f)
{
    uint32_t used;
    uint32_t elem;

    if (f == NULL) {
        return GFIFO_ERR_PARAM;
    }
    if (f->magic != (uint16_t)GFIFO_MAGIC) {
        return GFIFO_ERR_NOINIT;
    }
    if (f->buf == NULL || f->size == 0u || f->elem_size == 0u) {
        return GFIFO_ERR_CORRUPT;
    }

    elem = (uint32_t)f->elem_size;
    used = GFIFO_Used_Raw(f);

    if (used > (uint32_t)f->size) {
        return GFIFO_ERR_CORRUPT;
    }
    if ((used % elem) != 0u) {
        return GFIFO_ERR_CORRUPT;
    }
    if (((uint32_t)f->size % elem) != 0u) {
        return GFIFO_ERR_CORRUPT;
    }
    if ((uint32_t)f->rsv > (uint32_t)f->size) {
        return GFIFO_ERR_CORRUPT;
    }

    return GFIFO_OK;
}
