/**
 * @file ik_two_link.h
 * @brief 二连杆逆运动学解算
 * @author 李嘉图
 * @date 2026-06-11
 *
 * @note 输入足端在髋关节坐标系下的模型坐标 (x, y)，
 *       输出髋/膝关节角度（弧度）。
 *       坐标系：原点在髋关节，+x 前，+y 下（垂直身体平面）。
 *       注意：x 是模型内部坐标（含髋膝耦合分量），不代表物理前后偏移。
 *       标定：servo(0,90)↔(0,260), servo(-77,-63)↔(-57.5,27.4)
 */

#ifndef __IK_TWO_LINK_H__
#define __IK_TWO_LINK_H__

#include <stdint.h>

#define RAD_TO_DEG     57.29577951308232f
#define DEG_TO_RAD     0.017453292519943f

uint8_t IK_TwoLink(float L1, float L2, float x, float y,
                   float *hip, float *knee);

#endif /* __IK_TWO_LINK_H__ */
