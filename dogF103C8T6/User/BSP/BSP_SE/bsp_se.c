/**
 * @file bsp_se.c
 * @brief 定时器控制PWM输出
 * @author 李嘉图
 * @date 2026-5-12
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
#include <bsp_se.h>

/*==============================================================================
 * 内部工具函数
 *============================================================================*/
/**
 * @brief  修改舵机PWM频率（采用1MHz计数频率，1μs分辨率）
 * @param  htim            定时器句柄
 * @param  TimerClockFreq  定时器时钟源频率，单位Hz
 * @param  DesiredFreq     目标PWM频率
 * @note   调用后定时器的脉宽映射变为：1次计数 = 1μs，之前设置的CCR值会失效，
 *         需立即通过 Servo_SetPulse() 重新设定各通道的脉宽。
 */
static void BSP_SetFreq(TIM_HandleTypeDef *htim, uint32_t TimerClockFreq, uint32_t DesiredFreq)
{
    /* 1. 计算预分频值，使定时器计数频率 = 1MHz */
    uint32_t psc = (TimerClockFreq / 1000000) - 1;   // 72MHz → 71

    /* 2. 计算自动重装值，周期 = 1e6 / DesiredFreq 个计数 */
    uint32_t arr = (1000000U / DesiredFreq) - 1;     // 300Hz → 3332

    /* 3. ARR 范围检查（16位定时器最大值 65535） */
    if (arr > 65535 || psc > 65535) {
        // 参数不合法（例如目标频率过低导致周期超过65535μs）
        return;
    }

    /* 4. 更新影子寄存器，并通过软件更新事件立即生效 */
    htim->Instance->PSC = psc;
    htim->Instance->ARR = arr;
    htim->Instance->EGR = TIM_EGR_UG;   // 产生更新事件，装载影子寄存器
}

/**
 * @brief  设置指定通道的舵机脉宽（适用于1MHz计数频率）
 * @param  htim      定时器句柄
 * @param  Channel   通道宏，如 TIM_CHANNEL_1
 * @param  Pulse_us  高电平脉宽，单位μs（典型范围500~2500）
 */
static void BSP_SetPulse(TIM_HandleTypeDef *htim, uint32_t Channel, uint16_t Pulse_us)
{
    __HAL_TIM_SET_COMPARE(htim, Channel, Pulse_us);
}

/*==============================================================================
 * 舵机静态池
 *============================================================================*/
static BSP_SE_t BSP_SE[SE_MAX_SIZE];
static uint8_t bsp_se_count = 0;

/**
 * @brief 根据舵机编号 查找 BSP 舵机实例
 * @param id 舵机编号
 * @retval 非NULL：返回对应舵机实例结构体指针
 * @retval NULL：未找到
 */
static BSP_SE_t *BSP_SE_GetById(uint8_t id)
{
    for (uint8_t i = 0; i < bsp_se_count; i++)
    {
        if (BSP_SE[i].id == id)
        {
            return &BSP_SE[i];
        }
    }
    return NULL;
}

/*==============================================================================
 * 初始化函数
 *============================================================================*/
/**
 * @brief 初始化舵机定时器
 */
void BSP_SE_Init(void)
{
    // 初始化舵机实例池
    bsp_se_count = 0;
    // 初始化定时器频率
    BSP_SetFreq(&htim2, TIM_Hz, 300);
    BSP_SetFreq(&htim3, TIM_Hz, 300);
}

/**
 * @brief 注册一个 BSP 舵机实例
 * @note 初始化 FIFO 和 DMA 接收，使用结构体内部的 DMA 缓冲区
 * @param id        舵机编号（逻辑编号，不要求等于数组下标）
 * @param htim     TIM_HandleTypeDef句柄
 * @param Channel   PWM通道
 * @retval 1: 注册成功
 * @retval 0: 注册失败
 */
uint8_t BSP_SE_RegisterPort(uint8_t id, TIM_HandleTypeDef *htim, uint32_t Channel)
{
    if (!htim)
        return 0;

    /* 池满则失败 */
    if (bsp_se_count >= SE_MAX_SIZE)
        return 0;

    /* 防止重复注册同一 ID */
    if (BSP_SE_GetById(id) != NULL)
        return 0;

    /* 使用下一个空闲槽位 */
    BSP_SE_t *port = &BSP_SE[bsp_se_count];
    port->id         = id;
    port->htim       = htim;
    port->Channel    = Channel;

    bsp_se_count++;   /* 已注册数加一 */
    return 1;
}

/*==============================================================================
 * 定时器控制函数
 *============================================================================*/
/**
 * @brief 舵机定时器重置计数器
 * @param  htim  定时器句柄
 */
void BSP_SE_Reset(TIM_HandleTypeDef *htim)
{
    __HAL_TIM_SET_COUNTER(htim, 0);
}

/**
 * @brief 启动舵机定时器
 */ 
void BSP_SE_Start(TIM_HandleTypeDef *htim, uint32_t Channel)
{
    HAL_TIM_PWM_Start(htim, Channel);
}

/**
 * @brief 停止舵机定时器
 */ 
void BSP_SE_Stop(TIM_HandleTypeDef *htim, uint32_t Channel)
{
    HAL_TIM_PWM_Stop(htim, Channel);
}

/*==============================================================================
 * 舵机控制函数
 *============================================================================*/
/**
 * @brief  180°舵机角度控制
 * @param  id   舵机编号
 * @param  angle 角度（0~180）
 * @retval 无
 */
void BSP_SE_180Angle(uint8_t id, uint16_t angle)
{
    if(angle > 180) angle = 180;
    BSP_SE_t *port = BSP_SE_GetById(id);
    if(port == NULL)return;
    uint16_t off = 500 + (uint16_t)((uint32_t)angle * 2000 / 180);
    BSP_SetPulse(port->htim, port->Channel, off);
}

/**
 * @brief  270°舵机角度控制
 * @param  id   舵机编号
 * @param  angle 角度（0~270）
 * @retval 无
 */
void BSP_SE_270Angle(uint8_t id, uint16_t angle)
{
    if(angle > 270) angle = 270;
    BSP_SE_t *port = BSP_SE_GetById(id);
    if(port == NULL)return;
    uint16_t off = 500 + (uint16_t)((uint32_t)angle * 2000 / 270);
    BSP_SetPulse(port->htim, port->Channel, off);
}

/**
 * @brief  300°舵机角度控制
 * @param  id   舵机编号
 * @param  angle 角度（0~300）
 * @retval 无
 */
void BSP_SE_300Angle(uint8_t id, uint16_t angle)
{
    if(angle > 300) angle = 300;
    BSP_SE_t *port = BSP_SE_GetById(id);
    if(port == NULL)return;
    uint16_t off = 500 + (uint16_t)((uint32_t)angle * 2000 / 300);
    BSP_SetPulse(port->htim, port->Channel, off);
}