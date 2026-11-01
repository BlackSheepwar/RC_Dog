/**
 * @file gfifo.h
 * @brief GFIFO 通用环形缓冲库：字节流 / 定长结构体 / 变长消息三种用法共用一套内核
 * @author Ricardo
 * @date 2026-10-31
 *
 * @note 设计目标：纯软件、零动态内存、零外部依赖、可在任意 C99 工具链上编译。
 *       全部状态都在调用方提供的 gfifo_t 句柄里，库自身没有任何全局变量，
 *       因此天然可重入、可多实例、可在中断与任务之间安全共享。
 *
 * @note 三种用法：
 *       - 纯数据 FIFO：elem_size = 1，用 GFIFO_Write / GFIFO_Read 搬字节流。
 *       - 结构体 FIFO：elem_size = sizeof(结构体)，用 GFIFO_Put / GFIFO_Get
 *         整块进整块出，绝不会出现"半个结构体"的撕裂状态。
 *       - 变长消息 FIFO：elem_size = 1，用 GFIFO_Msg_xxx 收发带长度头的报文。
 *
 * @note 并发模型（务必先读这一段再动手）：
 *       - 默认假设 SPSC：一个生产者（通常是 ISR 或某个任务）、一个消费者。
 *         这种用法下库完全无锁，不关中断，不阻塞，生产者与消费者互不等待。
 *       - 多生产者或多消费者（MPMC）：必须把 GFIFO_CFG_ENABLE_CRITICAL 设为 1，
 *         在 gfifo_port.h 里实现 GFIFO_ENTER_CRITICAL / GFIFO_EXIT_CRITICAL，
 *         并在 Init 时给 flags 加上 GFIFO_FLAG_CRITICAL。三者缺一不可，
 *         少了任何一条都会在压力测试里丢数据，且现象随机、极难复现。
 *
 * @note 不支持动态内存：缓冲区由调用方提供（全局数组 / 静态数组 / 内存池），
 *       库内没有 malloc，也没有隐式的大栈对象。这是嵌入式场合的硬要求。
 */

#ifndef __GFIFO_H__
#define __GFIFO_H__

/*==============================================================================
 * 头文件包含
 *============================================================================*/
/* 固定包含 */
#include <stdint.h>
#include <stddef.h>
#include "gfifo_cfg.h"
#include "gfifo_port.h"

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
 * 版本与常量
 *============================================================================*/

/** @brief 主版本号，接口不兼容时递增 */
#define GFIFO_VERSION_MAJOR 1

/** @brief 次版本号，向后兼容地新增接口时递增 */
#define GFIFO_VERSION_MINOR 0

/** @brief 修订号，只修 bug 不改接口时递增 */
#define GFIFO_VERSION_PATCH 0

/** @brief 版本字符串，便于运行时打印或写入固件版本页 */
#define GFIFO_VERSION_STRING "1.0.0"

/**
 * @brief 句柄魔数，用于识别"传进来一块没初始化过的内存"
 *
 * @note 取 'G' 'F' 两个字符，调试器里按小端看到的是 46 47，肉眼可辨。
 *       它不是安全机制，只是把"忘记 Init"从随机崩溃变成确定的错误码。
 */
#define GFIFO_MAGIC 0x4746u

/*==============================================================================
 * 标志位
 *============================================================================*/

/** @brief 无特殊行为，最常用的取值 */
#define GFIFO_FLAG_NONE 0x0000u

/**
 * @brief 覆盖模式：空间不足时丢弃最旧的数据，而不是拒绝写入
 *
 * @note 适用场景：传感器最新值、日志快照、UI 状态刷新——旧数据没有保留价值，
 *       宁可丢旧的也要保证新数据进得来。串口 DMA 接收配合它，可以做到
 *       "上层处理慢一点也不会把后续数据卡死"。
 *
 * @note 不适用场景：控制命令、计费数据、任何"丢一条就错"的场合。
 *
 * @note 元素模式（elem_size > 1）下，丢弃量会向上对齐到整元素，不会出现
 *       "半个结构体"残留。
 */
#define GFIFO_FLAG_OVERWRITE 0x0001u

/**
 * @brief 并发模式：每次操作都进入临界区，支持多生产者 / 多消费者
 *
 * @note 只有同时满足下面三条才真正生效，缺一即为无效标志：
 *       1) gfifo_cfg.h 里 GFIFO_CFG_ENABLE_CRITICAL = 1；
 *       2) gfifo_port.h 里实现了 GFIFO_ENTER_CRITICAL / GFIFO_EXIT_CRITICAL；
 *       3) Init 时传入本标志。
 *
 * @note SPSC 场景不要加这个标志：关中断会拉高最坏响应时间，而 SPSC 本来就
 *       不需要锁，加了纯属自伤。
 */
#define GFIFO_FLAG_CRITICAL 0x0002u

/**
 * @brief 初始化时不清零缓冲区，覆盖 GFIFO_CFG_CLEAR_ON_INIT 的全局设置
 *
 * @note 只有在大缓冲区（几十 KiB 以上）且启动时间敏感时才用。清零的作用是
 *       让零拷贝接口在空 FIFO 上不会返回上一轮的残留数据。
 */
#define GFIFO_FLAG_NO_CLEAR 0x0004u

/**
 * @brief 调用方可传入的全部合法标志，其余位被库内部占用
 */
#define GFIFO_FLAG_MASK_USER \
    (GFIFO_FLAG_OVERWRITE | GFIFO_FLAG_CRITICAL | GFIFO_FLAG_NO_CLEAR)

/**
 * @brief 内部标志：容量恰好是 2 的幂，索引换算走掩码快路径
 *
 * @note 由 GFIFO_Init 自动设置，调用方不要手工置位。列在这里是为了让调试器
 *       里看到的 flags 值可以被解读，不是给人用的接口。
 */
#define GFIFO_FLAG_POW2 0x0100u

/*==============================================================================
 * 类型定义
 *============================================================================*/

/**
 * @brief 环形缓冲的索引与计数器类型，由 GFIFO_CFG_INDEX_TYPE 决定
 *
 * @note 语义上它是"从通电到现在累计写入/读出的字节数"，会自然回绕。库内所有
 *       位置计算都基于无符号减法 (head - tail)，回绕自动正确，不需要特殊处理。
 *       这也是本库能做到"容量任意、不需要预留一个空位判满"的原因。
 */
typedef GFIFO_CFG_INDEX_TYPE gfifo_idx_t;

/**
 * @brief 索引类型的最大可表示值，即单个 FIFO 的容量上限
 */
#define GFIFO_IDX_MAX ((gfifo_idx_t)~(gfifo_idx_t)0)

/**
 * @brief 库的返回码，全部为 0 或负值，可直接用 if (ret < 0) 判断失败
 *
 * @note 用枚举而不是宏，是为了让调试器能把变量显示成名字而不是数字。
 *       底层类型是 int，与规范里"0 成功、负值失败"的约定一致。
 */
typedef enum {
    GFIFO_OK           = 0,  /* 操作成功 */
    GFIFO_ERR_PARAM    = -1, /* 入参非法：空指针、零长度、非法标志、未对齐 */
    GFIFO_ERR_NOINIT   = -2, /* 句柄没有初始化，或魔数被踩 */
    GFIFO_ERR_FULL     = -3, /* 空间不足，一个完整元素都放不下 */
    GFIFO_ERR_EMPTY    = -4, /* 没有数据可读 */
    GFIFO_ERR_TOOSMALL = -5, /* 目标缓冲区比要读的消息还小 */
    GFIFO_ERR_TOOBIG   = -6, /* 请求长度超过 FIFO 总容量，永远不可能满足 */
    GFIFO_ERR_STATE    = -7, /* 状态机错乱：未 Reserve 就 Commit，或重复 Reserve */
    GFIFO_ERR_CORRUPT  = -8  /* 自检发现内部数据不一致，句柄已被破坏 */
} gfifo_err_t;

/**
 * @brief 预留写入返回的两段连续空间描述
 *
 * @note 环形缓冲的空闲区可能被"缓冲区末尾"切成两段。l1 是紧跟当前位置的
 *       第一段，l2 是回绕到缓冲区开头后的第二段，两段之和等于请求的 len。
 *
 * @note l2 为 0 时 p2 恒为 NULL，调用方不必判断 p2 是否有效。
 *
 * @note 两段必须"先写满 p1 再写 p2"，否则数据在逻辑上不连续。DMA 场景下
 *       若硬件不支持链式传输，请只使用 l1，或把请求长度限制在 l1 以内。
 */
typedef struct {
    uint8_t *p1; /* 第一段起始地址，l1 > 0 时有效 */
    uint32_t l1; /* 第一段可写字节数 */
    uint8_t *p2; /* 第二段起始地址（缓冲区开头），l2 > 0 时有效 */
    uint32_t l2; /* 第二段可写字节数 */
} gfifo_span_t;

/**
 * @brief FIFO 句柄，由调用方静态分配，布局由库维护
 *
 * @note 为什么完整暴露布局：调用方必须能写 static gfifo_t s_uart_fifo;
 *       才能做到零动态分配。C 语言里"不透明类型"要求实例只能由库内部创建，
 *       与本库"调用方提供内存"的设计冲突，所以这里选择暴露。
 *
 * @note 但请把它当不透明类型用：只声明、只取地址、只传给 GFIFO_xxx()。
 *       直接读 head/tail 只会读到瞬时快照，直接改任何一个字段都会让
 *       head/tail 的一致性假设失效，症状是数据错乱且无法定位。
 *
 * @note 生命周期：Init 之后才能调用其它接口；Reset 只清数据不破坏配置；
 *       句柄本身可以被 memcpy 到别处（内部无自指指针），但两个副本会各自
 *       独立演进，这通常不是你想要的。
 */
typedef struct gfifo {
    uint8_t             *buf;       /* 缓冲区基址，由调用方提供并保证生命周期 */
    gfifo_idx_t          size;      /* 实际可用字节数，已对齐到 elem_size 的整数倍 */
    gfifo_idx_t          mask;      /* size - 1，仅在 GFIFO_FLAG_POW2 置位时有效 */
    volatile gfifo_idx_t head;      /* 累计写入字节数，只由生产者修改 */
    volatile gfifo_idx_t tail;      /* 累计读出字节数，只由消费者修改 */
    volatile gfifo_idx_t rsv;       /* 已预留但未提交的字节数，仅 Reserve 模式使用 */
#if GFIFO_CFG_ENABLE_STATS
    uint32_t             stat_in;   /* 累计写入字节数（含被覆盖丢弃的部分） */
    uint32_t             stat_out;  /* 累计读出字节数（含 Drop 丢弃的部分） */
    uint32_t             stat_peak; /* 历史峰值占用字节数，用于定容量 */
    uint32_t             stat_ovf;  /* 因空间不足丢弃数据的次数 */
    uint32_t             stat_unf;  /* 因无数据可读而失败的次数 */
#endif
    uint16_t             elem_size; /* 元素大小，1 表示纯字节流 */
    uint16_t             flags;     /* GFIFO_FLAG_xxx 的组合，含库内部标志 */
    uint16_t             magic;     /* 恒为 GFIFO_MAGIC，用于识别未初始化句柄 */
} gfifo_t;

/* 索引类型必须是 16/32/64 位的无符号整型，否则无符号减法回绕的语义不成立 */
GFIFO_STATIC_ASSERT((sizeof(gfifo_idx_t) == 2u) ||
                    (sizeof(gfifo_idx_t) == 4u) ||
                    (sizeof(gfifo_idx_t) == 8u),
                    "gfifo idx type must be 16/32/64-bit");

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
 *                            flags 含未定义的位；对齐后可用容量为 0
 * @retval GFIFO_ERR_TOOBIG   size 超过索引类型上限（uint16_t 索引时为 65535）
 * @note 本函数不是线程安全的，必须在任何生产者/消费者启动之前调用。
 * @note size 不是 elem_size 整数倍时会被静默向下取整（例如 100 字节配 24 字节
 *       的元素，实际容量 96）。取整后的真实容量可用 GFIFO_Capacity() 查。
 * @note 结构体 FIFO 的 buf 必须按元素的对齐要求对齐，推荐一律用 GFIFO_ALIGN(8)
 *       修饰存储数组，否则在 Cortex-M0 上可能因非对齐访问触发 HardFault。
 */
gfifo_err_t GFIFO_Init(gfifo_t *f, void *buf, uint32_t size,
                       uint32_t elem_size, uint16_t flags);

/**
 * @brief 清空 FIFO 里的全部数据，保留容量、元素大小、标志与统计配置
 * @param f 句柄指针，不可为 NULL
 * @retval GFIFO_OK         清空成功
 * @retval GFIFO_ERR_PARAM  f 为 NULL
 * @retval GFIFO_ERR_NOINIT 句柄未初始化
 * @note 会一并复位未提交的预留区，因此 Reserve 之后忘记 Commit 的悬挂状态
 *       可以用 Reset 兜底恢复。
 * @note 统计计数（stat_in / stat_out / stat_peak）不做清零，需要清零请显式调
 *       GFIFO_Stat_Reset()。这样"跑了一整天的总量"不会因为一次清队列而丢失。
 * @note 多生产者/多消费者场景下调用本函数必须自己保证没有并发访问。
 */
gfifo_err_t GFIFO_Reset(gfifo_t *f);

/**
 * @brief 查询 FIFO 的实际可用字节容量（已按 elem_size 对齐后的值）
 * @param f 句柄指针，可为 NULL
 * @retval 可用字节数；f 为 NULL 或未初始化时返回 0
 * @note 与 Init 传入的 size 可能不同：不是 elem_size 整数倍时会被向下取整。
 */
uint32_t GFIFO_Capacity(const gfifo_t *f);

/**
 * @brief 查询单个元素的字节大小
 * @param f 句柄指针，可为 NULL
 * @retval 元素字节数；f 为 NULL 或未初始化时返回 0
 * @retval 1 表示这是一条纯字节流 FIFO
 */
uint32_t GFIFO_Elem_Size(const gfifo_t *f);

/**
 * @brief 查询最多能容纳多少个元素
 * @param f 句柄指针，可为 NULL
 * @retval 元素个数上限；f 为 NULL 或未初始化时返回 0
 * @note 等于 GFIFO_Capacity() / GFIFO_Elem_Size()，可直接用来定数组大小。
 */
uint32_t GFIFO_Elem_Capacity(const gfifo_t *f);

/*==============================================================================
 * 状态查询
 *============================================================================*/

/**
 * @brief 查询已占用的字节数
 * @param f 句柄指针，可为 NULL
 * @retval 已用字节数，范围 [0, GFIFO_Capacity()]；f 为 NULL 或未初始化时返回 0
 * @note 读取的是瞬时快照。SPSC 场景下返回值对消费者是精确的，对生产者可能
 *       已经过时（消费者可能刚取走一批），用于流控判断时请留余量。
 */
uint32_t GFIFO_Used(const gfifo_t *f);

/**
 * @brief 查询还能写入的字节数
 * @param f 句柄指针，可为 NULL
 * @retval 剩余可写字节数，已扣除未提交的预留区；f 为 NULL 或未初始化时返回 0
 */
uint32_t GFIFO_Free(const gfifo_t *f);

/**
 * @brief 查询已存放的完整元素个数
 * @param f 句柄指针，可为 NULL
 * @retval 元素个数；f 为 NULL 或未初始化时返回 0
 * @note 字节流 FIFO（elem_size == 1）下等价于 GFIFO_Used()。
 */
uint32_t GFIFO_Count(const gfifo_t *f);

/**
 * @brief 查询还能再放入多少个完整元素
 * @param f 句柄指针，可为 NULL
 * @retval 可写元素个数；f 为 NULL 或未初始化时返回 0
 */
uint32_t GFIFO_Space(const gfifo_t *f);

/**
 * @brief 判断 FIFO 是否为空
 * @param f 句柄指针，可为 NULL
 * @retval 1 空（没有任何可读数据）
 * @retval 0 非空
 * @note f 为 NULL 或未初始化时按"空"处理，便于调用方写成 if (!GFIFO_Is_Empty(f)) 而不必先判空指针。
 */
uint8_t GFIFO_Is_Empty(const gfifo_t *f);

/**
 * @brief 判断 FIFO 是否已满（连一个元素都放不下）
 * @param f 句柄指针，可为 NULL
 * @retval 1 满
 * @retval 0 未满
 * @note f 为 NULL 或未初始化时按"满"处理：这种句柄写不进去任何东西，
 *       返回 1 能让"满了就等待"的调用方自然停在等待分支上，而不是空转。
 */
uint8_t GFIFO_Is_Full(const gfifo_t *f);

/*==============================================================================
 * 字节流接口
 *============================================================================*/

/**
 * @brief 写入一段字节，空间不足时按"能写多少写多少"处理
 * @param f   句柄指针，不可为 NULL
 * @param src 源数据地址，不可为 NULL
 * @param len 请求写入的字节数，必须大于 0
 * @retval >0 实际写入的字节数，可能小于 len（非覆盖模式且空间不足时）
 * @retval 0  一个字节都没写：参数非法、句柄未初始化、FIFO 已满
 * @note 非覆盖模式是"部分写入"，调用方必须检查返回值，不能假设全写进去了。
 *       这是流式接口的固有语义：宁可写一半也不阻塞，实时性优先。
 * @note 覆盖模式（GFIFO_FLAG_OVERWRITE）下会先丢弃最旧的数据腾出空间，
 *       返回值恒等于 len（除非 len 超过总容量，此时被截断到容量）。
 * @note 元素模式下 len 会被向下对齐到 elem_size 的整数倍，避免写出半个元素。
 * @note 是线程安全的：SPSC 下无锁，MPMC 且置了 GFIFO_FLAG_CRITICAL 时走临界区。
 */
uint32_t GFIFO_Write(gfifo_t *f, const void *src, uint32_t len);

/**
 * @brief 读出一段字节，数据不足时按"有多少读多少"处理
 * @param f   句柄指针，不可为 NULL
 * @param dst 目标缓冲区地址，不可为 NULL
 * @param len 请求读取的字节数，必须大于 0
 * @retval >0 实际读出的字节数，可能小于 len（FIFO 里不够时）
 * @retval 0  一个字节都没读：参数非法、句柄未初始化、FIFO 已空
 * @note 读出的数据会从 FIFO 中移除，等价于 "Peek_Span + Skip" 的拷贝版。
 * @note 元素模式下 len 会被向下对齐到 elem_size 的整数倍。
 */
uint32_t GFIFO_Read(gfifo_t *f, void *dst, uint32_t len);

/**
 * @brief 写入单个字节
 * @param f    句柄指针，不可为 NULL
 * @param byte 待写入的字节
 * @retval GFIFO_OK         写入成功
 * @retval GFIFO_ERR_PARAM  f 为 NULL
 * @retval GFIFO_ERR_NOINIT 句柄未初始化
 * @retval GFIFO_ERR_FULL   空间不足，未写入
 * @note 覆盖模式下不会返回 FULL，而是丢掉最旧的一个元素后写入。
 * @note 对元素大小大于 1 的 FIFO 调用本函数时，写入的是"一个元素大小的、
 *       内容为一个字节加若干零"的数据，几乎肯定不是你想要的；请改调 GFIFO_Put()。
 */
gfifo_err_t GFIFO_Write_Byte(gfifo_t *f, uint8_t byte);

/**
 * @brief 读出单个字节
 * @param f    句柄指针，不可为 NULL
 * @param byte 输出参数，读出的字节写到这里，不可为 NULL
 * @retval GFIFO_OK         读取成功
 * @retval GFIFO_ERR_PARAM  f 或 byte 为 NULL
 * @retval GFIFO_ERR_NOINIT 句柄未初始化
 * @retval GFIFO_ERR_EMPTY  FIFO 为空，byte 未被修改
 */
gfifo_err_t GFIFO_Read_Byte(gfifo_t *f, uint8_t *byte);

/*==============================================================================
 * 定长元素接口
 *============================================================================*/

/**
 * @brief 放入一个完整元素（结构体 FIFO 的主力接口）
 * @param f    句柄指针，不可为 NULL
 * @param elem 指向待写入元素的指针，不可为 NULL，至少可读 elem_size 字节
 * @retval GFIFO_OK         写入成功
 * @retval GFIFO_ERR_PARAM  f 或 elem 为 NULL
 * @retval GFIFO_ERR_NOINIT 句柄未初始化
 * @retval GFIFO_ERR_FULL   剩余空间不足一个元素，未写入
 * @note 与 GFIFO_Write 的区别：本函数要么完整写入一个元素，要么一个字节都不写，
 *       绝不会产生"半个结构体"。元素 FIFO 必须用本函数，不能用 Write 拼。
 * @note 覆盖模式下空间不足时会丢弃最旧的一个元素后写入，仍保证元素完整。
 * @note 中断里可以安全调用，不会阻塞、不关中断（SPSC 场景下）。
 */
gfifo_err_t GFIFO_Put(gfifo_t *f, const void *elem);

/**
 * @brief 取出一个完整元素，取出后该元素从 FIFO 中移除
 * @param f    句柄指针，不可为 NULL
 * @param elem 输出参数，至少可写 elem_size 字节，不可为 NULL
 * @retval GFIFO_OK         读取成功
 * @retval GFIFO_ERR_PARAM  f 或 elem 为 NULL
 * @retval GFIFO_ERR_NOINIT 句柄未初始化
 * @retval GFIFO_ERR_EMPTY  FIFO 为空，elem 未被修改
 * @note 元素模式下的"读"天然是原子的：要么拿到完整的一份，要么什么都没拿到。
 *       这就是结构体 FIFO 相对字节流的全部价值。
 */
gfifo_err_t GFIFO_Get(gfifo_t *f, void *elem);

/**
 * @brief 查看最旧的元素但不移除（偷看）
 * @param f    句柄指针，不可为 NULL
 * @param elem 输出参数，至少可写 elem_size 字节，不可为 NULL
 * @retval GFIFO_OK         读取成功
 * @retval GFIFO_ERR_PARAM  f 或 elem 为 NULL
 * @retval GFIFO_ERR_NOINIT 句柄未初始化
 * @retval GFIFO_ERR_EMPTY  FIFO 为空，elem 未被修改
 * @note 典型用法："先看看队首是什么命令，决定要不要处理，处理完再 Get 掉"。
 * @note 多消费者场景下 Peek 与随后的 Get 之间存在竞争窗口，此时应改用
 *       GFIFO_Get 一次性拿走，或自行在外层加锁。
 */
gfifo_err_t GFIFO_Peek(gfifo_t *f, void *elem);

/**
 * @brief 查看指定位置的元素但不移除
 * @param f     句柄指针，不可为 NULL
 * @param index 相对最旧元素的偏移，0 表示队首，必须小于 GFIFO_Count()
 * @param elem  输出参数，至少可写 elem_size 字节，不可为 NULL
 * @retval GFIFO_OK         读取成功
 * @retval GFIFO_ERR_PARAM  f 或 elem 为 NULL
 * @retval GFIFO_ERR_NOINIT 句柄未初始化
 * @retval GFIFO_ERR_EMPTY  index 越界或 FIFO 为空，elem 未被修改
 * @note 时间复杂度 O(1)：直接由 (tail + index * elem_size) 换算，不需要遍历。
 * @note 适合"按序号回看最近 N 条记录"，例如故障前后的最后 8 条传感器数据。
 */
gfifo_err_t GFIFO_Peek_At(gfifo_t *f, uint32_t index, void *elem);

/**
 * @brief 丢弃最旧的若干个元素
 * @param f     句柄指针，不可为 NULL
 * @param count 要丢弃的元素个数，超过已有数量时按实际数量丢弃
 * @retval GFIFO_OK         丢弃完成
 * @retval GFIFO_ERR_PARAM  f 为 NULL
 * @retval GFIFO_ERR_NOINIT 句柄未初始化
 * @note 丢弃是"读而不取内容"，会计入 stat_out（视为已消费），但不触发下溢计数。
 * @note 典型用法：消费者发现自己处理不过来，主动丢几条旧数据降低延迟。
 */
gfifo_err_t GFIFO_Drop(gfifo_t *f, uint32_t count);

/*==============================================================================
 * 零拷贝接口
 *============================================================================*/

/**
 * @brief 取得队首之后的一段连续可读区域，不做任何拷贝
 * @param f   句柄指针，不可为 NULL
 * @param ptr 输出参数，返回指向 FIFO 内部缓冲的指针，不可为 NULL
 * @retval >0 第一段连续可读的字节数，*ptr 指向这段数据的起点
 * @retval 0  FIFO 为空或参数非法，*ptr 被置为 NULL
 * @note 返回的是内部缓冲区的指针，只在"下一次写操作"或"下一次 Skip"之前有效。
 *       不要保存它，不要 free 它，不要写它。
 * @note 环形缓冲的可读区可能被缓冲区末尾切成两段。本函数只返回第一段，
 *       调用方处理完第一段后应再次调用，直到返回 0，这样才能读完全部数据。
 * @note 与 GFIFO_Skip 配对使用：Peek_Span 拿指针 → 直接处理（比如原地解析、
 *       或把指针交给 DMA 发送）→ Skip 掉已处理的长度。
 * @note 仅在单消费者场景下成立。多消费者时两个消费者可能拿到同一段数据。
 */
uint32_t GFIFO_Peek_Span(const gfifo_t *f, const void **ptr);

/**
 * @brief 跳过（消费掉）指定长度的数据，不拷贝内容
 * @param f   句柄指针，不可为 NULL
 * @param len 要消费的**字节数**（不是元素个数），超过已有数据时按实际数量处理
 * @retval GFIFO_OK         处理完成
 * @retval GFIFO_ERR_PARAM  f 为 NULL
 * @retval GFIFO_ERR_NOINIT 句柄未初始化
 * @note 与 GFIFO_Peek_Span 严格配对。只 Skip 不 Peek 等于凭空丢数据，请确认
 *       你确实处理过那段内存。
 * @note 元素模式下 len 会被向下对齐到 elem_size 的整数倍，对齐不上的部分
 *       不会被跳过：元素 24 字节时，Skip(20) 一个都不跳（20/24 = 0），
 *       Skip(30) 也只跳 1 个。要按"个数"丢弃请用 GFIFO_Drop()。
 * @note 字节流（elem_size == 1）下 len 就是字节数，不存在对齐问题。
 */
gfifo_err_t GFIFO_Skip(gfifo_t *f, uint32_t len);

/**
 * @brief 预约一段写入空间，拿到可直接写的两段指针，稍后再提交
 * @param f    句柄指针，不可为 NULL
 * @param len  要预约的字节数，必须大于 0 且不超过当前空闲量
 * @param span 输出参数，返回两段可写区域的指针与长度，不可为 NULL
 * @retval GFIFO_OK         预约成功，调用方可以往 span 指向的内存写数据了
 * @retval GFIFO_ERR_PARAM  f 或 span 为 NULL，或 len 为 0
 * @retval GFIFO_ERR_NOINIT 句柄未初始化
 * @retval GFIFO_ERR_STATE  已经有一次未提交的预约，必须先 Commit 或 Abort
 * @retval GFIFO_ERR_FULL   空闲空间不足 len
 * @retval GFIFO_ERR_TOOBIG len 超过 FIFO 总容量，永远不可能满足
 * @note 这是"零拷贝写"的入口，典型用途有两个：
 *       1) DMA 直接写入：预约 → 启动 DMA 往 span.p1 灌 → DMA 完成中断里 Commit；
 *       2) 变长帧拼装：预约整帧长度 → 分别填头和数据 → 一次 Commit 生效。
 * @note 预约期间这段空间被"占位"但不对外可见：其他读者看到的仍是旧数据，
 *       不会读到写了一半的内容。这正是 rsv 字段存在的意义。
 * @note 同一时刻只允许一个未提交的预约。生产者也必须是单一线程/中断上下文，
 *       因为 rsv 只有一个，两个生产者同时预约会互相覆盖。
 * @note 预约后必须 Commit 或 Abort。忘记提交会让这段空间永久泄漏，直到 Reset。
 */
gfifo_err_t GFIFO_Write_Reserve(gfifo_t *f, uint32_t len, gfifo_span_t *span);

/**
 * @brief 提交此前预约的全部空间，让数据对消费者可见
 * @param f 句柄指针，不可为 NULL
 * @retval GFIFO_OK         提交成功，预约长度已并入 head
 * @retval GFIFO_ERR_PARAM  f 为 NULL
 * @retval GFIFO_ERR_NOINIT 句柄未初始化
 * @retval GFIFO_ERR_STATE  当前没有未提交的预约
 * @note 提交前会插入内存屏障，确保写入的数据先于 head 更新对消费者可见。
 *       这是无锁 SPSC 正确性的关键，不要自己绕过它直接改 head。
 */
gfifo_err_t GFIFO_Write_Commit(gfifo_t *f);

/**
 * @brief 放弃此前预约的空间，不写入任何数据，空间立即归还
 * @param f 句柄指针，不可为 NULL
 * @retval 无
 * @note 用于"DMA 传输失败""帧拼装过程中发现长度算错"这类异常路径。
 *       正常路径请用 Commit，否则缓冲区会一直空转不出数据。
 */
void GFIFO_Write_Abort(gfifo_t *f);

/*==============================================================================
 * 变长消息接口
 *============================================================================*/

/**
 * @brief 放入一条变长消息，自动附加长度头
 * @param f   句柄指针，不可为 NULL；元素大小必须为 1
 * @param msg 消息内容地址，不可为 NULL
 * @param len 消息长度，必须大于 0，且不超过 2^8 / 2^16 / 2^32（取决于长度头宽度）
 * @retval GFIFO_OK         写入成功
 * @retval GFIFO_ERR_PARAM  f 或 msg 为 NULL；len 为 0；len 超出长度头表示范围；
 *                          FIFO 的元素大小不是 1
 * @retval GFIFO_ERR_NOINIT 句柄未初始化
 * @retval GFIFO_ERR_FULL   剩余空间放不下"长度头 + 消息"，未写入
 * @retval GFIFO_ERR_TOOBIG len 加上长度头后超过 FIFO 总容量
 * @note 整条消息的写入是原子的：其他读者要么看到完整的一条，要么看不到，
 *       不会读到"头写进去了、数据还没写完"的中间态。
 * @note 长度头按主机字节序存放。FIFO 是同一颗 CPU 内部的通道，跨设备通信的
 *       字节序应由协议层负责，库不掺和。
 * @note 覆盖模式下空间不足会丢弃最旧的消息，但只能整条丢弃——若剩余空间
 *       仍放不下新消息，本次写入返回 FULL 而不做"丢一半"的危险操作。
 */
gfifo_err_t GFIFO_Msg_Put(gfifo_t *f, const void *msg, uint32_t len);

/**
 * @brief 取出一条变长消息，取出后该消息从 FIFO 中移除
 * @param f       句柄指针，不可为 NULL
 * @param dst     输出缓冲区，不可为 NULL
 * @param cap     dst 的可写字节数
 * @param out_len 输出参数，写入实际消息长度，可为 NULL
 * @retval GFIFO_OK           读取成功
 * @retval GFIFO_ERR_PARAM    f 或 dst 为 NULL
 * @retval GFIFO_ERR_NOINIT   句柄未初始化
 * @retval GFIFO_ERR_EMPTY    FIFO 里没有消息
 * @retval GFIFO_ERR_TOOSMALL cap 小于队首消息的长度，消息保持不动
 * @retval GFIFO_ERR_CORRUPT  长度头解析出的长度超过 FIFO 容量，数据已损坏
 * @note 返回 TOOSMALL 时消息仍在队列里，调用方可以换一个更大的缓冲区重试，
 *       也可以用 GFIFO_Msg_Drop 主动丢弃。库不会替你丢数据。
 */
gfifo_err_t GFIFO_Msg_Get(gfifo_t *f, void *dst, uint32_t cap, uint32_t *out_len);

/**
 * @brief 查看队首消息但不移除，返回消息体在缓冲区内的直接指针
 * @param f         句柄指针，不可为 NULL
 * @param ptr       输出参数，返回消息体的起始地址（跳过长度头），不可为 NULL
 * @param first_len 输出参数，返回第一段连续字节数，不可为 NULL
 * @param total_len 输出参数，返回整条消息长度，不可为 NULL
 * @retval GFIFO_OK         读取成功
 * @retval GFIFO_ERR_PARAM  任一指针为 NULL
 * @retval GFIFO_ERR_NOINIT 句柄未初始化
 * @retval GFIFO_ERR_EMPTY  FIFO 里没有消息
 * @retval GFIFO_ERR_CORRUPT 长度头解析出的长度超过 FIFO 容量，数据已损坏
 * @note 当 *first_len == *total_len 时，整条消息在内存里是连续的，可以直接
 *       把 *ptr 交给解析函数或 DMA，做到真正的零拷贝。
 * @note 当 *first_len < *total_len 时，消息跨越了缓冲区末尾，需要分两段处理，
 *       或者干脆改用 GFIFO_Msg_Get 拷出来。库不保证消息不跨段：为了不浪费
 *       尾部空间，没有做"放不下就整体回绕"的填充策略。
 * @note 返回值指针在下次写入或下次 Drop 之前有效。
 */
gfifo_err_t GFIFO_Msg_Peek(const gfifo_t *f, const void **ptr,
                           uint32_t *first_len, uint32_t *total_len);

/**
 * @brief 丢弃队首消息，不拷贝内容
 * @param f 句柄指针，不可为 NULL
 * @retval GFIFO_OK         丢弃成功
 * @retval GFIFO_ERR_PARAM  f 为 NULL
 * @retval GFIFO_ERR_NOINIT 句柄未初始化
 * @retval GFIFO_ERR_EMPTY  FIFO 里没有消息
 * @retval GFIFO_ERR_CORRUPT 长度头解析出的长度超过 FIFO 容量，数据已损坏
 * @note 用于"这条消息我不关心，直接跳过"的分支，比如过滤掉不认识的命令字。
 */
gfifo_err_t GFIFO_Msg_Drop(gfifo_t *f);

/**
 * @brief 查询队首消息的长度，不移动读指针
 * @param f   句柄指针，不可为 NULL
 * @param len 输出参数，返回消息体长度（不含长度头），不可为 NULL
 * @retval GFIFO_OK         查询成功
 * @retval GFIFO_ERR_PARAM  f 或 len 为 NULL
 * @retval GFIFO_ERR_NOINIT 句柄未初始化
 * @retval GFIFO_ERR_EMPTY  FIFO 里没有消息
 * @retval GFIFO_ERR_CORRUPT 长度头解析出的长度超过 FIFO 容量，数据已损坏
 * @note 先查长度再决定开多大的栈缓冲区，可以避免"缓冲区不够又不想动态分配"
 *       的尴尬。配合 GFIFO_Msg_Get 使用。
 */
gfifo_err_t GFIFO_Msg_Len(const gfifo_t *f, uint32_t *len);

/*==============================================================================
 * 统计接口
 *============================================================================*/

/**
 * @brief 复位全部统计数据，不影响 FIFO 里的实际数据
 * @param f 句柄指针，不可为 NULL
 * @retval 无
 * @note 典型用法：设备上电自检完成后调一次，让统计从"干净状态"开始计。
 */
void GFIFO_Stat_Reset(gfifo_t *f);

/**
 * @brief 查询累计写入字节数，包含因覆盖模式被丢弃的部分
 * @param f 句柄指针，可为 NULL
 * @retval 累计写入字节数；f 为 NULL 或未初始化时返回 0
 */
uint32_t GFIFO_Stat_In(const gfifo_t *f);

/**
 * @brief 查询累计读出字节数，包含被 Drop / Skip 丢弃的部分
 * @param f 句柄指针，可为 NULL
 * @retval 累计读出字节数；f 为 NULL 或未初始化时返回 0
 * @note 与 GFIFO_Stat_In 的差值就是"当前还在队列里 + 因溢出丢失"的量。
 */
uint32_t GFIFO_Stat_Out(const gfifo_t *f);

/**
 * @brief 查询历史峰值占用字节数
 * @param f 句柄指针，可为 NULL
 * @retval 峰值字节数；f 为 NULL 或未初始化时返回 0
 * @note 定缓冲区大小最有用的一个数：跑满业务峰值场景后看这个值，容量取它的
 *       1.3~1.5 倍即可，比拍脑袋估靠谱得多。
 */
uint32_t GFIFO_Stat_Peak(const gfifo_t *f);

/**
 * @brief 查询因空间不足而丢弃数据的次数
 * @param f 句柄指针，可为 NULL
 * @retval 溢出次数；f 为 NULL 或未初始化时返回 0
 * @note 非覆盖模式下"部分写入"也算一次溢出；覆盖模式下每次覆盖丢弃算一次。
 *       这个值长期大于 0 说明容量不够，或者消费者太慢。
 */
uint32_t GFIFO_Stat_Overflow(const gfifo_t *f);

/**
 * @brief 查询因无数据可读而失败的次数
 * @param f 句柄指针，可为 NULL
 * @retval 下溢次数；f 为 NULL 或未初始化时返回 0
 * @note 消费者空转的次数。偏大说明消费者跑得比生产者快，可以考虑改成事件驱动。
 */
uint32_t GFIFO_Stat_Underflow(const gfifo_t *f);

/**
 * @brief 查询统计功能是否被编入（供上层代码条件编译，避免到处都是 #if）
 * @retval 1 统计可用，GFIFO_Stat_xxx 返回真实值
 * @retval 0 统计被裁剪，GFIFO_Stat_xxx 恒返回 0
 */
uint8_t GFIFO_Stat_Enabled(void);

/*==============================================================================
 * 诊断接口
 *============================================================================*/

/**
 * @brief 自检句柄与内部计数的一致性，用于断言、上电自检、故障后取证
 * @param f 句柄指针，可为 NULL
 * @retval GFIFO_OK          一切正常
 * @retval GFIFO_ERR_PARAM   f 为 NULL
 * @retval GFIFO_ERR_NOINIT  魔数不对，句柄没初始化或被越界写踩了
 * @retval GFIFO_ERR_CORRUPT 占用字节数超过容量，或 head/tail 关系不成立
 * @note 时间复杂度 O(1)，不遍历缓冲区内容，可以在主循环里周期性调用。
 * @note 检测不出"数据内容被写坏"这类问题，只能发现计数器的结构性错误。
 *       缓冲区内容校验需要上层自己加 CRC。
 */
gfifo_err_t GFIFO_Check(const gfifo_t *f);

#ifdef __cplusplus
}
#endif

#endif /* __GFIFO_H__ */
