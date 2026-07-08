/**
 * @file common_utils.h
 * @brief 通用工具函数
 * @author 李嘉图
 * @date 2026-07-07
 *
 * @note 跨模块复用的纯工具函数，无硬件/平台依赖。
 */

#ifndef __COMMON_UTILS_H__
#define __COMMON_UTILS_H__

/*==============================================================================
 * 格式转换函数
 *============================================================================*/
/**
 * @brief float 格式转字符串
 * @param value    待转换的浮点数
 * @param buf      输出缓冲区（调用方确保足够大）
 * @param decimals 小数位数
 * @note  不使用 sprintf("%f")，避免依赖 libc 浮点格式化（占用大）。
 *        输出示例: value=3.14, decimals=3 → buf="3.140"
 */
void float_to_str(float value, char *buf, int decimals);

#endif /* __COMMON_UTILS_H__ */
