/**
 * @file common.h
 * @brief 平台无关通用定义
 * @author 李嘉图
 * @date 2026-06-04
 *
 * @note 换 MCU 平台时这一层不动，只改 BSP 和 OS Layer。
 *       包含基础类型别名、编译器宏、通用错误码。
 */

#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdint.h>
#include <stddef.h>

/*==============================================================================
 * 平台抽象
 *============================================================================*/
/* 字节序（小端为目标平台） */
#define IS_LITTLE_ENDIAN_PLATFORM   1

/* 结构体打包（不同编译器兼容） */
#if defined(__CC_ARM) || defined(__ARMCC_VERSION)
    #define PACKED      __packed
#elif defined(__GNUC__) || defined(__clang__)
    #define PACKED      __attribute__((packed))
#else
    #define PACKED
#endif

/*==============================================================================
 * 通用返回状态
 *============================================================================*/
typedef enum {
    ERR_OK      =  0,    // 成功
    ERR_PARAM   = -1,    // 参数错误
    ERR_BUSY    = -2,    // 资源忙
    ERR_TIMEOUT = -3,    // 超时
    ERR_LOCKED  = -4,    // 全局锁定（紧急停止/致命错误）
    ERR_UNKNOWN = -5,    // 未知错误
} ErrorCode_t;

#endif /* __COMMON_H__ */
