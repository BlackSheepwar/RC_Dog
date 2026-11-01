/**
 * @file bsp_can.h
 * @brief CAN（FDCAN）硬件驱动层：总线句柄 + 帧值类型
 * @author Ricardo
 * @date 2026-11-01
 *
 * @note 本层只做"把一帧丢上总线、从 FIFO 取一帧"，不认识任何 ID 的含义。
 *       哪些 ID 交给哪个处理函数，是 APP_CAN 的命令表决定的。
 *
 *       头文件里没有 fdcan.h：HAL 的 FDCAN_RxHeaderTypeDef 一旦出现在
 *       BSP 头里，Device / APP 的头文件链就会被迫认识 HAL。
 *       这里用 bsp_can_frame_t 这个值类型代替——它是队列元素，上层要算
 *       sizeof，所以按值类型完整定义在头文件里（值类型的合法例外）。
 *
 *       中断把收到的帧投进 CAN_RXF0_Q / CAN_RXF1_Q，由 TASK 取走分发；
 *       队列由 CubeMX 在 freertos.c 里创建，本层只使用、不创建。
 */

#ifndef __BSP_CAN_H__
#define __BSP_CAN_H__

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include <stdint.h>

/*==============================================================================
 * 宏定义与常量
 *============================================================================*/
/** @brief 一帧经典 CAN 的数据字节上限 */
#define BSP_CAN_DATA_LEN        8u
/** @brief 标准帧 ID 上限（11 bit） */
#define BSP_CAN_STD_ID_MAX      0x7FFu
/** @brief 扩展帧 ID 上限（29 bit） */
#define BSP_CAN_EXT_ID_MAX      0x1FFFFFFFu

/*==============================================================================
 * 类型定义
 *============================================================================*/
/**
 * @brief ID 类型
 */
typedef enum {
    BSP_CAN_ID_STD = 0,     /* 标准帧，11 bit */
    BSP_CAN_ID_EXT = 1,     /* 扩展帧，29 bit */
} bsp_can_id_t;

/**
 * @brief 帧类型
 */
typedef enum {
    BSP_CAN_FRAME_DATA   = 0,   /* 数据帧 */
    BSP_CAN_FRAME_REMOTE = 1,   /* 遥控帧，data 无效 */
} bsp_can_frame_type_t;

/**
 * @brief 接收 FIFO 编号
 */
typedef enum {
    BSP_CAN_FIFO0 = 0,      /* 高优先级：控制帧 */
    BSP_CAN_FIFO1 = 1,      /* 低优先级：遥测帧 */
} bsp_can_fifo_t;

/**
 * @brief 滤波器匹配方式
 */
typedef enum {
    BSP_CAN_FILTER_MASK  = 0,   /* ID + 掩码 */
    BSP_CAN_FILTER_RANGE = 1,   /* [id1, id2] 区间 */
    BSP_CAN_FILTER_DUAL  = 2,   /* id1 / id2 两个独立 ID */
} bsp_can_filter_mode_t;

/**
 * @brief 一帧 CAN 报文（值类型：队列元素，按值传递）
 */
typedef struct {
    uint32_t id;                    /* 帧 ID（标准 11 bit / 扩展 29 bit） */
    uint8_t  ide;                   /* ID 类型，取 bsp_can_id_t */
    uint8_t  rtr;                   /* 帧类型，取 bsp_can_frame_type_t */
    uint8_t  dlc;                   /* 数据长度 0 ~ 8 */
    uint8_t  data[BSP_CAN_DATA_LEN];/* 数据，遥控帧时全 0 */
} bsp_can_frame_t;

/**
 * @brief 兼容别名：CubeMX 生成的 freertos.c 用旧名字建队列
 * @note 生成代码在 USER CODE 之外，改不动；用别名让它继续编译，
 *       两边的 sizeof 一定一致（同一个类型）。新代码一律用 bsp_can_frame_t。
 */
typedef bsp_can_frame_t BSP_CAN_Packet_t;

/**
 * @brief 滤波器配置（值类型：上层按值构造一条规则）
 */
typedef struct {
    uint8_t  index;     /* 滤波器序号，FDCAN 标准列表 0 ~ 127 */
    uint8_t  fifo;      /* 命中后送哪个 FIFO，取 bsp_can_fifo_t */
    uint8_t  mode;      /* 匹配方式，取 bsp_can_filter_mode_t */
    uint8_t  ide;       /* 匹配的 ID 类型，取 bsp_can_id_t */
    uint32_t id1;       /* 主 ID / 区间下限 */
    uint32_t id2;       /* 次 ID / 区间上限，MASK 与 DUAL 模式用 */
} bsp_can_filter_t;

/**
 * @brief CAN 总线句柄（不透明类型，布局在 bsp_can.c）
 */
typedef struct bsp_can bsp_can_t;

/*==============================================================================
 * 外部声明
 *============================================================================*/
/** @brief FDCAN1 总线句柄 */
extern const bsp_can_t bsp_can1;

/*==============================================================================
 * 初始化与启停
 *============================================================================*/
/**
 * @brief 复位启动标志（不碰硬件）
 * @retval 0: 成功
 * @note 上电流程里调一次；之后 Start 才会真正去启 HAL。
 */
int BSP_CAN_Init(void);

/**
 * @brief 启动总线并使能 FIFO0 / FIFO1 接收中断
 * @param can 总线句柄，不可为 NULL
 * @retval 0: 已启动（重复调用直接返回成功）
 * @retval -1: 句柄为 NULL 或 HAL 启动失败
 * @note 只使能接收中断：发送是否完成由 TASK 周期排空软件缓冲判断，
 *       BSP 不掺和任何 OS 对象。
 */
int BSP_CAN_Start(const bsp_can_t *can);

/*==============================================================================
 * 滤波器
 *============================================================================*/
/**
 * @brief 配置一条滤波器规则
 * @param can 总线句柄，不可为 NULL
 * @param cfg 规则，不可为 NULL
 * @retval 0: 配置成功
 * @retval -1: 参数非法（句柄/规则为空、序号越界、ID 超出该类型上限）
 * @retval -2: HAL 配置失败
 */
int BSP_CAN_Set_Filter(const bsp_can_t *can, const bsp_can_filter_t *cfg);

/*==============================================================================
 * 收发
 *============================================================================*/
/**
 * @brief 把一帧放进硬件发送 FIFO（非阻塞）
 * @param can   总线句柄，不可为 NULL
 * @param frame 待发帧，不可为 NULL
 * @retval 0: 已进入硬件 FIFO
 * @retval -1: 参数非法（ID 超限、dlc > 8、数据帧却没给数据）
 * @retval -2: 硬件 FIFO 满或总线未启动
 * @note 只发一帧、不等完成。要保证不丢帧，请走 Svc_Can_Tx 的软件缓冲。
 */
int BSP_CAN_Send(const bsp_can_t *can, const bsp_can_frame_t *frame);

#endif /* __BSP_CAN_H__ */
