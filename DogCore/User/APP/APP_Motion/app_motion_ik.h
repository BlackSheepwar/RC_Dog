/**
 * @file app_motion_ik.h
 * @brief IK 协调层 — 在步态调度与角度控制之间附加二连杆逆运动学
 * @author 李嘉图
 * @date 2026-06-30
 *
 * @note 本层是 app_motion_scheduler → [IK 层] → app_motion 的中间层。
 *       根据 gait->mode 自动选择：
 *         mode=0: 直通角度控制（直接调 Motion_SetTarget）
 *         mode=1: 二连杆 IK 解算（坐标 → 角度 → Motion_SetTarget）
 *
 * @see IK_TwoLink_Deg
 */

#ifndef __APP_MOTION_IK_H__
#define __APP_MOTION_IK_H__

#include <stdint.h>
#include "app_motion_scheduler.h"

/*==============================================================================
 * API
 *============================================================================*/
/**
 * @brief 应用步态相位（自动按 mode 分发）
 * @param gait        步态指针
 * @param phase_index 相位索引
 *
 * @details
 *       mode=0: 直接取 leg.angle → Motion_SetTarget（原行为）
 *       mode=1: 取 leg.pos(x,y) → IK_TwoLink_Deg → Motion_SetTarget
 *
 * @note
 *       - IK 解算不可达时（超出工作空间），该腿被跳过（不发送指令）
 *       - 相位索引越界时静默返回
 */
void MotionIK_ApplyPhase(const motion_gait_t *gait, uint16_t phase_index);

#endif /* __APP_MOTION_IK_H__ */
