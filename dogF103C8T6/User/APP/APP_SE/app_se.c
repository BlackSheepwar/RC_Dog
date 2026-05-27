/**
 * @file app_se.c
 * @brief 舵机控制
 * @author 李嘉图
 * @date 2026-5-12
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
#include "app_se.h"

/*==============================================================================
 * 舵机静态池
 *============================================================================*/
static APP_SE_t app_se_pool[SE_MAX_SIZE];
static uint8_t app_se_count = 0;

/**
 * @brief 根据舵机编号 查找 APP_SE实例
 * @param id 舵机编号
 * @retval 非NULL：返回对应舵机实例结构体指针
 * @retval NULL：未找到
 */
static APP_SE_t *APP_SE_GetById(uint8_t id)
{
    for (uint8_t i = 0; i < app_se_count; i++)
    {
        if (app_se_pool[i].id == id)
        {
            return &app_se_pool[i];
        }
    }
    return NULL;
}
/*==============================================================================
 * 初始化函数
 *============================================================================*/
/**
 * @brief 初始化APP控制系统
 */
void APP_SE_Init(void)
{
    BSP_SE_Init();
    app_se_count = 0;
}

/**
 * @brief 注册一个舵机实例到APP控制系统
 * @param id         舵机编号
 * @param htim       舵机定时器句柄
 * @param Channel    舵机通道
 * @param offset     舵机角度偏移量（-25°~25°）
 * @param init_angle 初始角度
 * @param speed      运动步进速度（1°~10°）
 * @retval 1: 注册成功
 * @retval 0: 注册失败
 */
uint8_t APP_SE_Add(uint8_t id,  TIM_HandleTypeDef *htim, 
uint32_t Channel, int16_t offset, int16_t init_angle, uint8_t speed)
{
    if (app_se_count >= SE_MAX_SIZE)
        return 0;

    /* 防止重复ID */
    if (APP_SE_GetById(id) != NULL)
        return 0;

    // 注册舵机通道
    uint8_t ret = BSP_SE_RegisterPort(id, htim, Channel);
    if (ret == 0)return 0;

    APP_SE_t *se = &app_se_pool[app_se_count];

    se->id = id;
    se->offset = offset;
    se->current_angle = init_angle;
    se->target_angle = init_angle;
    se->speed = (speed == 0) ? 1 : speed;
    se->enable = 1;

    // 启动舵机定时器
    BSP_SE_Start(htim, Channel);
    // 重置舵机角度
    BSP_SE_Reset(htim);
    // 输出初始角度
    BSP_SE_300Angle(se->id,
                se->current_angle + se->offset + 150);

    app_se_count++;

    return 1;
}

/**
 * @brief 设置舵机目标角度
 * @param id    舵机编号
 * @param angle 目标角度（-125~125）
 * @retval 无
 */
void APP_SE_SetTarget(uint8_t id, int16_t angle)
{
    /* 查找舵机实例 */
    APP_SE_t *se = APP_SE_GetById(id);
    if (se == NULL) return;

    /* 限幅处理 */
    if (angle < -125)
        angle = -125;
    else if (angle > 125)
        angle = 125;

    se->target_angle = angle;
}

/**
 * @brief 设置舵机目标角度数（增量）
 * @param id    舵机编号
 * @param angle 目标角度增量
 * @retval 无
 */
void APP_SE_SetincreaseTarget(uint8_t id, int16_t angle)
{
    APP_SE_t *se = APP_SE_GetById(id);
    if (se == NULL) return;

    int16_t target;

    target = se->target_angle + angle;

    if (target < -125)
        target = -125;
    else if (target > 125)
        target = 125;

    se->target_angle = target;
}

/**
 * @brief 获取舵机当前角度数
 * @param id    舵机编号
 * @retval 无
 */
int16_t APP_SE_GetCurrent(uint8_t id)
{
    APP_SE_t *se = APP_SE_GetById(id);
    if (se == NULL) return 0;

    return se->current_angle;
}

/**
 * @brief 舵机平滑调度器（周期调用）
 * @note  建议5~20ms调用一次
 * @retval 无
 */
void APP_SE_Scheduler(void)
{
    for (uint8_t i = 0; i < app_se_count; i++)
    {
        APP_SE_t *se = &app_se_pool[i];

        if (se->enable == 0)
            continue;

        int16_t diff = se->target_angle - se->current_angle;

        if (diff == 0)
            continue;

        /* 向目标逼近 */
        if (diff > 0)
        {
            if (diff > se->speed)
                se->current_angle += se->speed;
            else
                se->current_angle += diff;
        }
        else
        {
            if (-diff > se->speed)
                se->current_angle -= se->speed;
            else
                se->current_angle += diff;
        }

        int16_t out_angle;

        out_angle = se->current_angle + se->offset + 150;

        if(out_angle < 0)
            out_angle = 0;
        else if(out_angle > 300)
            out_angle = 300;

        BSP_SE_300Angle(se->id, (uint16_t)out_angle);
    }
}