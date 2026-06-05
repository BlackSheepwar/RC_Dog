/**
 * @file app_usart_cmd.c
 * @brief 命令分发
 * @author 李嘉图
 * @date 2026-5-4
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
#include "main.h"
#include <stdint.h>
#include "app_usart_cmd.h"
#include "app_usart.h"

/*==============================================================================
 * 命令分发
 *============================================================================*/
static void APP_USART_AA(void)
{
    HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_11);
}

static void APP_USART_A9(void)
{
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
}

static void APP_USART_A8(void)
{
    for (uint8_t i = 0; i<10; i++)
    {
        APP_USART_BuildTxPacket(1, 0xA8, &i, 1);
    }
}

/**
 * @brief 命令分发函数
 * @param cmd 命令字
 * @param payload 命令数据
 * @param len 数据长度
 */
void APP_USART_Cmd(uint8_t cmd, uint8_t *payload, uint8_t len)
{
    switch(cmd)
    {
        case 0xAA: APP_USART_AA(); break;
        case 0xA9: APP_USART_A9(); break;
        case 0xA8: APP_USART_A8(); break;

        default: break;
    }
}