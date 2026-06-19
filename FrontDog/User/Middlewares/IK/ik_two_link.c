/**
 * @file ik_two_link.c
 * @brief 二连杆逆运动学（IK）解算实现
 * @author 李嘉图
 * @date 2026-06-11
 *
 * @note 关节角度约定：
 *       髋关节：正 = 前摆，负 = 后摆
 *       膝关节：正 = 弯曲，负 = 过伸（正常情况下不会出现）
 *       两连杆模型：髋 → 膝（L1）→ 足（L2）
 *
 *       解算步骤：
 *       1. 余弦定理求膝角
 *       2. 用 atan2 + β 角求髋角
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include <stdint.h>
#include <math.h>
#include "ik_two_link.h"

/*==============================================================================
 * 函数实现
 *============================================================================*/
/**
 * @brief 二连杆逆运动学解算
 * @param L1    大腿连杆长度（mm）
 * @param L2    小腿连杆长度（mm）
 * @param x     足端 x 坐标（前后，+前）
 * @param z     足端 z 坐标（竖直，+下）
 * @param hip   [out] 髋关节角（弧度）
 * @param knee  [out] 膝关节角（弧度）
 * @retval 1 可达，角度有效
 * @retval 0 不可达（超出工作空间）
 *
 * @note 解算步骤：
 *       1. d = sqrt(x² + z²)，检查是否在 [|L1-L2|, L1+L2] 内
 *       2. 余弦定理求 cos_knee → acos → knee
 *       3. atan2(x,z) - atan2(L2·sin(knee), L1+L2·cos(knee)) → hip
 */
uint8_t IK_TwoLink(float L1, float L2, float x, float z,
                   float *hip, float *knee)
{
    float d_sq = x * x + z * z;
    float d    = sqrtf(d_sq);

    /* ── 可达性检查 ── */
    if (d > L1 + L2 || d < fabsf(L1 - L2))
        return 0;

    /* ── 膝角（余弦定理） ── */
    float cos_knee = (L1 * L1 + L2 * L2 - d_sq) / (2.0f * L1 * L2);
    /* 数值保护：浮点舍入可能导致 cos 略超 [-1,1] */
    if (cos_knee > 1.0f)  cos_knee = 1.0f;
    if (cos_knee < -1.0f) cos_knee = -1.0f;
    *knee = acosf(cos_knee);

    /* ── 髋角（几何法） ── */
    *hip = atan2f(x, z) - atan2f(L2 * sinf(*knee), L1 + L2 * cosf(*knee));

    return 1;
}
