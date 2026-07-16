/**
 * @brief 步态序列数据
 * @author 李嘉图
 * @date 2026-06-30
 */

#ifndef __APP_GAIT_SEQ_H__
#define __APP_GAIT_SEQ_H__

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 功能包含
#include "app_motion_scheduler.h"
#include "app_motion_ik.h"

/*==============================================================================
 * 站立
 *============================================================================*/
/** @name GAIT_Stand 坐标常量
 *  @brief 站立步态的腿部支撑高度参数
 *  @note  Y 轴：数值越小抬得越高（坐标原点在肩关节）
 *  @{
 */
#define STAND_HEIGHT   (160)    /**< 站立支撑高度 */
/** @} */

static const motion_gait_t GAIT_Stand = {
    .phases = (const motion_phase_t[]){
        {
            .legs = (const leg_phase_data_t[]){
                { .leg_id = 1, .pos = {.x = 0, .y = STAND_HEIGHT} },
                { .leg_id = 2, .pos = {.x = 0, .y = STAND_HEIGHT} },
                { .leg_id = 3, .pos = {.x = 0, .y = STAND_HEIGHT} },
                { .leg_id = 4, .pos = {.x = 0, .y = STAND_HEIGHT} },
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
 * 竖直站立
 *============================================================================*/
/** @name GAIT_Vertical 坐标常量
 *  @brief 竖直站立步态的腿部支撑高度参数
 *  @note  Y 轴：数值越小抬得越高（坐标原点在肩关节）
 *  @{
 */
#define VERTICAL_HEIGHT   (280)    /**< 竖直站立支撑高度 */
/** @} */

static const motion_gait_t GAIT_Vertical = {
    .phases = (const motion_phase_t[]){
        {
            .legs = (const leg_phase_data_t[]){
                { .leg_id = 1, .pos = {.x = 0, .y = VERTICAL_HEIGHT} },
                { .leg_id = 2, .pos = {.x = 0, .y = VERTICAL_HEIGHT} },
                { .leg_id = 3, .pos = {.x = 0, .y = VERTICAL_HEIGHT} },
                { .leg_id = 4, .pos = {.x = 0, .y = VERTICAL_HEIGHT} },
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
 * 趴伏
 *============================================================================*/
/** @name GAIT_Grovel 坐标常量
 *  @brief 趴伏步态的腿部支撑高度参数
 *  @note  Y 轴：数值越小抬得越高（坐标原点在肩关节）
 *  @{
 */
#define GROVEL_HEIGHT   (100)    /**< 趴伏支撑高度 */
/** @} */

static const motion_gait_t GAIT_Grovel = {
    .phases = (const motion_phase_t[]){
        {
            .legs = (const leg_phase_data_t[]){
                { .leg_id = 1, .pos = {.x = 0, .y = GROVEL_HEIGHT} },
                { .leg_id = 2, .pos = {.x = 0, .y = GROVEL_HEIGHT} },
                { .leg_id = 3, .pos = {.x = 0, .y = GROVEL_HEIGHT} },
                { .leg_id = 4, .pos = {.x = 0, .y = GROVEL_HEIGHT} },
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
 * 踱步1
 *============================================================================*/
/** @name GAIT_Trot_1 坐标常量
 *  @brief 踱步步态相位中腿部的高度参数
 *  @note  Y 轴：数值越小抬得越高（坐标原点在肩关节）
 *         - TROT_SWING  : 摆动相高度，对角腿抬起
 *         - TROT_STANCE : 支撑相高度，对角腿着地
 *  @{
 */
#define TROT_SWING_1     (90)    /**< 踱步摆动相高度 */
#define TROT_STANCE_1    (160)    /**< 踱步支撑相高度 */

/** @brief 踱步步态速度百分比 */
#define SPEED_TROT_1     (100.0f) /**< 踱步主速度 */
/** @} */

static const motion_gait_t GAIT_Trot_1 = {
    .phases = (const motion_phase_t[]){
        {
            .legs = (const leg_phase_data_t[]){
                { .leg_id = 1, .pos = {.x = 0, .y = TROT_SWING_1} },
                { .leg_id = 2, .pos = {.x = 0, .y = TROT_STANCE_1} },
                { .leg_id = 3, .pos = {.x = 0, .y = TROT_STANCE_1} },
                { .leg_id = 4, .pos = {.x = 0, .y = TROT_SWING_1} },
            },
            .leg_count = 4,
            .speed = SPEED_TROT_1,
        },
        {
            .legs = (const leg_phase_data_t[]){
                { .leg_id = 1, .pos = {.x = 0, .y = TROT_STANCE_1} },
                { .leg_id = 2, .pos = {.x = 0, .y = TROT_STANCE_1} },
                { .leg_id = 3, .pos = {.x = 0, .y = TROT_STANCE_1} },
                { .leg_id = 4, .pos = {.x = 0, .y = TROT_STANCE_1} },
            },
            .leg_count = 4,
            .speed = SPEED_TROT_1,
        },
        {
            .legs = (const leg_phase_data_t[]){
                { .leg_id = 1, .pos = {.x = 0, .y = TROT_STANCE_1} },
                { .leg_id = 2, .pos = {.x = 0, .y = TROT_SWING_1} },
                { .leg_id = 3, .pos = {.x = 0, .y = TROT_SWING_1} },
                { .leg_id = 4, .pos = {.x = 0, .y = TROT_STANCE_1} },
            },
            .leg_count = 4,
            .speed = SPEED_TROT_1,
        },
        {
            .legs = (const leg_phase_data_t[]){
                { .leg_id = 1, .pos = {.x = 0, .y = TROT_STANCE_1} },
                { .leg_id = 2, .pos = {.x = 0, .y = TROT_STANCE_1} },
                { .leg_id = 3, .pos = {.x = 0, .y = TROT_STANCE_1} },
                { .leg_id = 4, .pos = {.x = 0, .y = TROT_STANCE_1} },
            },
            .leg_count = 4,
            .speed = SPEED_TROT_1,
        },
    },
    .phase_count = 4,
    .loop = 1,
    .mode = 1,
};

/*==============================================================================
 * 踱步2
 *============================================================================*/
/** @name GAIT_Trot_2 坐标常量
 *  @brief 踱步步态相位中腿部的高度参数
 *  @note  Y 轴：数值越小抬得越高（坐标原点在肩关节）
 *         - TROT_SWING  : 摆动相高度，对角腿抬起
 *         - TROT_STANCE : 支撑相高度，对角腿着地
 *  @{
 */
#define TROT_SWING_2     (90)    /**< 踱步摆动相高度 */
#define TROT_STANCE_2    (160)    /**< 踱步支撑相高度 */

/** @brief 踱步步态速度百分比 */
#define SPEED_TROT_2     (100.0f) /**< 踱步主速度 */
/** @} */

static const motion_gait_t GAIT_Trot_2 = {
    .phases = (const motion_phase_t[]){
        {
            .legs = (const leg_phase_data_t[]){
                { .leg_id = 1, .pos = {.x = 0, .y = TROT_SWING_2} },
                { .leg_id = 2, .pos = {.x = 0, .y = TROT_STANCE_2} },
                { .leg_id = 3, .pos = {.x = 0, .y = TROT_STANCE_2} },
                { .leg_id = 4, .pos = {.x = 0, .y = TROT_SWING_2} },
            },
            .leg_count = 4,
            .speed = SPEED_TROT_2,
        },
        {
            .legs = (const leg_phase_data_t[]){
                { .leg_id = 1, .pos = {.x = 0, .y = TROT_STANCE_2} },
                { .leg_id = 2, .pos = {.x = 0, .y = TROT_STANCE_2} },
                { .leg_id = 3, .pos = {.x = 0, .y = TROT_STANCE_2} },
                { .leg_id = 4, .pos = {.x = 0, .y = TROT_STANCE_2} },
            },
            .leg_count = 4,
            .speed = SPEED_TROT_2,
        },
        {
            .legs = (const leg_phase_data_t[]){
                { .leg_id = 1, .pos = {.x = 0, .y = TROT_STANCE_2} },
                { .leg_id = 2, .pos = {.x = 0, .y = TROT_SWING_2} },
                { .leg_id = 3, .pos = {.x = 0, .y = TROT_SWING_2} },
                { .leg_id = 4, .pos = {.x = 0, .y = TROT_STANCE_2} },
            },
            .leg_count = 4,
            .speed = SPEED_TROT_2,
        },
        {
            .legs = (const leg_phase_data_t[]){
                { .leg_id = 1, .pos = {.x = 0, .y = TROT_STANCE_2} },
                { .leg_id = 2, .pos = {.x = 0, .y = TROT_STANCE_2} },
                { .leg_id = 3, .pos = {.x = 0, .y = TROT_STANCE_2} },
                { .leg_id = 4, .pos = {.x = 0, .y = TROT_STANCE_2} },
            },
            .leg_count = 4,
            .speed = SPEED_TROT_2,
        },
    },
    .phase_count = 4,
    .loop = 1,
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
#define A_W1F   (40)        /**< 全步幅偏移 */
#define B_W1F   (20)        /**< 半步幅偏移 */
#define C_W1F   (0)         /**< 居中位置 */

#define E_W1F   (160.0f)   /**< 支撑相高度 */
#define F_W1F   (100.0f)   /**< 摆动相高度 */
#define G_W1F   (118.8f)   /**< 过渡高度 */

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
#define A_W1Q   (-40)        /**< 全步幅偏移 */
#define B_W1Q   (-20)        /**< 半步幅偏移 */
#define C_W1Q   (0)         /**< 居中位置 */

#define E_W1Q   (160.0f)   /**< 支撑相高度 */
#define F_W1Q   (100.0f)   /**< 摆动相高度 */
#define G_W1Q   (118.8f)   /**< 过渡高度 */

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

#define E_W2F   (200.0f)   /**< 支撑相高度（从 230 降低以减少腿部伸展量，增大电机力矩余量） */
#define F_W2F   (130.0f)   /**< 摆动相高度（从 170 降低以增大抬腿量至 70） */
#define G_W2F   (142.0f)   /**< 过渡高度（从 178.8 降低以增大过渡抬腿量至 58） */

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

/*==============================================================================
 * 行走步态3 — Front（优化参数）
 *============================================================================*/
/** @name GAIT_Walk_3_Front 坐标常量
 *  @brief 步态相位中腿部的位置参数（低抬腿、低速、省力矩版）
 *  @note  X 轴：正值为前进方向，负值为后退方向
 *         Y 轴：数值越小抬得越高（坐标原点在肩关节）
 *         - E  : 支撑相高度，脚着地
 *         - F  : 摆动相高度，脚抬起（抬腿 50 单位，比 W2F 减少 30% 扭矩）
 *         - G  : 过渡高度，介于支撑与摆动之间
 *  @{
 */
#define A_W3F   (50)        /**< 全步幅偏移 */
#define B_W3F   (25)        /**< 半步幅偏移 */
#define C_W3F   (0)         /**< 居中位置 */

#define E_W3F   (210.0f)   /**< 支撑相高度 */
#define F_W3F   (150.0f)   /**< 摆动相高度 */
#define G_W3F   (158.0f)   /**< 过渡高度 */

/** @brief 行走步态速度百分比 */
#define SPEED_W3F      (100.0f)
/** @} */

static const motion_gait_t GAIT_Walk_3_Front = {
    .phases = (const motion_phase_t[]){
        {
            .legs = (const leg_phase_data_t[]){
                { .leg_id = 1, .pos = {.x = A_W3F, .y = E_W3F} },
                { .leg_id = 2, .pos = {.x = -A_W3F, .y = E_W3F} },
                { .leg_id = 3, .pos = {.x = A_W3F, .y = E_W3F} },
                { .leg_id = 4, .pos = {.x = -A_W3F, .y = E_W3F} },
            },
            .leg_count = 4,
            .speed = SPEED_W3F,
        },
        {
            .legs = (const leg_phase_data_t[]){
                { .leg_id = 1, .pos = {.x = B_W3F, .y = E_W3F} },
                { .leg_id = 2, .pos = {.x = -B_W3F, .y = G_W3F} },
                { .leg_id = 3, .pos = {.x = B_W3F, .y = G_W3F} },
                { .leg_id = 4, .pos = {.x = -B_W3F, .y = E_W3F} },
            },
            .leg_count = 4,
            .speed = SPEED_W3F,
        },
        {
            .legs = (const leg_phase_data_t[]){
                { .leg_id = 1, .pos = {.x = C_W3F, .y = E_W3F} },
                { .leg_id = 2, .pos = {.x = C_W3F, .y = F_W3F} },
                { .leg_id = 3, .pos = {.x = C_W3F, .y = F_W3F} },
                { .leg_id = 4, .pos = {.x = C_W3F, .y = E_W3F} },
            },
            .leg_count = 4,
            .speed = SPEED_W3F,
        },
        {
            .legs = (const leg_phase_data_t[]){
                { .leg_id = 1, .pos = {.x = -B_W3F, .y = E_W3F} },
                { .leg_id = 2, .pos = {.x = B_W3F, .y = G_W3F} },
                { .leg_id = 3, .pos = {.x = -B_W3F, .y = G_W3F} },
                { .leg_id = 4, .pos = {.x = B_W3F, .y = E_W3F} },
            },
            .leg_count = 4,
            .speed = SPEED_W3F,
        },
        {
            .legs = (const leg_phase_data_t[]){
                { .leg_id = 1, .pos = {.x = -A_W3F, .y = E_W3F} },
                { .leg_id = 2, .pos = {.x = A_W3F, .y = E_W3F} },
                { .leg_id = 3, .pos = {.x = -A_W3F, .y = E_W3F} },
                { .leg_id = 4, .pos = {.x = A_W3F, .y = E_W3F} },
            },
            .leg_count = 4,
            .speed = SPEED_W3F,
        },
        {
            .legs = (const leg_phase_data_t[]){
                { .leg_id = 1, .pos = {.x = -B_W3F, .y = G_W3F} },
                { .leg_id = 2, .pos = {.x = B_W3F, .y = E_W3F} },
                { .leg_id = 3, .pos = {.x = -B_W3F, .y = E_W3F} },
                { .leg_id = 4, .pos = {.x = B_W3F, .y = G_W3F} },
            },
            .leg_count = 4,
            .speed = SPEED_W3F,
        },
        {
            .legs = (const leg_phase_data_t[]){
                { .leg_id = 1, .pos = {.x = C_W3F, .y = F_W3F} },
                { .leg_id = 2, .pos = {.x = C_W3F, .y = E_W3F} },
                { .leg_id = 3, .pos = {.x = C_W3F, .y = E_W3F} },
                { .leg_id = 4, .pos = {.x = C_W3F, .y = F_W3F} },
            },
            .leg_count = 4,
            .speed = SPEED_W3F,
        },
        {
            .legs = (const leg_phase_data_t[]){
                { .leg_id = 1, .pos = {.x = B_W3F, .y = G_W3F} },
                { .leg_id = 2, .pos = {.x = -B_W3F, .y = E_W3F} },
                { .leg_id = 3, .pos = {.x = B_W3F, .y = E_W3F} },
                { .leg_id = 4, .pos = {.x = -B_W3F, .y = G_W3F} },
            },
            .leg_count = 4,
            .speed = SPEED_W3F,
        },
    },
    .phase_count = 8,
    .loop = 1,
    .mode = 1,
};

/*==============================================================================
 * 行走步态3 — Queen（优化参数，后退）
 *============================================================================*/
/** @name GAIT_Walk_3_Queen 坐标常量
 *  @brief 步态相位中腿部的位置参数（低抬腿、低速、省力矩版，后退）
 *  @note  X 轴：负值为后退方向（与 Front 符号相反）
 *         Y 轴：数值越小抬得越高（坐标原点在肩关节）
 *         - E  : 支撑相高度，脚着地
 *         - F  : 摆动相高度，脚抬起（抬腿 50 单位，降低后腿扭矩需求）
 *         - G  : 过渡高度，介于支撑与摆动之间
 *  @{
 */
#define A_W3Q   (-50)        /**< 全步幅偏移（后退方向） */
#define B_W3Q   (-25)        /**< 半步幅偏移（后退方向） */
#define C_W3Q   (0)         /**< 居中位置 */

#define E_W3Q   (210.0f)   /**< 支撑相高度（71% 最大腿长） */
#define F_W3Q   (135.0f)   /**< 摆动相高度（抬腿 50 单位，降低后腿扭矩需求） */
#define G_W3Q   (165.0f)   /**< 过渡高度（抬腿 30 单位，平缓过渡） */

/** @brief 行走步态速度百分比 */
#define SPEED_W3Q      (100.0f)    /**< 行走主速度（降速减少峰值电流） */
/** @} */

static const motion_gait_t GAIT_Walk_3_Queen = {
    .phases = (const motion_phase_t[]){
        {
            .legs = (const leg_phase_data_t[]){
                { .leg_id = 1, .pos = {.x = A_W3Q, .y = E_W3Q} },
                { .leg_id = 2, .pos = {.x = -A_W3Q, .y = E_W3Q} },
                { .leg_id = 3, .pos = {.x = A_W3Q, .y = E_W3Q} },
                { .leg_id = 4, .pos = {.x = -A_W3Q, .y = E_W3Q} },
            },
            .leg_count = 4,
            .speed = SPEED_W3Q,
        },
        {
            .legs = (const leg_phase_data_t[]){
                { .leg_id = 1, .pos = {.x = B_W3Q, .y = E_W3Q} },
                { .leg_id = 2, .pos = {.x = -B_W3Q, .y = G_W3Q} },
                { .leg_id = 3, .pos = {.x = B_W3Q, .y = G_W3Q} },
                { .leg_id = 4, .pos = {.x = -B_W3Q, .y = E_W3Q} },
            },
            .leg_count = 4,
            .speed = SPEED_W3Q,
        },
        {
            .legs = (const leg_phase_data_t[]){
                { .leg_id = 1, .pos = {.x = C_W3Q, .y = E_W3Q} },
                { .leg_id = 2, .pos = {.x = C_W3Q, .y = F_W3Q} },
                { .leg_id = 3, .pos = {.x = C_W3Q, .y = F_W3Q} },
                { .leg_id = 4, .pos = {.x = C_W3Q, .y = E_W3Q} },
            },
            .leg_count = 4,
            .speed = SPEED_W3Q,
        },
        {
            .legs = (const leg_phase_data_t[]){
                { .leg_id = 1, .pos = {.x = -B_W3Q, .y = E_W3Q} },
                { .leg_id = 2, .pos = {.x = B_W3Q, .y = G_W3Q} },
                { .leg_id = 3, .pos = {.x = -B_W3Q, .y = G_W3Q} },
                { .leg_id = 4, .pos = {.x = B_W3Q, .y = E_W3Q} },
            },
            .leg_count = 4,
            .speed = SPEED_W3Q,
        },
        {
            .legs = (const leg_phase_data_t[]){
                { .leg_id = 1, .pos = {.x = -A_W3Q, .y = E_W3Q} },
                { .leg_id = 2, .pos = {.x = A_W3Q, .y = E_W3Q} },
                { .leg_id = 3, .pos = {.x = -A_W3Q, .y = E_W3Q} },
                { .leg_id = 4, .pos = {.x = A_W3Q, .y = E_W3Q} },
            },
            .leg_count = 4,
            .speed = SPEED_W3Q,
        },
        {
            .legs = (const leg_phase_data_t[]){
                { .leg_id = 1, .pos = {.x = -B_W3Q, .y = G_W3Q} },
                { .leg_id = 2, .pos = {.x = B_W3Q, .y = E_W3Q} },
                { .leg_id = 3, .pos = {.x = -B_W3Q, .y = E_W3Q} },
                { .leg_id = 4, .pos = {.x = B_W3Q, .y = G_W3Q} },
            },
            .leg_count = 4,
            .speed = SPEED_W3Q,
        },
        {
            .legs = (const leg_phase_data_t[]){
                { .leg_id = 1, .pos = {.x = C_W3Q, .y = F_W3Q} },
                { .leg_id = 2, .pos = {.x = C_W3Q, .y = E_W3Q} },
                { .leg_id = 3, .pos = {.x = C_W3Q, .y = E_W3Q} },
                { .leg_id = 4, .pos = {.x = C_W3Q, .y = F_W3Q} },
            },
            .leg_count = 4,
            .speed = SPEED_W3Q,
        },
        {
            .legs = (const leg_phase_data_t[]){
                { .leg_id = 1, .pos = {.x = B_W3Q, .y = G_W3Q} },
                { .leg_id = 2, .pos = {.x = -B_W3Q, .y = E_W3Q} },
                { .leg_id = 3, .pos = {.x = B_W3Q, .y = E_W3Q} },
                { .leg_id = 4, .pos = {.x = -B_W3Q, .y = G_W3Q} },
            },
            .leg_count = 4,
            .speed = SPEED_W3Q,
        },
    },
    .phase_count = 8,
    .loop = 1,
    .mode = 1,
};

#endif /* __APP_GAIT_SEQ_H__ */
