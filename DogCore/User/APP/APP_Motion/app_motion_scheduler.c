/**
 * @file app_motion_scheduler.c
 * @brief 周期调度层实现
 * @author Ricardo
 * @date 2026-06-29
 *
 * @note 核心功能：
 *       - 维护当前步态和相位状态
 *       - 角度到达检测推进相位
 *       - 调用Motion层执行角度控制（百分比速度）
 *
 * 调用顺序：
 *       1. APP_Motion_Sched_Init()       - 初始化
 *       2. APP_Motion_Sched_Start(gait)  - 启动步态（立即应用相位0）
 *       3. APP_Motion_Sched_Scheduler()  - 每10ms调用（推进相位）
 */

#include <stddef.h>

#include "app_motion_scheduler.h"
// 功能包含
#include "app_motion.h"      /* 腿级：角度映射与同步配速 */
#include "common_ik.h"       /* 纯算法：二连杆逆解 */
#include "bsp_sys.h"
#include "app_gait_seq.h"

/*==============================================================================
 * 调度器内部状态
 *============================================================================*/
/**
 * @brief 步态调度器的运行时状态
 * @details
 *       gait:              当前执行的步态序列指针
 *       phase_index:       当前相位索引
 *       phase_start_tick:  当前相位的起始时刻(ms)
 *       active:            1=步态运行中, 0=空闲
 */
typedef struct {
    const motion_gait_t *gait;      /* 当前步态 */
    uint16_t phase_index;           /* 当前相位索引 */
    uint32_t phase_start_tick;      /* 相位起始时刻(ms) */
    uint8_t active;                 /* 1=正在执行步态 */
} sched_state_t;

/** @brief 全局调度器状态实例 */
static sched_state_t s_sched = {0};

/*==============================================================================
 * 前置声明
 *============================================================================*/
/**
 * @brief 应用单个相位
 * @param phase_index 相位索引
 */
static void APP_Motion_Sched_ApplyPhase(uint16_t phase_index);

/*==============================================================================
 * 初始化函数
 *============================================================================*/
/**
 * @brief 初始化调度器
 * @details
 *       - 清零步态指针
 *       - 重置相位索引为0
 *       - 标记为非活跃状态(active=0)
 * @note 必须在Motion_Init之后调用
 */
void APP_Motion_Sched_Init(void)
{
    s_sched.gait = NULL;
    s_sched.phase_index = 0;
    s_sched.phase_start_tick = 0;
    s_sched.active = 0;
}

/*==============================================================================
 * 步态控制函数
 *============================================================================*/
/**
 * @brief 启动一个步态序列
 * @param gait 步态数据指针
 *
 * @details
 *       1. 参数检查（非NULL、至少1个相位）
 *       2. 保存步态指针
 *       3. 重置相位索引为0
 *       4. 记录起始时刻
 *       5. 标记为活跃(active=1)
 *       6. 立即应用第一个相位（无延迟）
 *
 * @warning gait不能为NULL，phase_count必须>0
 * @note 立即应用相位0是关键，确保收到命令立即响应
 */
void APP_Motion_Sched_Start(const motion_gait_t *gait)
{
    if (gait == NULL || gait->phase_count == 0)
        return;

    s_sched.gait = gait;
    s_sched.phase_index = 0;
    s_sched.phase_start_tick = BSP_SYS_Get_Tick_Ms();
    s_sched.active = 1;

    /* ── 立即应用第一个相位（无延迟） ── */
    APP_Motion_Sched_ApplyPhase(0);
}

/**
 * @brief 停止当前步态
 * @details
 *       - 清零active标记
 *       - 调用Motion_StopAll停止所有四条腿
 * @note 腿会保持当前角度，下次SetTarget会从这里开始插值
 */
void APP_Motion_Sched_Stop(void)
{
    s_sched.active = 0;
    APP_Motion_Stop_All();
}

/**
 * @brief 查询是否有步态正在执行
 * @retval 1: 步态运行中
 * @retval 0: 停止或空闲
 */
uint8_t APP_Motion_Sched_Is_Running(void)
{
    return s_sched.active;
}

/*==============================================================================
 * 相位应用
 *============================================================================*/
/**
 * @brief 把一个相位的目标下发给各条腿
 * @param phase_index 相位索引，必须 < gait->phase_count
 * @retval 无
 *
 * @details
 *       按 gait->mode 分两种：
 *         APP_MOTION_MODE_ANGLE：直接用 leg->angle（度），不做任何换算；
 *         APP_MOTION_MODE_POS  ：leg->pos 是足端坐标，走 Common_Ik 解算，
 *                                再按 APP_MOTION_IK_SOLUTION 挑一组解。
 *
 * @note **符号约定**：步态表的 pos.y 是"足端向下伸展的距离"（正值），
 *       而 Common_Ik 的数学系是 +y 上。本机的舵机安装表（calibration = −90、
 *       post_reverse = 1）是按"向下为正"整定的，两者正好抵消——
 *       所以这里直接把 +pos.y 送进解算，不要再取反一次。
 *       如果哪天发现腿朝反方向动，先查舵机安装表，别急着改这里。
 *
 *       解算不可达（足端超出工作空间）时跳过该腿，保持当前姿态，
 *       宁可不动也不要让舵机乱转。
 */
static void APP_Motion_Sched_ApplyPhase(uint16_t phase_index)
{
    const motion_gait_t  *gait  = s_sched.gait;
    const motion_phase_t *phase;

    if ((gait == NULL) || (phase_index >= gait->phase_count)) {
        return;
    }

    phase = &gait->phases[phase_index];

    for (uint8_t i = 0; i < phase->leg_count; i++) {
        const leg_phase_data_t *leg = &phase->legs[i];
        float                   target[APP_MOTION_JOINTS_PER_LEG];

        if (gait->mode == APP_MOTION_MODE_ANGLE) {
            /* ---------- 角度直通 ---------- */
            target[0] = leg->angle.hip;
            target[1] = leg->angle.knee;
        } else {
            /* ---------- 足端坐标 → 逆解 ---------- */
            common_ik_solution_t    solution;
            const common_ik_link_t *link = APP_Motion_Get_Link(leg->leg_id);
            uint8_t                 pick = APP_MOTION_IK_SOLUTION;

            if ((link == NULL) || (Common_Ik_Solve(link, leg->pos.x, leg->pos.y,
                                                   &solution) <= 0)) {
                continue;   /* 腿号非法或不可达：保持当前姿态 */
            }

            /* 边界奇异时只有一组解，别越界取第二组 */
            if (pick >= solution.count) {
                pick = 0u;
            }

            target[0] = solution.hip[pick];
            target[1] = solution.knee[pick];
        }

        APP_Motion_Set_Target(leg->leg_id, target, phase->speed);
    }
}

/*==============================================================================
 * 调度器主函数
 *============================================================================*/
/**
 * @brief 运动周期调度器（每10ms调用一次）
 *
 * @details
 *       执行流程：
 *       1. 先调用Motion_Scheduler()更新各腿关节到达状态
 *       2. 若无活跃步态，直接返回
 *       3. 检查当前相位所有腿是否已完成（APP_Motion_Is_Moving == 0）
 *       4. 若全部完成：
 *          a. phase_index++ 推进到下一相位
 *          b. 检查是否超过总相位数
 *             - 若超过且loop=1：重置为0（循环）
 *             - 若超过且loop=0：标记完成(active=0)，返回
 *          c. 应用新相位
 *       5. 若腿未完成，什么都不做（等待下一 tick 再次检测）
 *
 * @note
 *       - 必须由Task_Motion每10ms调用
 *       - APP_Motion_Scheduler 负责读取舵机位置、更新 current_angle 和 reached 标记
 *       - 本函数检测到所有腿 completed 后立即推进相位
 */
void APP_Motion_Sched_Scheduler(void)
{
    /* ── 先Motion底层调度（更新关节到达状态） ── */
    APP_Motion_Scheduler();

    /* ── 若无活跃步态，直接返回 ── */
    if (!s_sched.active || s_sched.gait == NULL)
        return;

    /* ── 获取当前相位 ── */
    const motion_phase_t *phase = &s_sched.gait->phases[s_sched.phase_index];
    uint32_t now = BSP_SYS_Get_Tick_Ms();
    uint32_t elapsed = now - s_sched.phase_start_tick;

    /* ── 检查完成状态 + 超时兜底 ── */
    uint8_t all_done = 1;
    uint8_t timed_out = 0;

    for (uint8_t i = 0; i < phase->leg_count; i++)
    {
        if (APP_Motion_Is_Moving(phase->legs[i].leg_id))
        {
            if (elapsed >= APP_MOTION_PHASE_TIMEOUT_MS)
            {
                /* 超时 → 强停该腿并继续推进 */
                timed_out = 1;
                APP_Motion_Stop(phase->legs[i].leg_id);
            }
            else
            {
                all_done = 0;
            }
        }
    }

    if (all_done || timed_out)
    {
        /* 推进到下一相位 */
        s_sched.phase_index++;

        /* 检查循环 */
        if (s_sched.phase_index >= s_sched.gait->phase_count)
        {
            if (s_sched.gait->loop)
                s_sched.phase_index = 0;  /* 从头循环 */
            else
            {
                s_sched.active = 0;       /* 步态结束 */
                return;
            }
        }

        /* 应用新相位 */
        APP_Motion_Sched_ApplyPhase(s_sched.phase_index);
        s_sched.phase_start_tick = BSP_SYS_Get_Tick_Ms();
    }
}
