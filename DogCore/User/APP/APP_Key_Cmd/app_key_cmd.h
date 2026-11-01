/**
 * @file app_key_cmd.h
 * @brief 按键业务：按键事件 → 产品动作（命令表驱动）
 * @author Ricardo
 * @date 2026-11-01
 *
 * @note 这一层只做两件事：保存业务状态、把 (key_id, event) 映射成动作。
 *       消抖与长短按判定在 Service/Svc_Key_Logic，扫描与队列在 TASK，
 *       本层既不读 GPIO 也不碰 OS 队列。
 *
 *       新增一个按键动作 = 在 app_key_cmd.c 的 APP_KEY_CMD_TABLE 里加一行。
 */

#ifndef __APP_Key_Cmd_H__
#define __APP_Key_Cmd_H__

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include <stdint.h>
#include "svc_key_logic.h"      /* key_event_t */

/*==============================================================================
 * 接口
 *============================================================================*/
/**
 * @brief 分发一次按键事件
 * @param key_id 业务按键编号（与 TASK 的扫描表一致，1 起）
 * @param event  事件类型
 * @retval 无
 * @note 在命令线程里调用（TASK 上下文）；表里没有的组合静默忽略。
 */
void APP_Key_Cmd_Dispatch(uint8_t key_id, key_event_t event);

#endif /* __APP_Key_Cmd_H__ */
