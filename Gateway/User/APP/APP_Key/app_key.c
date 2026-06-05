/**
 * @file app_key.c
 * @brief 按键应用层处理按键事件
 * @author 李嘉图
 * @date 2026-05-08
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
#include "main.h"
#include "app_key.h"

/*==============================================================================
 * 静态资源池
 *============================================================================*/
static Debounce_Key_t app_key_pool[KEY_MAX_NUM];
static uint8_t app_key_count = 0;

/*==============================================================================
 * 内部函数
 *============================================================================*/
/**
 * @brief 根据ID查找任务结构体（O(n)）
 * @param key_id 按键ID
 * @retval 非NULL：找到
 * @retval NULL：未找到
 */
static Debounce_Key_t* App_Key_GetById(uint8_t key_id)
{
    for(uint8_t i = 0; i < app_key_count; i++)
    {
        if(app_key_pool[i].id == key_id)
        {
            return &app_key_pool[i];
        }
    }
    return NULL;
}

/**
 * @brief 队列发送按键事件
 * @param id 按键ID
 * @param event 按键事件
 */
static void App_Key_SendEvent(uint8_t id, Debounce_Event_t event)
{
    Debounce_Event_packet_t msg;
    msg.id = id;
    msg.event = event;

    if(osMessageQueuePut(KEY_QHandle, &msg, 0, 0) != osOK)
    {
        // TODO: 队列满处理
    }
}

/*==============================================================================
 * 初始化函数
 *============================================================================*/
/**
 * @brief 初始化
 */
void App_Key_Init(void)
{
    BSP_Key_Init();
    app_key_count = 0;
}

/**
 * @brief 注册一个按键到静态资源池
 * @param key_id  按键ID（用户定义）
 * @param long_press 长按判定阈值（单位:任务周期）
 * @param active_level 按键激活电平（0:低电平，1:高电平）
 * @param GPIOx   按键GPIO端口
 * @param Pin     按键引脚
 * @retval 0: 参数错误
 * @retval 1: 成功
 * @retval 2: 资源池已满
 */
uint8_t App_Key_Register(uint8_t key_id,
                        uint8_t long_press,
                        uint8_t active_level,
                        GPIO_TypeDef *GPIOx,
                        uint16_t Pin)
{
    if(GPIOx == NULL) return 0;

    /* ---------- BSP注册 ---------- */
    uint8_t ret = BSP_Key_Register(key_id, GPIOx, Pin);
    if (ret != 1) return ret;

    /* ---------- 查重（O(n)） ---------- */
    if(App_Key_GetById(key_id) != NULL)
    {
        return 0;
    }

    /* ---------- 容量检查 ---------- */
    if(app_key_count >= KEY_MAX_NUM)
    {
        return 2;
    }

    /* ---------- 注册 ---------- */
    Debounce_Key_t *key = &app_key_pool[app_key_count];

    key->id           = key_id;
    key->last_raw     = BSP_Key_ReadState(key_id);
    key->counter      = 0;
    key->long_counter = 0;
    key->state        = KEY_NO;
    key->long_press   = long_press;
    key->active_level = active_level;

    app_key_count++;

    return 1;
}

/*==============================================================================
 * 更新按键状态
 *============================================================================*/
/**
 * @brief 更新按键状态
 */
void App_Key_Update(void)
{

    for(uint8_t i = 0; i < app_key_count; i++)
    {
        Debounce_Key_t *key = &app_key_pool[i];
        uint8_t raw = BSP_Key_ReadState(key->id);
        Debounce_Event_t event = Debounce_Update(key, raw);
        if(event != KEY_EVENT_NONE)
        {
            App_Key_SendEvent(key->id, event);
        }
    }
}