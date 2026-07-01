/**
 * @file app_motion_ik.c
 * @brief IK 协调层实现
 * @author 李嘉图
 * @date 2026-07-01
 *
 * @note 本层位于 app_motion_scheduler 与 app_motion 之间，
 *       对上层提供统一的 MotionIK_ApplyPhase() 接口，
 *       对内根据 gait->mode 自动选择角度直通或 IK 解算。
 *       IK 层使用改进版 ik_two_link（double 精度，两组解），
 *       通过 MOTIONIK_SOLUTION 宏选择膝向前/向后姿态。
 *
 * @see app_motion_ik.h
 * @see ik_two_link
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include <stddef.h>
#include "app_motion_ik.h"
// 功能包含
#include "app_motion.h"
#include "ik_two_link.h"

/*==============================================================================
 * 默认杆长（mm）
 *============================================================================*/
/**
 * @brief 默认大腿长度(mm)
 * @note 可根据实际机器人尺寸修改，或运行时调用 MotionIK_SetLinkLength()
 */
#define MOTIONIK_L1  140.0f

/**
 * @brief 默认小腿长度(mm)
 * @note 可根据实际机器人尺寸修改，或运行时调用 MotionIK_SetLinkLength()
 */
#define MOTIONIK_L2  120.0f

/*==============================================================================
 * IK 解选择配置
 *============================================================================*/
/**
 * @brief IK 解选择枚举
 *
 * 二连杆 IK 存在两组运动学解，对应膝关节的两种弯曲方向：
 *   - IK_SOLUTION_KNEE_BACKWARD: 膝向后（θ2 > 0），等同于旧版 acosf 解法
 *   - IK_SOLUTION_KNEE_FORWARD:  膝向前（θ2 < 0），膝反向弯曲
 *
 * @note 旧版 IK_TwoLink 仅返回膝向后（θ2 > 0）单组解。
 *       如需膝向前姿态，将 MOTIONIK_SOLUTION 改为 IK_SOLUTION_KNEE_FORWARD。
 * @warning 实际关节空间需要根据机械结构确认，如方向不对直接交换宏值即可。
 */
#define IK_SOLUTION_KNEE_FORWARD   0   /**< 方式①：膝在前 */
#define IK_SOLUTION_KNEE_BACKWARD  1   /**< 方式②：膝在后（默认） */

/**
 * @brief 当前使用的 IK 解
 *
 * 默认为 IK_SOLUTION_KNEE_BACKWARD（膝向后，与旧版一致）。
 * 如需膝向前，改为 IK_SOLUTION_KNEE_FORWARD。
 */
#define MOTIONIK_SOLUTION  IK_SOLUTION_KNEE_FORWARD


/*==============================================================================
 * 相位应用（核心分发）
 *============================================================================*/
/**
 * @brief 应用步态相位（自动按 mode 分发）
 * @param gait        步态指针
 * @param phase_index 相位索引
 *
 * @details
 *       根据 gait->mode 决定控制方式：
 *
 *       【mode=0：角度直通】
 *         取 leg.angle.hip / leg.angle.knee 直接传入 Motion_SetTarget。
 *         与原有的 MotionSched_ApplyPhase 行为完全一致。
 *
 *       【mode=1：二连杆 IK 解算】
 *         取 leg.pos.x / leg.pos.y 作为足端坐标，
 *         调用 ik_two_link() 逆解出两组关节角度，
 *         由 MOTIONIK_SOLUTION 宏选择膝向前/向后，
 *         再将选中的角度传入 Motion_SetTarget。
 *         解算不可达时跳过该腿（不发送指令），避免舵机异常动作。
 *
 * @note
 *       - IK 解算不可达时腿保持当前位置不变
 *       - 用户坐标系（+x 前，+y 下）在调用 IK 前转换为标准数学坐标系（+x 前，+y 上）
 *       - 膝向后（θ2>0）为默认解，等同于旧版 IK_TwoLink_Deg 行为
 */
void MotionIK_ApplyPhase(const motion_gait_t *gait, uint16_t phase_index)
{
    if (gait == NULL) return;
    if (phase_index >= gait->phase_count) return;

    const motion_phase_t *phase = &gait->phases[phase_index];

    if (gait->mode == 0)
    {
        /* ── 角度直通模式（原始行为） ── */
        for (uint8_t i = 0; i < phase->leg_count; i++)
        {
            uint8_t leg_id    = phase->legs[i].leg_id;
            float target[2] = {phase->legs[i].angle.hip, phase->legs[i].angle.knee};
            Motion_SetTarget(leg_id, target, phase->speed);
        }
    }
    else
    {
        /* ── 二连杆坐标控制模式 ──
         *
         * 用户数据约定：pos.y 使用正值表示足端向下伸展的距离（如 y=260 → 伸260mm）
         *
         * 处理分两层：
         *   ① 反转层：正值 → 内部负值
         *      pos.y = 260 → y_rev = -260
         *   ② 坐标转换：+y 下 → +y 上（IK 数学坐标系）
         *      y_rev = -260 → y_ik = +260
         *
         * 净效果：y_ik = pos.y（数据正值直接送入 IK）
         */
        for (uint8_t i = 0; i < phase->leg_count; i++)
        {
            uint8_t leg_id = phase->legs[i].leg_id;
            double x_ik = (double)phase->legs[i].pos.x;

            /* ① 反转层：正值（向下距离）→ 内部负值 */
            double y_rev = -(double)phase->legs[i].pos.y;

            /* ② 坐标系转换（+y 下 → +y 上） */
            double y_ik = -y_rev;

            double hip1, knee1;   /* 解1：膝向后（θ2>0） */
            double hip2, knee2;   /* 解2：膝向前（θ2<0） */

            int n = ik_two_link(MOTIONIK_L1, MOTIONIK_L2,
                                x_ik, y_ik,
                                &hip1, &knee1,   /* 解1：膝向后 */
                                &hip2, &knee2);  /* 解2：膝向前 */

            if (n > 0)
            {
                /* ── 按配置选择膝向后/向前解 ── */
                double hip_selected  = hip1;
                double knee_selected = knee1;
#if MOTIONIK_SOLUTION == IK_SOLUTION_KNEE_FORWARD
                hip_selected  = hip2;
                knee_selected = knee2;
#endif
                float target[2] = {(float)hip_selected, (float)knee_selected};
                Motion_SetTarget(leg_id, target, phase->speed);
            }
            /* 不可达时跳过该腿，保持当前位置 */
        }
    }
}
