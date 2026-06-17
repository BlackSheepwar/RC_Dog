/**
 * @file ik_two_link.h
 * @brief 二连杆逆运动学解算
 * @author 李嘉图
 * @date 2026-06-11
 *
 * @note 输入足端在髋关节坐标系下的 (x, z) 坐标，
 *       输出髋/膝关节角度（弧度）。
 *       适用模型：髋关节在原点，膝关节在 L1 末端，足端在 L2 末端。
 *       x = 前后（+前），z = 竖直（+下，远离髋关节）。
 */

#ifndef __IK_TWO_LINK_H__
#define __IK_TWO_LINK_H__

#include <stdint.h>

#define RAD_TO_DEG     57.29577951308232f   /* 弧度 → 度 */
#define DEG_TO_RAD     0.017453292519943f   /* 度 → 弧度 */

/**
 * @brief 二连杆逆运动学
 * @param L1    大腿连杆长度（mm）
 * @param L2    小腿连杆长度（mm）
 * @param x     足端 x 坐标（前后，+前）
 * @param z     足端 z 坐标（竖直，+下）
 * @param hip   [out] 髋关节角（弧度）
 * @param knee  [out] 膝关节角（弧度）
 * @retval 1 可达，角度有效
 * @retval 0 不可达（超出工作空间 | 奇异）
 */
uint8_t IK_TwoLink(float L1, float L2, float x, float z,
                   float *hip, float *knee);

#endif /* __IK_TWO_LINK_H__ */
