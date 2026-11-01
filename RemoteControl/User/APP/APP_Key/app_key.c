/**
 * @file app_key.c
 * @brief 按键应用层（内置消抖状态机）
 * @author 李嘉图
 * @date 2026-06-18
 *
 * @note GPIO 引脚映射已下沉到 BSP 层（bsp_gpio.c 的 GPIO_HW_MAP），
 *       消抖状态机从 Middlewares/Debounce 合并至此，本模块不再依赖外部模块。
 *
 *       内部状态机：
 *         APP_KEY_STATE_NO → (按下消抖稳定) → APP_KEY_EVENT_DOWN → APP_KEY_STATE_YES
 *         APP_KEY_STATE_YES → (释放消抖稳定) → APP_KEY_EVENT_CLICK → APP_KEY_STATE_NO
 *         APP_KEY_STATE_YES → (长按超时) → APP_KEY_EVENT_LONG → APP_KEY_STATE_LONG
 *         APP_KEY_STATE_LONG → (释放) → APP_KEY_EVENT_LONG_UP → APP_KEY_STATE_NO
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include <stdint.h>
#include "main.h"
#include "app_key.h"
// 功能包含
#include "bsp_gpio.h"      /* BSP_GPIO_ReadLevel, BSP_GPIO_Init */
#include "cmsis_os.h"      /* 消息队列 API */

/*==============================================================================
 * 内部类型定义
 *============================================================================*/
/** @brief 按键内部状态 */
typedef enum {
    APP_KEY_STATE_NO   = 0,   /**< 未按下 */
    APP_KEY_STATE_YES  = 1,   /**< 已按下 */
    APP_KEY_STATE_LONG = 2,   /**< 长按状态 */
} App_Key_State_t;

/**
 * @brief 按键实例结构体
 * @note 位域字段 packed 为 1 字节节省 RAM
 */
typedef struct {
    uint8_t  key_id;             /**< 逻辑按键 ID（分发表匹配用） */
    uint8_t  gpio_id;            /**< GPIO 映射 ID（BSP_GPIO 寻址用） */
    uint8_t  stable       : 1;   /**< 消抖后的稳定电平 */
    uint8_t  last_raw     : 1;   /**< 上一次原始电平 */
    uint8_t  active_level : 1;   /**< 有效电平（1=高电平按下，0=低电平按下） */
    uint8_t  state        : 2;   /**< 按键状态机（App_Key_State_t） */
    uint8_t  reserved     : 3;
    uint8_t  counter;            /**< 消抖计数器 */
    uint8_t  long_counter;       /**< 长按计数器 */
    uint8_t  long_press;         /**< 长按判定阈值（任务周期数） */
    uint8_t  debounce_ticks;     /**< 消抖稳定阈值（任务周期数） */
} App_Key_Key_t;

/*==============================================================================
 * 静态资源池
 *============================================================================*/
static App_Key_Key_t app_key_pool[KEY_MAX_NUM];
static uint8_t app_key_count = 0;

/** @brief 消息队列丢包计数（调试用） */
static uint32_t key_queue_drop_count = 0;

/*==============================================================================
 * 内部函数
 *============================================================================*/
/**
 * @brief 根据 ID 查找按键实例（O(n)）
 * @param key_id 按键ID
 * @retval 非 NULL：找到
 * @retval NULL：未找到
 */
static App_Key_Key_t *App_Key_GetById(uint8_t key_id)
{
    for (uint8_t i = 0; i < app_key_count; i++)
    {
        if (app_key_pool[i].key_id == key_id)
            return &app_key_pool[i];
    }
    return NULL;
}

/**
 * @brief 消抖状态机核心
 * @param key 按键实例指针
 * @param raw 原始电平（来自 BSP_GPIO_ReadLevel）
 * @retval 按键事件（APP_KEY_EVENT_NONE 表示无事件）
 *
 * @note 分两步：
 *       1. 消抖：连续 debounce_ticks 次采样一致才更新 stable
 *       2. 状态机：只用 stable 信号驱动，避免毛刺误触发
 */
static App_Key_Event_t App_Key_DebounceUpdate(App_Key_Key_t *key, uint8_t raw)
{
    if (raw == 0xFF)
        return APP_KEY_EVENT_NONE;      /* BSP 层无效 ID */

    /* ---- 1. 消抖：连续 N 次一致才算稳定 ---- */
    if (raw == key->last_raw)
    {
        if (key->counter < key->debounce_ticks)
        {
            key->counter++;
            return APP_KEY_EVENT_NONE;
        }
        key->stable = raw;              /* 只有这里才更新稳定值 */
    }
    else
    {
        key->counter   = 0;
        key->last_raw  = raw;
        return APP_KEY_EVENT_NONE;
    }

    /* ---- 2. 状态机：只用 stable，不用 raw ---- */
    switch (key->state)
    {
        case APP_KEY_STATE_NO:
            if (key->stable == key->active_level)
            {
                key->state        = APP_KEY_STATE_YES;
                key->long_counter = 0;
                return APP_KEY_EVENT_DOWN;
            }
            break;

        case APP_KEY_STATE_YES:
            if (key->stable == key->active_level)
            {
                if (key->long_counter < key->long_press)
                    key->long_counter++;
                else
                {
                    key->state = APP_KEY_STATE_LONG;
                    return APP_KEY_EVENT_LONG;
                }
            }
            else
            {
                key->state = APP_KEY_STATE_NO;

                if (key->long_counter < key->long_press)
                    return APP_KEY_EVENT_CLICK;
                else
                    return APP_KEY_EVENT_LONG_UP;
            }
            break;

        case APP_KEY_STATE_LONG:
            if (key->stable != key->active_level)
            {
                key->state = APP_KEY_STATE_NO;
                return APP_KEY_EVENT_LONG_UP;
            }
            break;
    }

    return APP_KEY_EVENT_NONE;
}

/**
 * @brief 发送按键事件到消息队列
 * @param key_id 逻辑按键 ID
 * @param event  按键事件
 */
static void App_Key_SendEvent(uint8_t key_id, App_Key_Event_t event)
{
    App_Key_EventPacket_t msg;
    msg.key_id = key_id;
    msg.event  = event;

    if (osMessageQueuePut(KEY_CMD_QHandle, &msg, 0, 0) != osOK)
        key_queue_drop_count++;
}

/*==============================================================================
 * 初始化函数
 *============================================================================*/
void App_Key_Init(void)
{
    BSP_GPIO_Init();
    app_key_count        = 0;
    key_queue_drop_count = 0;
}

/**
 * @brief 注册一个按键到静态资源池
 * @param key_id         逻辑按键 ID（分发表匹配用）
 * @param gpio_id        GPIO 映射 ID（BSP_GPIO 寻址用，须与 GPIO_HW_MAP 一致）
 * @param long_press     长按判定阈值（单位:任务周期）
 * @param active_level   按键激活电平（0:低电平，1:高电平）
 * @param debounce_ticks 消抖稳定计数（单位:任务周期，推荐 2~3）
 * @retval 0: 参数错误
 * @retval 1: 成功
 * @retval 2: 资源池已满
 *
 * @note key_id 和 gpio_id 分离设计，逻辑标识与硬件映射解耦。
 *       例如：协议定义的按键 #1 可以映射到任意 GPIO 引脚。
 */
uint8_t App_Key_Register(uint8_t key_id,
                         uint8_t gpio_id,
                         uint8_t long_press,
                         uint8_t active_level,
                         uint8_t debounce_ticks)
{
    /* ---------- 查重 ---------- */
    if (App_Key_GetById(key_id) != NULL)
        return 0;

    /* ---------- 容量检查 ---------- */
    if (app_key_count >= KEY_MAX_NUM)
        return 2;

    /* ---------- 确认 gpio_id 在 BSP 层有效 ---------- */
    uint8_t test = BSP_GPIO_ReadLevel(gpio_id);
    if (test == 0xFF)
        return 0;   /* BSP 层无此 gpio_id */

    /* ---------- 注册 ---------- */
    App_Key_Key_t *key = &app_key_pool[app_key_count];

    key->key_id        = key_id;
    key->gpio_id       = gpio_id;
    key->last_raw      = test;
    key->counter       = 0;
    key->long_counter  = 0;
    key->state         = APP_KEY_STATE_NO;
    key->long_press    = long_press;
    key->active_level  = active_level;
    key->debounce_ticks = debounce_ticks;

    app_key_count++;
    return 1;
}

/*==============================================================================
 * 更新按键状态
 *============================================================================*/
void App_Key_Update(void)
{
    for (uint8_t i = 0; i < app_key_count; i++)
    {
        App_Key_Key_t *key = &app_key_pool[i];
        uint8_t raw = BSP_GPIO_ReadLevel(key->gpio_id);
        App_Key_Event_t event = App_Key_DebounceUpdate(key, raw);

        if (event != APP_KEY_EVENT_NONE)
            App_Key_SendEvent(key->key_id, event);
    }
}
