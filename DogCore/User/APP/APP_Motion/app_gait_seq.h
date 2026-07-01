/**
 * @brief 步态序列数据
 * @author 李嘉图
 * @date 2026-06-30
 *
 * @note 支持两种控制模式（由 gait->mode 指定）：
 *       mode=0：输入关节角度（°）
 *       mode=1：输入足端坐标（mm），由 IK 层逆解为关节角度
 *
 *       mode=1 时 pos.y 使用正值表示足端向下伸展距离，
 *       IK 层内部反转后解算（参见 app_motion_ik.c 反转层）。
 *       例：
 *         { .leg_id = 1, .pos = {.x = 100.0f, .y = 200.0f} }
 *         → 右前方 100mm、向下伸展 200mm
 *
 * 标定数据：
 *   servo(0°, 90°)      = 足端垂直最远(0, 260)
 *   servo(-77°, -63°)   = 足端抬起最高(0, 27.4)
 *
 * 角度符合二连杆模型（考虑offset_deg=-90的小腿偏移）
 */

#ifndef __APP_GAIT_SEQ_H__
#define __APP_GAIT_SEQ_H__

#include "app_motion_scheduler.h"

/*==============================================================================
 * 踱步（循环）
 *============================================================================*/
static const motion_gait_t GAIT_KICK0 = {
    .phases = (const motion_phase_t[]){
        {
            .legs = (const leg_phase_data_t[]){
                { .leg_id = 1, .pos = {.x = 0, .y = 80} },
                { .leg_id = 2, .pos = {.x = 0, .y = 150} },
                { .leg_id = 3, .pos = {.x = 0, .y = 150} },
                { .leg_id = 4, .pos = {.x = 0, .y = 150} },

            },
            .leg_count = 4,
            .speed = 100.0f,
        },
        {
            .legs = (const leg_phase_data_t[]){
                { .leg_id = 1, .pos = {.x = 0, .y = 150} },
                { .leg_id = 2, .pos = {.x = 0, .y = 150} },
                { .leg_id = 3, .pos = {.x = 0, .y = 150} },
                { .leg_id = 4, .pos = {.x = 0, .y = 80} }
            },
            .leg_count = 4,
            .speed = 100.0f,         
        },
                {
            .legs = (const leg_phase_data_t[]){
                { .leg_id = 1, .pos = {.x = 0, .y = 150} },
                { .leg_id = 2, .pos = {.x = 0, .y = 80} },
                { .leg_id = 3, .pos = {.x = 0, .y = 150} },
                { .leg_id = 4, .pos = {.x = 0, .y = 150} },

            },
            .leg_count = 4,
            .speed = 100.0f,
        },
        {
            .legs = (const leg_phase_data_t[]){
                { .leg_id = 1, .pos = {.x = 0, .y = 150} },
                { .leg_id = 2, .pos = {.x = 0, .y = 80} },
                { .leg_id = 3, .pos = {.x = 0, .y = 150} },
                { .leg_id = 4, .pos = {.x = 0, .y = 150} }
            },
            .leg_count = 4,
            .speed = 100.0f,
        },
    },
    .phase_count = 4,
    .loop = 1,
    .mode = 1,
};

/*==============================================================================
 * 踱步（循环）
 *============================================================================*/
static const motion_gait_t GAIT_KICK1 = {
    .phases = (const motion_phase_t[]){
        {
            .legs = (const leg_phase_data_t[]){
                { .leg_id = 1, .pos = {.x = 0, .y = 80} },
                { .leg_id = 2, .pos = {.x = 20, .y = 150} },
                { .leg_id = 3, .pos = {.x = -20, .y = 150} },
                { .leg_id = 4, .pos = {.x = -20, .y = 150} },

            },
            .leg_count = 4,
            .speed = 100.0f,
        },
        {
            .legs = (const leg_phase_data_t[]){
                { .leg_id = 1, .pos = {.x = 0, .y = 150} },
                { .leg_id = 2, .pos = {.x = 20, .y = 150} },
                { .leg_id = 3, .pos = {.x = -20, .y = 150} },
                { .leg_id = 4, .pos = {.x = -20, .y = 150} }
            },
            .leg_count = 4,
            .speed = 100.0f,         
        },
    },
    .phase_count = 2,
    .loop = 0,
    .mode = 1,
};


/*==============================================================================
 * 爬服（单次）
 *============================================================================*/
static const motion_gait_t GAIT_KICK2 = {
    .phases = (const motion_phase_t[]){
        {
            .legs = (const leg_phase_data_t[]){
                { .leg_id = 1, .pos = {.x = 0, .y = 200} },
                { .leg_id = 2, .pos = {.x = 0, .y = 200} },
                { .leg_id = 3, .pos = {.x = 0, .y = 200} },
                { .leg_id = 4, .pos = {.x = 0, .y = 200} },
            },
            .leg_count = 4,
            .speed = 50.0f,
        },
    },
    .phase_count = 1,
    .loop = 0,
    .mode = 1,
};





#endif /* __APP_GAIT_SEQ_H__ */
