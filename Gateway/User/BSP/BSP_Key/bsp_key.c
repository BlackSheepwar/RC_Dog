/**
 * @file bsp_key.c
 * @brief 按键硬件接口实现
 * @author 李嘉图
 * @date 2026-04-18
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
#include "main.h"
#include "bsp_key.h"

/*==============================================================================
 * 静态资源池
 *============================================================================*/
static BSP_Key_t bsp_key_pool[KEY_MAX_NUM];      // 按键资源池
static uint8_t bsp_key_count = 0;            // 已注册按键数量

/*==============================================================================
 * 内部函数
 *============================================================================*/
/**
 * @brief 根据按键ID获取按键结构体（O(n)）
 * @param key_id 按键ID
 * @retval 非NULL：返回对应按键结构体指针
 * @retval NULL：未找到
 */
static BSP_Key_t* BSP_Key_GetById(uint8_t key_id)
{
    /* ---------- 遍历已注册按键 ---------- */
    for(uint8_t i = 0; i < bsp_key_count; i++)
    {
        if(bsp_key_pool[i].id == key_id)
        {
            return &bsp_key_pool[i];
        }
    }

    return NULL; // 未找到
}

/*==============================================================================
 * 初始化函数
 *============================================================================*/
/**
 * @brief 按键初始化
 */
void BSP_Key_Init(void)
{
    bsp_key_count = 0;
}

/**
 * @brief 注册一个按键到资源池
 * @param key_id  按键ID
 * @param GPIOx   GPIO端口
 * @param Pin     引脚
 * @retval 0: 参数错误/重复
 * @retval 1: 成功
 * @retval 2: 资源池已满
 */
uint8_t BSP_Key_Register(uint8_t key_id, GPIO_TypeDef *GPIOx, uint16_t Pin)
{
    if(GPIOx == NULL) return 0;

    /* ---------- 查重（O(n)） ---------- */
    if(BSP_Key_GetById(key_id) != NULL)
    {
        return 0; // 已存在
    }

    /* ---------- 容量检查 ---------- */
    if(bsp_key_count >= KEY_MAX_NUM)
    {
        return 2;
    }

    /* ---------- 注册 ---------- */
    bsp_key_pool[bsp_key_count].id    = key_id;
    bsp_key_pool[bsp_key_count].GPIOx = GPIOx;
    bsp_key_pool[bsp_key_count].Pin   = Pin;

    bsp_key_count++;

    return 1;
}


/*==============================================================================
 * 功能函数
 *============================================================================*/
/**
 * @brief 按键电平状态获取
 * @param key_id 按键ID
 * @retval 1: 高电平
 * @retval 0: 低电平
 * @retval 0xFF: 未找到
 */
uint8_t BSP_Key_ReadState(uint8_t key_id)
{
    BSP_Key_t* key = BSP_Key_GetById(key_id);
    if(key == NULL) return 0xFF;

    return HAL_GPIO_ReadPin(key->GPIOx, key->Pin);
}