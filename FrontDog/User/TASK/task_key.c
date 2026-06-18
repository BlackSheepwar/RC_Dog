/**
 * @file task_key.c
 * @brief 实现按键扫描与消息发送任务
 * @author 李嘉图
 * @date 2026-06-17
 *
 * @note 按键注册参数已提取到 app_key_cfg.h，
 *       GPIO 引脚映射已下沉到 bsp_gpio.c 的 GPIO_HW_MAP，
 *       Task 层遍历配置表完成注册，新增按键只需在表中加一项。
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
#include "main.h"
#include "app_key.h"
#include "app_key_cfg.h"
#include "common.h"

/*==============================================================================
 * 任务函数
 *============================================================================*/
void Task_KEY(void *argument)
{
    App_Key_Init();

    for (uint8_t i = 0; i < ARRAY_SIZE(KEY_REG_TABLE); i++)
    {
        App_Key_Register(KEY_REG_TABLE[i].key_id,
                         KEY_REG_TABLE[i].long_press,
                         KEY_REG_TABLE[i].active_level);
    }

    for (;;)
    {
        App_Key_Update();
        osDelay(KEY_SCAN_PERIOD_MS);
    }
}
