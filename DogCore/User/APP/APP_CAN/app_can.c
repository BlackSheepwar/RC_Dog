/**
 * @file app_can.c
 * @brief CAN 业务层实现：端口表 + 发送出口 + 排空
 * @author Ricardo
 * @date 2026-11-01
 *
 * @note 每个端口 = 一个总线句柄 + 一条发送缓冲。
 *       排队与"邮箱满就留到下一轮"的逻辑在 Svc_Can_Tx，
 *       本文件只负责把业务调用翻译成帧、再交给 Service。
 *
 *       本层没有 cmsis_os.h：唤醒发送线程靠 TASK 的周期调度，
 *       不靠信号量，APP 因此完全与 OS 解耦。
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include <stddef.h>
#include "common.h"             /* ARRAY_SIZE */
#include "app_can.h"
// 功能包含
#include "svc_can_tx.h"         /* 发送缓冲 */
#include "app_can_cmd.h"

/*==============================================================================
 * 类型定义
 *============================================================================*/
/**
 * @brief CAN 端口实例
 */
typedef struct {
    const bsp_can_t *bus;       /* 总线句柄 */
    svc_can_tx_t     tx;        /* 发送缓冲 */
    uint8_t          opened;    /* 1: 已启动 */
} app_can_port_t;

/*==============================================================================
 * 内部变量
 *============================================================================*/
/** @brief 端口表：加第二路 CAN 时新增一项，并把 APP_CAN_PORT_COUNT 改成 2 */
static app_can_port_t s_can_ports[APP_CAN_PORT_COUNT] = {
    /* 发送缓冲靠静态零初始化：head/tail/count/dropped 全 0 就是"空队列" */
    { .bus = &bsp_can1, .opened = 0u },
};

/*==============================================================================
 * 内部函数
 *============================================================================*/
/**
 * @brief 按端口号取实例
 * @param port 端口编号，1 起
 * @retval 非 NULL: 端口实例
 * @retval NULL: 编号越界
 */
static app_can_port_t *APP_CAN_Find_Port(uint8_t port)
{
    if ((port < 1u) || (port > APP_CAN_PORT_COUNT)) {
        return NULL;
    }
    return &s_can_ports[port - 1u];
}

/*==============================================================================
 * 初始化与开闭
 *============================================================================*/
/**
 * @brief 清空发送缓冲并复位 BSP 启动状态
 * @retval 0: 成功
 */
int APP_CAN_Init(void)
{
    for (uint8_t i = 0u; i < APP_CAN_PORT_COUNT; i++) {
        Svc_Can_Tx_Init(&s_can_ports[i].tx);
        s_can_ports[i].opened = 0u;
    }

    return BSP_CAN_Init();
}

/**
 * @brief 按配置表配置滤波器并启动总线
 * @param port 端口编号，1 ~ APP_CAN_PORT_COUNT
 * @retval 0: 成功
 * @retval -1: 端口号非法
 * @retval -2: 某个滤波器配置失败
 * @retval -3: 总线启动失败
 */
int APP_CAN_Open(uint8_t port)
{
    app_can_port_t *inst = APP_CAN_Find_Port(port);

    if (inst == NULL) {
        return -1;
    }

    if (inst->opened != 0u) {
        return 0;
    }

    /* ---------- 1. 滤波器 ---------- */
    for (uint8_t i = 0u; i < ARRAY_SIZE(APP_CAN1_FILTER_TABLE); i++) {
        if (BSP_CAN_Set_Filter(inst->bus, &APP_CAN1_FILTER_TABLE[i]) != 0) {
            return -2;
        }
    }

    /* ---------- 2. 启动总线与接收中断 ---------- */
    if (BSP_CAN_Start(inst->bus) != 0) {
        return -3;
    }

    inst->opened = 1u;
    return 0;
}

/*==============================================================================
 * 发送
 *============================================================================*/
/**
 * @brief 组一帧并放进发送缓冲
 * @param port 端口编号
 * @param id   帧 ID
 * @param ide  ID 类型
 * @param data 数据，可为 NULL
 * @param dlc  数据长度 0 ~ 8
 * @retval 0: 已入队
 * @retval -1: 参数非法
 * @retval -2: 发送缓冲已满
 */
int APP_CAN_Send(uint8_t port, uint32_t id, uint8_t ide,
                 const uint8_t *data, uint8_t dlc)
{
    app_can_port_t *inst = APP_CAN_Find_Port(port);
    bsp_can_frame_t frame;

    if (inst == NULL) {
        return -1;
    }

    if (dlc > BSP_CAN_DATA_LEN) {
        return -1;
    }

    if ((dlc > 0u) && (data == NULL)) {
        return -1;
    }

    /* ---------- 组帧（数据帧，dlc 以外的字节填 0） ---------- */
    frame.id  = id;
    frame.ide = ide;
    frame.rtr = (uint8_t)BSP_CAN_FRAME_DATA;
    frame.dlc = dlc;

    for (uint8_t i = 0u; i < BSP_CAN_DATA_LEN; i++) {
        frame.data[i] = (i < dlc) ? data[i] : 0u;
    }

    return (Svc_Can_Tx_Push(&inst->tx, &frame) == 0) ? 0 : -2;
}

/**
 * @brief 把各端口发送缓冲里的帧推进硬件邮箱
 * @retval 无
 */
void APP_CAN_Flush(void)
{
    for (uint8_t i = 0u; i < APP_CAN_PORT_COUNT; i++) {
        if (s_can_ports[i].opened == 0u) {
            continue;
        }
        (void)Svc_Can_Tx_Flush(&s_can_ports[i].tx, s_can_ports[i].bus);
    }
}

/**
 * @brief 查询某端口的累计丢弃帧数
 * @param port 端口编号
 * @retval 累计丢弃帧数；端口非法时返回 0
 */
uint32_t APP_CAN_Dropped(uint8_t port)
{
    app_can_port_t *inst = APP_CAN_Find_Port(port);

    return (inst != NULL) ? Svc_Can_Tx_Dropped(&inst->tx) : 0u;
}
