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
#include "app_motion_ik.h"

#define bab1  130
#define bab2  200

/*==============================================================================
 * 踱步1（循环）
 *============================================================================*/
static const motion_gait_t GAIT_KICK1 = {
    .phases = (const motion_phase_t[]){



                {
            .legs = (const leg_phase_data_t[]){
                { .leg_id = 1, .pos = {.x = 0, .y = bab2} },
                { .leg_id = 2, .pos = {.x = 0, .y = bab1} },
                { .leg_id = 3, .pos = {.x = 0, .y = bab2} },
                { .leg_id = 4, .pos = {.x = 0, .y = bab2} },

            },
            .leg_count = 4,
            .speed = 100.0f,
        },
        {
            .legs = (const leg_phase_data_t[]){
                { .leg_id = 1, .pos = {.x = 0, .y = bab2} },
                { .leg_id = 2, .pos = {.x = 0, .y = bab2} },
                { .leg_id = 3, .pos = {.x = 0, .y = bab1} },
                { .leg_id = 4, .pos = {.x = 0, .y = bab2} }
            },
            .leg_count = 4,
            .speed = 100.0f,
        },
                {
            .legs = (const leg_phase_data_t[]){
                { .leg_id = 1, .pos = {.x = 0, .y = bab1} },
                { .leg_id = 2, .pos = {.x = 0, .y = bab2} },
                { .leg_id = 3, .pos = {.x = 0, .y = bab2} },
                { .leg_id = 4, .pos = {.x = 0, .y = bab2} },

            },
            .leg_count = 4,
            .speed = 100.0f,
        },
        {
            .legs = (const leg_phase_data_t[]){
                { .leg_id = 1, .pos = {.x = 0, .y = bab2} },
                { .leg_id = 2, .pos = {.x = 0, .y = bab2} },
                { .leg_id = 3, .pos = {.x = 0, .y = bab2} },
                { .leg_id = 4, .pos = {.x = 0, .y = bab1} }
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
 * 踱步2（循环）
 *============================================================================*/
static const motion_gait_t GAIT_KICK2 = {
    .phases = (const motion_phase_t[]){
        {
            .legs = (const leg_phase_data_t[]){
                { .leg_id = 1, .pos = {.x = 0, .y = bab1} },
                { .leg_id = 2, .pos = {.x = 0, .y = bab2} },
                { .leg_id = 3, .pos = {.x = 0, .y = bab2} },
                { .leg_id = 4, .pos = {.x = 0, .y = bab1} },

            },
            .leg_count = 4,
            .speed = 80.0f,
        },
                {
            .legs = (const leg_phase_data_t[]){
                { .leg_id = 1, .pos = {.x = 0, .y = bab2} },
                { .leg_id = 2, .pos = {.x = 0, .y = bab2} },
                { .leg_id = 3, .pos = {.x = 0, .y = bab2} },
                { .leg_id = 4, .pos = {.x = 0, .y = bab2} },

            },
            .leg_count = 4,
            .speed = 80.0f,
        },

        {
            .legs = (const leg_phase_data_t[]){
                { .leg_id = 1, .pos = {.x = 0, .y = bab2} },
                { .leg_id = 2, .pos = {.x = 0, .y = bab1} },
                { .leg_id = 3, .pos = {.x = 0, .y = bab1} },
                { .leg_id = 4, .pos = {.x = 0, .y = bab2} }
            },
            .leg_count = 4,
            .speed = 80.0f,
        },
                {
            .legs = (const leg_phase_data_t[]){
                { .leg_id = 1, .pos = {.x = 0, .y = bab2} },
                { .leg_id = 2, .pos = {.x = 0, .y = bab2} },
                { .leg_id = 3, .pos = {.x = 0, .y = bab2} },
                { .leg_id = 4, .pos = {.x = 0, .y = bab2} }
            },
            .leg_count = 4,
            .speed = 80.0f,
        },

    },
    .phase_count = 4,
    .loop = 1,
    .mode = 1,
};

/*==============================================================================
 * 站立（单次）
 *============================================================================*/
static const motion_gait_t GAIT_Init = {
    .phases = (const motion_phase_t[]){
        {
            .legs = (const leg_phase_data_t[]){
                { .leg_id = 1, .pos = {.x = 0, .y = 200} },
                { .leg_id = 2, .pos = {.x = 0, .y = 200} },
                { .leg_id = 3, .pos = {.x = 0, .y = 200} },
                { .leg_id = 4, .pos = {.x = 0, .y = 200} },
            },
            .leg_count = 4,
            .speed = 75.0f,
        },
    },
    .phase_count = 1,
    .loop = 0,
    .mode = 1,
};

/*==============================================================================
 * 竖直
 *============================================================================*/
static const motion_gait_t GAIT_Init_1 = {
    .phases = (const motion_phase_t[]){
        {
            .legs = (const leg_phase_data_t[]){
                { .leg_id = 1, .pos = {.x = 0, .y = MOTIONIK_L1+MOTIONIK_L2} },
                { .leg_id = 2, .pos = {.x = 0, .y = MOTIONIK_L1+MOTIONIK_L2} },
                { .leg_id = 3, .pos = {.x = 0, .y = MOTIONIK_L1+MOTIONIK_L2} },
                { .leg_id = 4, .pos = {.x = 0, .y = MOTIONIK_L1+MOTIONIK_L2} },
            },
            .leg_count = 4,
            .speed = 75.0f,
        },
    },
    .phase_count = 1,
    .loop = 0,
    .mode = 1,
};

/*==============================================================================
 * 趴着
 *============================================================================*/
static const motion_gait_t GAIT_Init_2 = {
    .phases = (const motion_phase_t[]){
        {
            .legs = (const leg_phase_data_t[]){
                { .leg_id = 1, .pos = {.x = 0, .y = 100} },
                { .leg_id = 2, .pos = {.x = 0, .y = 100} },
                { .leg_id = 3, .pos = {.x = 0, .y = 100} },
                { .leg_id = 4, .pos = {.x = 0, .y = 100} },
            },
            .leg_count = 4,
            .speed = 75.0f,
        },
    },
    .phase_count = 1,
    .loop = 0,
    .mode = 1,
};

/*==============================================================================
 * 行走步态1 — Front
 *============================================================================*/
/** @name GAIT_Walk_1_Front 坐标常量
 *  @brief 步态相位中腿部的位置参数
 *  @note  X 轴：正值为前进方向，负值为后退方向
 *         Y 轴：数值越小抬得越高（坐标原点在肩关节）
 *         - E  : 支撑相高度，脚着地
 *         - F  : 摆动相高度，脚抬起
 *         - G  : 过渡高度，介于支撑与摆动之间
 *  @{
 */
#define A_W1F   (50)        /**< 全步幅偏移 */
#define B_W1F   (25)        /**< 半步幅偏移 */
#define C_W1F   (0)         /**< 居中位置 */

#define E_W1F   (260.0f)   /**< 支撑相高度 */
#define F_W1F   (200.0f)   /**< 摆动相高度 */
#define G_W1F   (223.8f)   /**< 过渡高度 */

/** @brief 行走步态速度百分比 */
#define SPEED_W1F      (100.0f)    /**< 行走主速度 */
/** @} */

static const motion_gait_t GAIT_Walk_1_Front = {
    .phases = (const motion_phase_t[]){
        {
            .legs = (const leg_phase_data_t[]){
                { .leg_id = 1, .pos = {.x = A_W1F, .y = E_W1F} },
                { .leg_id = 2, .pos = {.x = -A_W1F, .y = E_W1F} },
                { .leg_id = 3, .pos = {.x = A_W1F, .y = E_W1F} },
                { .leg_id = 4, .pos = {.x = -A_W1F, .y = E_W1F} },
            },
            .leg_count = 4,
            .speed = SPEED_W1F,
        },
        {
            .legs = (const leg_phase_data_t[]){
                { .leg_id = 1, .pos = {.x = B_W1F, .y = E_W1F} },
                { .leg_id = 2, .pos = {.x = -B_W1F, .y = G_W1F} },
                { .leg_id = 3, .pos = {.x = B_W1F, .y = G_W1F} },
                { .leg_id = 4, .pos = {.x = -B_W1F, .y = E_W1F} },
            },
            .leg_count = 4,
            .speed = SPEED_W1F,
        },
        {
            .legs = (const leg_phase_data_t[]){
                { .leg_id = 1, .pos = {.x = C_W1F, .y = E_W1F} },
                { .leg_id = 2, .pos = {.x = C_W1F, .y = F_W1F} },
                { .leg_id = 3, .pos = {.x = C_W1F, .y = F_W1F} },
                { .leg_id = 4, .pos = {.x = C_W1F, .y = E_W1F} },
            },
            .leg_count = 4,
            .speed = SPEED_W1F,
        },
        {
            .legs = (const leg_phase_data_t[]){
                { .leg_id = 1, .pos = {.x = -B_W1F, .y = E_W1F} },
                { .leg_id = 2, .pos = {.x = B_W1F, .y = G_W1F} },
                { .leg_id = 3, .pos = {.x = -B_W1F, .y = G_W1F} },
                { .leg_id = 4, .pos = {.x = B_W1F, .y = E_W1F} },
            },
            .leg_count = 4,
            .speed = SPEED_W1F,
        },
        {
            .legs = (const leg_phase_data_t[]){
                { .leg_id = 1, .pos = {.x = -A_W1F, .y = E_W1F} },
                { .leg_id = 2, .pos = {.x = A_W1F, .y = E_W1F} },
                { .leg_id = 3, .pos = {.x = -A_W1F, .y = E_W1F} },
                { .leg_id = 4, .pos = {.x = A_W1F, .y = E_W1F} },
            },
            .leg_count = 4,
            .speed = SPEED_W1F,
        },
        {
            .legs = (const leg_phase_data_t[]){
                { .leg_id = 1, .pos = {.x = -B_W1F, .y = G_W1F} },
                { .leg_id = 2, .pos = {.x = B_W1F, .y = E_W1F} },
                { .leg_id = 3, .pos = {.x = -B_W1F, .y = E_W1F} },
                { .leg_id = 4, .pos = {.x = B_W1F, .y = G_W1F} },
            },
            .leg_count = 4,
            .speed = SPEED_W1F,
        },
        {
            .legs = (const leg_phase_data_t[]){
                { .leg_id = 1, .pos = {.x = C_W1F, .y = F_W1F} },
                { .leg_id = 2, .pos = {.x = C_W1F, .y = E_W1F} },
                { .leg_id = 3, .pos = {.x = C_W1F, .y = E_W1F} },
                { .leg_id = 4, .pos = {.x = C_W1F, .y = F_W1F} },
            },
            .leg_count = 4,
            .speed = SPEED_W1F,
        },
        {
            .legs = (const leg_phase_data_t[]){
                { .leg_id = 1, .pos = {.x = B_W1F, .y = G_W1F} },
                { .leg_id = 2, .pos = {.x = -B_W1F, .y = E_W1F} },
                { .leg_id = 3, .pos = {.x = B_W1F, .y = E_W1F} },
                { .leg_id = 4, .pos = {.x = -B_W1F, .y = G_W1F} },
            },
            .leg_count = 4,
            .speed = SPEED_W1F,
        },
    },
    .phase_count = 8,
    .loop = 1,
    .mode = 1,
};

/*==============================================================================
 * 行走步态1 — Queen
 *============================================================================*/
/** @name GAIT_Walk_1_Queen 坐标常量
 *  @brief 步态相位中腿部的位置参数
 *  @note  X 轴：正值为前进方向，负值为后退方向
 *         Y 轴：数值越小抬得越高（坐标原点在肩关节）
 *         - E  : 支撑相高度，脚着地
 *         - F  : 摆动相高度，脚抬起
 *         - G  : 过渡高度，介于支撑与摆动之间
 *  @{
 */
#define A_W1Q   (-50)        /**< 全步幅偏移 */
#define B_W1Q   (-25)        /**< 半步幅偏移 */
#define C_W1Q   (0)         /**< 居中位置 */

#define E_W1Q   (260.0f)   /**< 支撑相高度 */
#define F_W1Q   (200.0f)   /**< 摆动相高度 */
#define G_W1Q   (223.8f)   /**< 过渡高度 */

/** @brief 行走步态速度百分比 */
#define SPEED_W1Q      (100.0f)    /**< 行走主速度 */
/** @} */

static const motion_gait_t GAIT_Walk_1_Queen = {
    .phases = (const motion_phase_t[]){
        {
            .legs = (const leg_phase_data_t[]){
                { .leg_id = 1, .pos = {.x = A_W1Q, .y = E_W1Q} },
                { .leg_id = 2, .pos = {.x = -A_W1Q, .y = E_W1Q} },
                { .leg_id = 3, .pos = {.x = A_W1Q, .y = E_W1Q} },
                { .leg_id = 4, .pos = {.x = -A_W1Q, .y = E_W1Q} },
            },
            .leg_count = 4,
            .speed = SPEED_W1Q,
        },
        {
            .legs = (const leg_phase_data_t[]){
                { .leg_id = 1, .pos = {.x = B_W1Q, .y = E_W1Q} },
                { .leg_id = 2, .pos = {.x = -B_W1Q, .y = G_W1Q} },
                { .leg_id = 3, .pos = {.x = B_W1Q, .y = G_W1Q} },
                { .leg_id = 4, .pos = {.x = -B_W1Q, .y = E_W1Q} },
            },
            .leg_count = 4,
            .speed = SPEED_W1Q,
        },
        {
            .legs = (const leg_phase_data_t[]){
                { .leg_id = 1, .pos = {.x = C_W1Q, .y = E_W1Q} },
                { .leg_id = 2, .pos = {.x = C_W1Q, .y = F_W1Q} },
                { .leg_id = 3, .pos = {.x = C_W1Q, .y = F_W1Q} },
                { .leg_id = 4, .pos = {.x = C_W1Q, .y = E_W1Q} },
            },
            .leg_count = 4,
            .speed = SPEED_W1Q,
        },
        {
            .legs = (const leg_phase_data_t[]){
                { .leg_id = 1, .pos = {.x = -B_W1Q, .y = E_W1Q} },
                { .leg_id = 2, .pos = {.x = B_W1Q, .y = G_W1Q} },
                { .leg_id = 3, .pos = {.x = -B_W1Q, .y = G_W1Q} },
                { .leg_id = 4, .pos = {.x = B_W1Q, .y = E_W1Q} },
            },
            .leg_count = 4,
            .speed = SPEED_W1Q,
        },
        {
            .legs = (const leg_phase_data_t[]){
                { .leg_id = 1, .pos = {.x = -A_W1Q, .y = E_W1Q} },
                { .leg_id = 2, .pos = {.x = A_W1Q, .y = E_W1Q} },
                { .leg_id = 3, .pos = {.x = -A_W1Q, .y = E_W1Q} },
                { .leg_id = 4, .pos = {.x = A_W1Q, .y = E_W1Q} },
            },
            .leg_count = 4,
            .speed = SPEED_W1Q,
        },
        {
            .legs = (const leg_phase_data_t[]){
                { .leg_id = 1, .pos = {.x = -B_W1Q, .y = G_W1Q} },
                { .leg_id = 2, .pos = {.x = B_W1Q, .y = E_W1Q} },
                { .leg_id = 3, .pos = {.x = -B_W1Q, .y = E_W1Q} },
                { .leg_id = 4, .pos = {.x = B_W1Q, .y = G_W1Q} },
            },
            .leg_count = 4,
            .speed = SPEED_W1Q,
        },
        {
            .legs = (const leg_phase_data_t[]){
                { .leg_id = 1, .pos = {.x = C_W1Q, .y = F_W1Q} },
                { .leg_id = 2, .pos = {.x = C_W1Q, .y = E_W1Q} },
                { .leg_id = 3, .pos = {.x = C_W1Q, .y = E_W1Q} },
                { .leg_id = 4, .pos = {.x = C_W1Q, .y = F_W1Q} },
            },
            .leg_count = 4,
            .speed = SPEED_W1Q,
        },
        {
            .legs = (const leg_phase_data_t[]){
                { .leg_id = 1, .pos = {.x = B_W1Q, .y = G_W1Q} },
                { .leg_id = 2, .pos = {.x = -B_W1Q, .y = E_W1Q} },
                { .leg_id = 3, .pos = {.x = B_W1Q, .y = E_W1Q} },
                { .leg_id = 4, .pos = {.x = -B_W1Q, .y = G_W1Q} },
            },
            .leg_count = 4,
            .speed = SPEED_W1Q,
        },
    },
    .phase_count = 8,
    .loop = 1,
    .mode = 1,
};

/*==============================================================================
 * 行走步态2 — Front
 *============================================================================*/
/** @name GAIT_Walk_2_Front 坐标常量
 *  @brief 步态相位中腿部的位置参数
 *  @note  X 轴：正值为前进方向，负值为后退方向
 *         Y 轴：数值越小抬得越高（坐标原点在肩关节）
 *         - E  : 支撑相高度，脚着地
 *         - F  : 摆动相高度，脚抬起
 *         - G  : 过渡高度，介于支撑与摆动之间
 *  @{
 */
#define A_W2F   (60)        /**< 全步幅偏移 */
#define B_W2F   (30)        /**< 半步幅偏移 */
#define C_W2F   (0)         /**< 居中位置 */

#define E_W2F   (230.0f)   /**< 支撑相高度 */
#define F_W2F   (170.0f)   /**< 摆动相高度 */
#define G_W2F   (178.8f)   /**< 过渡高度 */

/** @brief 行走步态速度百分比 */
#define SPEED_W2F      (100.0f)    /**< 行走主速度 */
/** @} */

static const motion_gait_t GAIT_Walk_2_Front = {
    .phases = (const motion_phase_t[]){
        {
            .legs = (const leg_phase_data_t[]){
                { .leg_id = 1, .pos = {.x = A_W2F, .y = E_W2F} },
                { .leg_id = 2, .pos = {.x = -A_W2F, .y = E_W2F} },
                { .leg_id = 3, .pos = {.x = A_W2F, .y = E_W2F} },
                { .leg_id = 4, .pos = {.x = -A_W2F, .y = E_W2F} },
            },
            .leg_count = 4,
            .speed = SPEED_W2F,
        },
        {
            .legs = (const leg_phase_data_t[]){
                { .leg_id = 1, .pos = {.x = B_W2F, .y = E_W2F} },
                { .leg_id = 2, .pos = {.x = -B_W2F, .y = G_W2F} },
                { .leg_id = 3, .pos = {.x = B_W2F, .y = G_W2F} },
                { .leg_id = 4, .pos = {.x = -B_W2F, .y = E_W2F} },
            },
            .leg_count = 4,
            .speed = SPEED_W2F,
        },
        {
            .legs = (const leg_phase_data_t[]){
                { .leg_id = 1, .pos = {.x = C_W2F, .y = E_W2F} },
                { .leg_id = 2, .pos = {.x = C_W2F, .y = F_W2F} },
                { .leg_id = 3, .pos = {.x = C_W2F, .y = F_W2F} },
                { .leg_id = 4, .pos = {.x = C_W2F, .y = E_W2F} },
            },
            .leg_count = 4,
            .speed = SPEED_W2F,
        },
        {
            .legs = (const leg_phase_data_t[]){
                { .leg_id = 1, .pos = {.x = -B_W2F, .y = E_W2F} },
                { .leg_id = 2, .pos = {.x = B_W2F, .y = G_W2F} },
                { .leg_id = 3, .pos = {.x = -B_W2F, .y = G_W2F} },
                { .leg_id = 4, .pos = {.x = B_W2F, .y = E_W2F} },
            },
            .leg_count = 4,
            .speed = SPEED_W2F,
        },
        {
            .legs = (const leg_phase_data_t[]){
                { .leg_id = 1, .pos = {.x = -A_W2F, .y = E_W2F} },
                { .leg_id = 2, .pos = {.x = A_W2F, .y = E_W2F} },
                { .leg_id = 3, .pos = {.x = -A_W2F, .y = E_W2F} },
                { .leg_id = 4, .pos = {.x = A_W2F, .y = E_W2F} },
            },
            .leg_count = 4,
            .speed = SPEED_W2F,
        },
        {
            .legs = (const leg_phase_data_t[]){
                { .leg_id = 1, .pos = {.x = -B_W2F, .y = G_W2F} },
                { .leg_id = 2, .pos = {.x = B_W2F, .y = E_W2F} },
                { .leg_id = 3, .pos = {.x = -B_W2F, .y = E_W2F} },
                { .leg_id = 4, .pos = {.x = B_W2F, .y = G_W2F} },
            },
            .leg_count = 4,
            .speed = SPEED_W2F,
        },
        {
            .legs = (const leg_phase_data_t[]){
                { .leg_id = 1, .pos = {.x = C_W2F, .y = F_W2F} },
                { .leg_id = 2, .pos = {.x = C_W2F, .y = E_W2F} },
                { .leg_id = 3, .pos = {.x = C_W2F, .y = E_W2F} },
                { .leg_id = 4, .pos = {.x = C_W2F, .y = F_W2F} },
            },
            .leg_count = 4,
            .speed = SPEED_W2F,
        },
        {
            .legs = (const leg_phase_data_t[]){
                { .leg_id = 1, .pos = {.x = B_W2F, .y = G_W2F} },
                { .leg_id = 2, .pos = {.x = -B_W2F, .y = E_W2F} },
                { .leg_id = 3, .pos = {.x = B_W2F, .y = E_W2F} },
                { .leg_id = 4, .pos = {.x = -B_W2F, .y = G_W2F} },
            },
            .leg_count = 4,
            .speed = SPEED_W2F,
        },
    },
    .phase_count = 8,
    .loop = 1,
    .mode = 1,
};

/*==============================================================================
 * 行走步态2 — Queen
 *============================================================================*/
/** @name GAIT_Walk_2_Queen 坐标常量
 *  @brief 步态相位中腿部的位置参数
 *  @note  X 轴：正值为前进方向，负值为后退方向
 *         Y 轴：数值越小抬得越高（坐标原点在肩关节）
 *         - E  : 支撑相高度，脚着地
 *         - F  : 摆动相高度，脚抬起
 *         - G  : 过渡高度，介于支撑与摆动之间
 *  @{
 */
#define A_W2Q   (-60)        /**< 全步幅偏移 */
#define B_W2Q   (-30)        /**< 半步幅偏移 */
#define C_W2Q   (0)         /**< 居中位置 */

#define E_W2Q   (230.0f)   /**< 支撑相高度 */
#define F_W2Q   (170.0f)   /**< 摆动相高度 */
#define G_W2Q   (178.8f)   /**< 过渡高度 */

/** @brief 行走步态速度百分比 */
#define SPEED_W2Q      (100.0f)    /**< 行走主速度 */
/** @} */

static const motion_gait_t GAIT_Walk_2_Queen = {
    .phases = (const motion_phase_t[]){
        {
            .legs = (const leg_phase_data_t[]){
                { .leg_id = 1, .pos = {.x = A_W2Q, .y = E_W2Q} },
                { .leg_id = 2, .pos = {.x = -A_W2Q, .y = E_W2Q} },
                { .leg_id = 3, .pos = {.x = A_W2Q, .y = E_W2Q} },
                { .leg_id = 4, .pos = {.x = -A_W2Q, .y = E_W2Q} },
            },
            .leg_count = 4,
            .speed = SPEED_W2Q,
        },
        {
            .legs = (const leg_phase_data_t[]){
                { .leg_id = 1, .pos = {.x = B_W2Q, .y = E_W2Q} },
                { .leg_id = 2, .pos = {.x = -B_W2Q, .y = G_W2Q} },
                { .leg_id = 3, .pos = {.x = B_W2Q, .y = G_W2Q} },
                { .leg_id = 4, .pos = {.x = -B_W2Q, .y = E_W2Q} },
            },
            .leg_count = 4,
            .speed = SPEED_W2Q,
        },
        {
            .legs = (const leg_phase_data_t[]){
                { .leg_id = 1, .pos = {.x = C_W2Q, .y = E_W2Q} },
                { .leg_id = 2, .pos = {.x = C_W2Q, .y = F_W2Q} },
                { .leg_id = 3, .pos = {.x = C_W2Q, .y = F_W2Q} },
                { .leg_id = 4, .pos = {.x = C_W2Q, .y = E_W2Q} },
            },
            .leg_count = 4,
            .speed = SPEED_W2Q,
        },
        {
            .legs = (const leg_phase_data_t[]){
                { .leg_id = 1, .pos = {.x = -B_W2Q, .y = E_W2Q} },
                { .leg_id = 2, .pos = {.x = B_W2Q, .y = G_W2Q} },
                { .leg_id = 3, .pos = {.x = -B_W2Q, .y = G_W2Q} },
                { .leg_id = 4, .pos = {.x = B_W2Q, .y = E_W2Q} },
            },
            .leg_count = 4,
            .speed = SPEED_W2Q,
        },
        {
            .legs = (const leg_phase_data_t[]){
                { .leg_id = 1, .pos = {.x = -A_W2Q, .y = E_W2Q} },
                { .leg_id = 2, .pos = {.x = A_W2Q, .y = E_W2Q} },
                { .leg_id = 3, .pos = {.x = -A_W2Q, .y = E_W2Q} },
                { .leg_id = 4, .pos = {.x = A_W2Q, .y = E_W2Q} },
            },
            .leg_count = 4,
            .speed = SPEED_W2Q,
        },
        {
            .legs = (const leg_phase_data_t[]){
                { .leg_id = 1, .pos = {.x = -B_W2Q, .y = G_W2Q} },
                { .leg_id = 2, .pos = {.x = B_W2Q, .y = E_W2Q} },
                { .leg_id = 3, .pos = {.x = -B_W2Q, .y = E_W2Q} },
                { .leg_id = 4, .pos = {.x = B_W2Q, .y = G_W2Q} },
            },
            .leg_count = 4,
            .speed = SPEED_W2Q,
        },
        {
            .legs = (const leg_phase_data_t[]){
                { .leg_id = 1, .pos = {.x = C_W2Q, .y = F_W2Q} },
                { .leg_id = 2, .pos = {.x = C_W2Q, .y = E_W2Q} },
                { .leg_id = 3, .pos = {.x = C_W2Q, .y = E_W2Q} },
                { .leg_id = 4, .pos = {.x = C_W2Q, .y = F_W2Q} },
            },
            .leg_count = 4,
            .speed = SPEED_W2Q,
        },
        {
            .legs = (const leg_phase_data_t[]){
                { .leg_id = 1, .pos = {.x = B_W2Q, .y = G_W2Q} },
                { .leg_id = 2, .pos = {.x = -B_W2Q, .y = E_W2Q} },
                { .leg_id = 3, .pos = {.x = B_W2Q, .y = E_W2Q} },
                { .leg_id = 4, .pos = {.x = -B_W2Q, .y = G_W2Q} },
            },
            .leg_count = 4,
            .speed = SPEED_W2Q,
        },
    },
    .phase_count = 8,
    .loop = 1,
    .mode = 1,
};

#endif /* __APP_GAIT_SEQ_H__ */
