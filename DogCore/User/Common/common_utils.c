/**
 * @file common_utils.c
 * @brief 通用工具函数实现
 * @author 李嘉图
 * @date 2026-07-07
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
#include <stdio.h>
#include "common_utils.h"

/*==============================================================================
 * 格式转换函数
 *============================================================================*/
/**
 * @brief float 格式转字符串
 * @param value    待转换的浮点数
 * @param buf      输出缓冲区
 * @param decimals 小数位数
 */
void float_to_str(float value, char *buf, int decimals)
{
    if (value < 0) {
        *buf++ = '-';
        value = -value;
    }

    int int_part = (int)value;
    float frac = value - int_part;

    sprintf(buf, "%d", int_part);
    while (*buf) buf++;

    *buf++ = '.';

    for (int i = 0; i < decimals; i++)
    {
        frac *= 10;
        int digit = (int)frac;
        *buf++ = digit + '0';
        frac -= digit;
    }

    *buf = '\0';
}
