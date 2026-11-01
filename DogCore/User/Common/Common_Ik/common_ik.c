/**
 * @file common_ik.c
 * @brief 二连杆逆/正运动学实现（纯数学，无硬件、无状态）
 * @author Ricardo
 * @date 2026-11-01
 *
 * @note 只用 sqrtf/atan2f/sinf/cosf 与四则运算，没有 static 变量：
 *       同一次上电里可以被四条腿并发调用，互不影响。
 *
 *       不能直接用 acos 求 θ1：cos 是偶函数，θ2 取 ±时 cosθ2 相同，
 *       但 θ1 不同。所以先由 ±θ2 算出两组，再用几何法分别求 θ1。
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include <stddef.h>
#include <math.h>
#include "common_ik.h"

/*==============================================================================
 * 宏定义与常量
 *============================================================================*/
/** @brief 圆周率（单精度） */
#define COMMON_IK_PI        3.14159265358979323846f
/** @brief 判定"正好伸直/折叠"的容差（长度单位），单精度下 1 µm 足够 */
#define COMMON_IK_EPS       1.0e-3f

/*==============================================================================
 * 内部函数
 *============================================================================*/
/**
 * @brief 校验杆长是否可用
 * @param link 杆长，可为 NULL
 * @retval 1: 可用
 * @retval 0: 为空或存在非正杆长
 */
static uint8_t Common_Ik_Link_Valid(const common_ik_link_t *link)
{
    if (link == NULL) {
        return 0u;
    }

    if ((link->l1 <= 0.0f) || (link->l2 <= 0.0f)) {
        return 0u;
    }

    return 1u;
}

/**
 * @brief 把解算结果整体清零
 * @param out 结果，可为 NULL
 * @retval 无
 */
static void Common_Ik_Solution_Clear(common_ik_solution_t *out)
{
    if (out == NULL) {
        return;
    }

    out->hip[0] = 0.0f;
    out->hip[1] = 0.0f;
    out->knee[0] = 0.0f;
    out->knee[1] = 0.0f;
    out->count = 0u;
}

/*==============================================================================
 * 逆解：坐标 → 角度
 *============================================================================*/
/**
 * @brief 二连杆逆解，返回两组（或一/零组）关节角
 * @param link 杆长，不可为 NULL
 * @param x    足端 x 坐标（+x 前）
 * @param y    足端 y 坐标（+y 上）
 * @param out  解算结果，不可为 NULL
 * @retval -1: 参数非法，out 已清零
 * @retval 0: 不可达，out 已清零
 * @retval 1: 边界奇异，只有一组解
 * @retval 2: 两组解
 */
int Common_Ik_Solve(const common_ik_link_t *link, float x, float y,
                    common_ik_solution_t *out)
{
    float d2;
    float d;
    float c2;
    float s2;
    float theta2[2];
    float psi;
    float phi[2];

    /* ---------- 1. 参数与可达性 ---------- */
    if ((out == NULL) || (Common_Ik_Link_Valid(link) == 0u)) {
        Common_Ik_Solution_Clear(out);
        return -1;
    }

    Common_Ik_Solution_Clear(out);

    d2 = (x * x) + (y * y);
    d  = sqrtf(d2);

    if ((d > (link->l1 + link->l2 + COMMON_IK_EPS))
        || (d < (fabsf(link->l1 - link->l2) - COMMON_IK_EPS))) {
        return 0;
    }

    /* ---------- 2. 余弦定理求膝角 ---------- */
    c2 = ((d2 - (link->l1 * link->l1) - (link->l2 * link->l2))
          / (2.0f * link->l1 * link->l2));

    if (c2 > 1.0f) {
        c2 = 1.0f;
    } else if (c2 < -1.0f) {
        c2 = -1.0f;
    }

    s2       = sqrtf(1.0f - (c2 * c2));      /* |sinθ2| */
    theta2[0] = atan2f(s2, c2);              /* ≥ 0：膝向后 */
    theta2[1] = atan2f(-s2, c2);             /* ≤ 0：膝向前 */

    /* ---------- 3. 髋角：每组的 θ2 各自求一次 ---------- */
    psi = atan2f(y, x);
    for (uint8_t i = 0u; i < 2u; i++) {
        phi[i] = atan2f(link->l2 * sinf(theta2[i]),
                        link->l1 + (link->l2 * cosf(theta2[i])));

        out->hip[i]  = (psi - phi[i]) * 180.0f / COMMON_IK_PI;
        out->knee[i] = theta2[i] * 180.0f / COMMON_IK_PI;
    }

    /* ---------- 4. 边界情况：完全伸直 / 完全折叠时两组重合 ---------- */
    if ((fabsf(d - (link->l1 + link->l2)) < COMMON_IK_EPS)
        || (fabsf(d - fabsf(link->l1 - link->l2)) < COMMON_IK_EPS)) {
        out->hip[1]  = out->hip[0];
        out->knee[1] = out->knee[0];
        out->count   = 1u;
        return 1;
    }

    out->count = 2u;
    return 2;
}

/*==============================================================================
 * 正解：角度 → 坐标
 *============================================================================*/
/**
 * @brief 二连杆正解：给定关节角求足端坐标
 * @param link 杆长，不可为 NULL
 * @param hip  髋关节角度（度）
 * @param knee 膝关节角度（度）
 * @param x    输出足端 x 坐标，可为 NULL
 * @param y    输出足端 y 坐标，可为 NULL
 * @retval 0: 成功
 * @retval -1: 杆长为空或非正
 * @note x = l1·cos(θ1) + l2·cos(θ1+θ2)，y = l1·sin(θ1) + l2·sin(θ1+θ2)。
 */
int Common_Ik_Solve_Forward(const common_ik_link_t *link, float hip, float knee,
                            float *x, float *y)
{
    float t1;
    float t12;

    if (Common_Ik_Link_Valid(link) == 0u) {
        return -1;
    }

    t1  = hip * COMMON_IK_PI / 180.0f;
    t12 = (hip + knee) * COMMON_IK_PI / 180.0f;

    if (x != NULL) {
        *x = (link->l1 * cosf(t1)) + (link->l2 * cosf(t12));
    }
    if (y != NULL) {
        *y = (link->l1 * sinf(t1)) + (link->l2 * sinf(t12));
    }

    return 0;
}
