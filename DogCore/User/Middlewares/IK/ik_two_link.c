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
 *       用户坐标系：原点在髋关节，+x 前（平行身体平面），+y 下（垂直身体平面）
 *       标准数学坐标系：+x 右，+y 上
 *       坐标变换：x_math = x_user, y_math = -y_user
 *
 *       标准IK公式（用变换后的坐标）：
 *       θ1 = atan2(y_math, x_math) - atan2(L2·sin(θ2), L1+L2·cos(θ2))
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include <stdint.h>
#include <math.h>
#include "ik_two_link.h"

/*==============================================================================
 * IK 解算（弧度输出）
 *============================================================================*/
/**
 * @brief 二连杆逆运动学解算（输出弧度）
 * @param L1    大腿长度(mm)
 * @param L2    小腿长度(mm)
 * @param x     足端 x 坐标（用户坐标系，+x 前）
 * @param y     足端 y 坐标（用户坐标系，+y 下）
 * @param hip   输出：髋关节角度(rad)
 * @param knee  输出：膝关节角度(rad)
 * @return 1=可达，0=不可达（超出工作空间）
 *
 * @details
 *       标准二连杆 IK 模型：
 *           髋 → L1 → 膝 → L2 → 足
 *
 *       用户坐标系：原点在髋关节，+x 前，+y 下。
 *       内部转换为数学坐标系（+x 右，+y 上）后求解。
 *
 *       求解步骤：
 *       1. 余弦定理算膝角
 *       2. 几何法算髋角
 *       3. atan2(-y, x) 做坐标系转换
 *
 * @note
 *       - 角度约定：髋正=前摆，膝正=弯曲
 *       - 不可达时返回 0，输出值不变
 *
 * @see IK_TwoLink_Deg 输出角度的版本
 */
uint8_t IK_TwoLink(float L1, float L2, float x, float y,
                   float *hip, float *knee)
{
    float d_sq = x * x + y * y;
    float d    = sqrtf(d_sq);

    /* ── 可达性检查：足端必须在两杆之和/差范围内 ── */
    if (d > L1 + L2 || d < fabsf(L1 - L2))
        return 0;

    /* ── 膝角（余弦定理） ── */
    float cos_knee = (L1 * L1 + L2 * L2 - d_sq) / (2.0f * L1 * L2);
    if (cos_knee > 1.0f)  cos_knee = 1.0f;
    if (cos_knee < -1.0f) cos_knee = -1.0f;
    *knee = acosf(cos_knee);

    /* ── 髋角（几何法）── 用户坐标系转数学坐标系 ──
       用户y向下，数学y向上，所以 y_math = -y_user
       标准 IK：θ1 = atan2(y_math, x_math) - atan2(...) */
    float alpha = atan2f(L2 * sinf(*knee), L1 + L2 * cosf(*knee));
    *hip = atan2f(-y, x) - alpha;  /* -y 进行坐标系变换 */

    return 1;
}

/*==============================================================================
 * IK 解算（角度输出）
 *============================================================================*/
/**
 * @brief 二连杆逆运动学解算（输出角度）
 * @param L1       大腿长度(mm)
 * @param L2       小腿长度(mm)
 * @param x        足端 x 坐标（用户坐标系，+x 前）
 * @param y        足端 y 坐标（用户坐标系，+y 下）
 * @param hip_deg  输出：髋关节角度(°)
 * @param knee_deg 输出：膝关节角度(°)
 * @return 1=可达，0=不可达（超出工作空间）
 *
 * @details
 *       对 IK_TwoLink 的弧度结果乘 RAD_TO_DEG 得到角度值。
 *       可直接将结果传入 Motion_SetTarget 使用。
 *
 * @note
 *       - hip_deg/knee_deg 为 NULL 时跳过写入，可用于仅检查可达性
 *       - 推荐使用此版本，避免调用方手动转换
 */
uint8_t IK_TwoLink_Deg(float L1, float L2, float x, float y,
                        float *hip_deg, float *knee_deg)
{
    float hip_rad, knee_rad;
    uint8_t ret = IK_TwoLink(L1, L2, x, y, &hip_rad, &knee_rad);

    if (hip_deg)  *hip_deg  = hip_rad  * RAD_TO_DEG;
    if (knee_deg) *knee_deg = knee_rad * RAD_TO_DEG;

    return ret;
}
