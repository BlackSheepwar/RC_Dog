/**
 * @file gfifo_port.h
 * @brief GFIFO 移植层：编译器特性、内存屏障、临界区、断言、通用工具宏
 * @author Ricardo
 * @date 2026-10-31
 *
 * @note 这是整个库唯一与"编译器 / CPU / 操作系统"沾边的地方。gfifo.c 与
 *       gfifo.h 里不出现任何编译器专有语法、不出现任何 CMSIS / HAL / RTOS
 *       头文件，全部经由本文件的宏间接表达。换平台时只改这一个文件。
 *
 * @note 移植工作分三档，按需选择：
 *       1) 什么都不做：单生产者单消费者（SPSC）场景下库本身无锁、无临界区、
 *          无阻塞，只要编译器认识 C99，直接能跑。
 *       2) 定义 GFIFO_ENTER_CRITICAL / GFIFO_EXIT_CRITICAL：支持多生产者或
 *          多消费者（MPMC），实现方式可以是关中断、RTOS 互斥量、自旋锁。
 *       3) 额外覆盖 GFIFO_BARRIER / GFIFO_ASSERT：多核 SMP 平台需要真内存
 *          屏障；带看门狗的产品需要断言时喂狗或复位。
 *
 * @note 本文件不做任何函数定义，全部是宏与 typedef，可以安全地被任意多个
 *       编译单元包含。
 */

#ifndef __GFIFO_PORT_H__
#define __GFIFO_PORT_H__

/*==============================================================================
 * 头文件包含
 *============================================================================*/
/* 固定包含 */
#include <stdint.h>
#include <stddef.h>
#include "gfifo_cfg.h"

/*==============================================================================
 * 编译器识别
 *============================================================================*/

/**
 * @brief 编译器家族识别，供后续特性宏分支使用
 *
 * @note 判据顺序有讲究：clang 同时定义 __GNUC__，所以先判 clang；ARMCC 5
 *       同时定义 __CC_ARM 与 __GNUC__（部分版本），所以 __CC_ARM 要排在
 *       __GNUC__ 之前。顺序写错会导致"以为走了 GCC 分支，其实在 ARMCC 下
 *       编不过"。
 */
#if defined(__clang__)
#define GFIFO_CC_CLANG 1
#elif defined(__ICCARM__) || defined(__IAR_SYSTEMS_ICC__)
#define GFIFO_CC_IAR 1
#elif defined(__CC_ARM)
#define GFIFO_CC_ARMCC 1
#elif defined(__ARMCC_VERSION)
#define GFIFO_CC_ARMCC 1
#elif defined(__GNUC__)
#define GFIFO_CC_GNU 1
#elif defined(_MSC_VER)
#define GFIFO_CC_MSVC 1
#else
#define GFIFO_CC_UNKNOWN 1
#endif

/*==============================================================================
 * 内联函数
 *============================================================================*/

/**
 * @brief 内部小函数的内联修饰，统一为 "static + inline"
 *
 * @note 必须带 static：头文件里的 inline 函数如果不加 static，在 C99 下会
 *       产生外部链接符号，多个编译单元包含时链接期报 multiple definition。
 *
 * @note 老编译器（ARMCC 5、C89 模式的 IAR）不认 inline 关键字，退化为 static，
 *       函数仍能工作，只是多一次调用开销。
 */
#ifndef GFIFO_INLINE
#if defined(__cplusplus)
#define GFIFO_INLINE static inline
#elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)
#define GFIFO_INLINE static inline
#elif defined(GFIFO_CC_MSVC) || defined(GFIFO_CC_ARMCC) || defined(GFIFO_CC_IAR)
#define GFIFO_INLINE static __inline
#elif defined(GFIFO_CC_GNU) || defined(GFIFO_CC_CLANG)
/* C89 模式下 GCC 不认标准 inline，但认双下划线的扩展写法，
   而双下划线标识符属于保留命名空间，-pedantic 不会因此报错 */
#define GFIFO_INLINE static __inline__
#else
#define GFIFO_INLINE static
#endif
#endif

/*==============================================================================
 * 对齐
 *============================================================================*/

/**
 * @brief 变量对齐修饰，用法为前置：GFIFO_ALIGN(8) static uint8_t buf[256];
 *
 * @note 为什么需要它：结构体 FIFO 的缓冲区必须按元素的最大对齐要求对齐。
 *       如果 buf 落在奇数地址而上层用 *(sensor_t *)ptr 直接解引用，在
 *       Cortex-M0/M0+ 上会触发 HardFault（unaligned access），在 Cortex-M3/M4
 *       上虽然硬件自动拆成两次访问，但慢 2~3 倍。
 *
 * @note C11 的 _Alignas 最标准；C++ 下用 alignas；MSVC 用 __declspec(align())；
 *       GCC 系用 __attribute__((aligned()))。四者都支持"写在声明前面"的形式。
 *
 * @note 保守做法是不管什么平台，结构体 FIFO 的缓冲区一律 GFIFO_ALIGN(8)。
 *       多占几字节 RAM，换掉一整类玄学 HardFault，非常划算。
 */
#ifndef GFIFO_ALIGN
#if defined(__cplusplus) && (__cplusplus >= 201103L)
#define GFIFO_ALIGN(n) alignas(n)
#elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
#define GFIFO_ALIGN(n) _Alignas(n)
#elif defined(GFIFO_CC_MSVC)
#define GFIFO_ALIGN(n) __declspec(align(n))
#elif defined(GFIFO_CC_GNU) || defined(GFIFO_CC_CLANG)
#define GFIFO_ALIGN(n) __attribute__((aligned(n)))
#elif defined(GFIFO_CC_IAR)
#define GFIFO_ALIGN(n) __attribute__((aligned(n)))
#else
#define GFIFO_ALIGN(n) /* 该编译器不支持对齐修饰，调用方需自行保证 */
#endif
#endif

/*==============================================================================
 * 内存屏障
 *============================================================================*/

/**
 * @brief 编译器屏障：阻止编译器把屏障前后的内存访问重排
 *
 * @note 单核 MCU 上这就够了。SPSC 无锁的正确性依赖两件事：
 *       生产者"先写数据、再更新 head"，消费者"先读 head、再读数据"。
 *       如果编译器把 head 的写入提到数据拷贝之前，消费者就会读到未写完的
 *       数据。volatile 只保证"不被优化掉"，不保证"顺序"，所以必须显式屏障。
 *
 * @note 多核 SMP（Cortex-A、双核 M7/M33）上编译期屏障不够，还要数据屏障。
 *       在那种平台上请在此处覆盖为 __DMB() 或 __sync_synchronize()：
 *         #define GFIFO_BARRIER()  __DMB()
 *
 * @note 开销：GCC 下是一条空 asm，编译后不产生任何指令，只影响调度顺序。
 */
#ifndef GFIFO_BARRIER
#if defined(GFIFO_CC_GNU) || defined(GFIFO_CC_CLANG)
#define GFIFO_BARRIER() __asm__ __volatile__("" ::: "memory")
#elif defined(GFIFO_CC_MSVC)
#include <intrin.h>
#define GFIFO_BARRIER() _ReadWriteBarrier()
#elif defined(GFIFO_CC_IAR)
#define GFIFO_BARRIER() __asm volatile("" ::: "memory")
#else
#define GFIFO_BARRIER() ((void)0)
#endif
#endif

/*==============================================================================
 * 临界区
 *============================================================================*/

/**
 * @brief 进入 / 退出临界区，成对使用，参数是当前操作的 gfifo_t 句柄
 *
 * @note 什么时候需要：同一个 FIFO 存在多个生产者，或多个消费者。此时
 *       "读 free → 算位置 → 拷贝 → 更新 head"这一串必须不可分割，否则两个
 *       生产者会算出同一个写入位置，后写的覆盖先写的。
 *
 * @note 什么时候不需要：SPSC（一个中断写、一个任务读）是库的默认假设，
 *       完全无锁，中断不会被关，实时性最好。这是嵌入式里 90% 的用法。
 *
 * @note 为什么把句柄 f 传进来：允许实现"每个 FIFO 一把锁"（RTOS 互斥量
 *       句柄可以挂在 gfifo_t 上），而不是全系统一把大锁。不需要 per-FIFO
 *       锁的实现直接忽略这个参数即可。
 *
 * @note 实现约束（重要）：
 *       - 必须是"可嵌套安全"或"保证不嵌套"的。库内部不会在持锁时调用
 *         另一个会加锁的公共 API，所以正常使用不会嵌套。
 *       - 关中断实现必须在退出时恢复"进入前的状态"，不能无条件开中断，
 *         否则会破坏调用者的临界区嵌套（见文件末尾的 Cortex-M 参考实现）。
 *       - 临界区内不允许阻塞：不能拿 RTOS 互斥量以外的信号量，不能延时。
 *
 * @note 开启 GFIFO_CFG_ENABLE_CRITICAL 却忘记定义这两个宏时，下面的 #error
 *       会立刻拦住，避免"以为有锁其实没锁"的静默错误。
 */
#ifndef GFIFO_ENTER_CRITICAL
#if GFIFO_CFG_ENABLE_CRITICAL
#error "GFIFO_CFG_ENABLE_CRITICAL=1 requires GFIFO_ENTER_CRITICAL to be defined in gfifo_port.h"
#else
#define GFIFO_ENTER_CRITICAL(f) ((void)(f))
#endif
#endif

#ifndef GFIFO_EXIT_CRITICAL
#if GFIFO_CFG_ENABLE_CRITICAL
#error "GFIFO_CFG_ENABLE_CRITICAL=1 requires GFIFO_EXIT_CRITICAL to be defined in gfifo_port.h"
#else
#define GFIFO_EXIT_CRITICAL(f) ((void)(f))
#endif
#endif

/*==============================================================================
 * 断言
 *============================================================================*/

/**
 * @brief 库内断言，仅在 GFIFO_CFG_ENABLE_ASSERT=1 时生效
 *
 * @note 默认映射到 <assert.h>，从而停在调试器里并打印文件名与行号。
 *       裸机产品通常希望换成"记录错误码 + 看门狗复位"，覆盖方法：
 *         #define GFIFO_ASSERT(x)  do { if (!(x)) { g_fault = __LINE__; NVIC_SystemReset(); } } while (0)
 *
 * @note 断言只用于"绝不该发生"的内部一致性检查（例如已持锁却重入），
 *       参数错误一律走返回码，不用断言，避免正常业务把系统打断。
 */
#ifndef GFIFO_ASSERT
#if GFIFO_CFG_ENABLE_ASSERT
#include <assert.h>
#define GFIFO_ASSERT(x) assert(x)
#else
#define GFIFO_ASSERT(x) ((void)0)
#endif
#endif

/*==============================================================================
 * 通用工具宏
 *============================================================================*/

/**
 * @brief 消除"未使用变量/参数"的编译告警，用于被配置裁剪掉的形参
 *
 * @note -Wall -Wextra -Werror 下，功能被 #if 裁掉后形参就没人用了，
 *       不加这个会直接编译失败。
 */
#ifndef GFIFO_UNUSED
#define GFIFO_UNUSED(x) ((void)(x))
#endif

/**
 * @brief 取数组元素个数，只在数组（非指针）上有效
 */
#ifndef GFIFO_ARRAY_SIZE
#define GFIFO_ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif

/**
 * @brief 取两数较小 / 较大值，参数会被求值两次，不要传带副作用的表达式
 */
#ifndef GFIFO_MIN
#define GFIFO_MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef GFIFO_MAX
#define GFIFO_MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

/**
 * @brief 把 v 向上对齐到 a 的整数倍，a 必须大于 0，不要求 a 是 2 的幂
 *
 * @note 用除法而不是"加掩码"实现，是为了让 elem_size = 24 / 36 这类
 *       非 2 的幂也能正确工作。编译器对"除数是编译期常量"的情况会自动
 *       优化成乘法移位；这里的除数来自运行期字段，走硬件除法或库函数，
 *       所以只在初始化与丢弃路径调用，不在热路径调用。
 */
#ifndef GFIFO_ALIGN_UP
#define GFIFO_ALIGN_UP(v, a) ((((v) + (a) - 1u) / (a)) * (a))
#endif

/**
 * @brief 把 v 向下对齐到 a 的整数倍
 */
#ifndef GFIFO_ALIGN_DOWN
#define GFIFO_ALIGN_DOWN(v, a) (((v) / (a)) * (a))
#endif

/**
 * @brief 编译期断言，条件为假时编译失败，msg 在支持的编译器上会出现在报错信息里
 *
 * @note 优先用 C11 的 _Static_assert；老工具链退化为"负长度数组"技巧，
 *       该技巧在 C89 下也成立。两者都不产生任何运行期代码。
 */
#ifndef GFIFO_STATIC_ASSERT
#define GFIFO_SA_CAT_(a, b) a##b
#define GFIFO_SA_CAT(a, b) GFIFO_SA_CAT_(a, b)
#if defined(__cplusplus) && (__cplusplus >= 201103L)
#define GFIFO_STATIC_ASSERT(cond, msg) static_assert(cond, msg)
#elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
#define GFIFO_STATIC_ASSERT(cond, msg) _Static_assert(cond, msg)
#else
/* C99 及更早：负长度数组。条件为假时数组长度为负，编译期报错。
   msg 在这个分支里用不上，但保留形参可以让两边的调用写法完全一致。 */
#define GFIFO_STATIC_ASSERT(cond, msg) \
    typedef char GFIFO_SA_CAT(gfifo_static_assert_, __LINE__)[(cond) ? 1 : -1]
#endif
#endif

/*==============================================================================
 * 平台参考实现（按需取消注释，或直接抄到自己的 board 头文件里）
 *============================================================================*/

#if 0
/*------------------------------------------------------------------------------
 * 参考 1：Cortex-M 裸机，关中断实现临界区
 *
 * 要点：保存 PRIMASK 再恢复，而不是无条件 __enable_irq()。无条件开中断会
 *       把外层更早进入的临界区一起打开，是中断嵌套场景最经典的 bug。
 *----------------------------------------------------------------------------*/
#include "cmsis_compiler.h"          /* 提供 __get_PRIMASK / __set_PRIMASK */

static uint32_t s_gfifo_primask;     /* 不支持嵌套时用全局保存，嵌套场景见参考 2 */

#define GFIFO_ENTER_CRITICAL(f)  do { (void)(f); s_gfifo_primask = __get_PRIMASK(); __disable_irq(); } while (0)
#define GFIFO_EXIT_CRITICAL(f)   do { (void)(f); __set_PRIMASK(s_gfifo_primask); } while (0)
#define GFIFO_BARRIER()          __DMB()

/*------------------------------------------------------------------------------
 * 参考 2：FreeRTOS，每个 FIFO 一把互斥量
 *
 * 要点：互斥量句柄挂在 gfifo_t 的扩展字段或旁路表里；中断里不能用互斥量，
 *       所以 ISR 侧必须是唯一生产者，即这种配置只适用于"多任务生产 +
 *       单任务消费"或"单 ISR 生产 + 多任务消费"。
 *----------------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "semphr.h"

#define GFIFO_ENTER_CRITICAL(f)  xSemaphoreTake((f)->lock, portMAX_DELAY)
#define GFIFO_EXIT_CRITICAL(f)   xSemaphoreGive((f)->lock)

/*------------------------------------------------------------------------------
 * 参考 3：Linux / Windows 用户态，pthread 互斥量
 *
 * 要点：PC 上做单元测试或跑协议栈仿真时用；窗口很小，性能不是瓶颈。
 *----------------------------------------------------------------------------*/
#include <pthread.h>

#define GFIFO_ENTER_CRITICAL(f)  pthread_mutex_lock(&(f)->lock)
#define GFIFO_EXIT_CRITICAL(f)   pthread_mutex_unlock(&(f)->lock)
#endif /* 平台参考实现 */

#endif /* __GFIFO_PORT_H__ */
