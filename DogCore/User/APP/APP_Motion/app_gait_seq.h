/**
 * @file app_gait_seq.h
 * @brief 步态序列数据（简化版：角度模式）
 * @author 李嘉图
 * @date 2026-06-29
 *
 * @note 当前版本：纯角度控制
 *       输入格式：leg_id + hip角度 + knee角度
 *       角度符合二连杆模型要求
 */

#ifndef __APP_GAIT_SEQ_H__
#define __APP_GAIT_SEQ_H__

#include "app_motion_scheduler.h"

/*==============================================================================
 * 踢腿动作（角度模式）
 *
 * 腿0交替执行：抬腿 → 落回
 * 其他腿保持空闲
 *
 * 标定数据：
 *   servo(0°, 90°)      = 足端垂直最远(0, 260)
 *   servo(-77°, -63°)   = 足端抬起最高(0, 27.4)
 *
 * 角度符合二连杆模型（考虑offset_deg=-90的小腿偏移）
 *============================================================================*/
static const motion_gait_t GAIT_KICK0 = {
    .phases = (const motion_phase_t[]){
        {
            .legs = (const leg_phase_data_t[]){
                { .leg_id = 1, .angle = {.hip = -77, .knee = -63} },
            },
            .leg_count = 1,
            .speed = 100.0f,
        },
        {
            .legs = (const leg_phase_data_t[]){
                { .leg_id = 1, .angle = {.hip = -38, .knee = 27} },
            },
            .leg_count = 1,
            .speed = 61.8f,         
        },
    },
    .phase_count = 2,
    .loop = 1,
};

/*==============================================================================
 * 站立动作
 *============================================================================*/
static const motion_gait_t GAIT_KICK1 = {
    .phases = (const motion_phase_t[]){
        {
            .legs = (const leg_phase_data_t[]){
                { .leg_id = 1, .angle = {.hip = 0, .knee = 90} },
            },
            .leg_count = 1,
            .speed = 50.0f,
        },
    },
    .phase_count = 1,
    .loop = 0,
};

#endif /* __APP_GAIT_SEQ_H__ */
