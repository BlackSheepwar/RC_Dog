/**
 * @file bsp_iic.h
 * @brief IIC 硬件驱动层：统一传输入口，屏蔽 HAL 与器件差异
 * @author 李嘉图
 * @date 2026-10-31
 *
 * @note 一个 bsp_iic_t 对应一路硬件 IIC。上层只描述"发给谁、发什么、
 *       带不带寄存器子地址"，怎么起 START、怎么等完成由本层决定。
 *       公共头文件不暴露 HAL 类型，HAL 句柄只存在于 .c 内部。
 *
 *       本工程 I2C2 没有配 DMA（i2c.c 里无 hdmatx），所以一次传输用
 *       轮询完成、按 timeout_ms 兜底。接口形状与 frame 工程那套
 *       DMA + 完成中断的实现保持一致：将来在 .ioc 里打开 I2C2 的
 *       TX/RX DMA，只需改 .c 里的 BSP_IIC_StartHal 与回调，上层不动。
 *
 *       总线由 IIC_LOCK 互斥量排队（配在 .ioc，由 freertos.c 创建）：
 *       谁先抢到锁谁先发，抢不到就在锁上等，排队深度等于任务数。
 *       互斥量带优先级继承，不会出现低优先级任务占着总线把高优先级饿死。
 */

#ifndef __BSP_IIC_H__
#define __BSP_IIC_H__

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include <stdint.h>

/*==============================================================================
 * 类型定义
 *============================================================================*/
/**
 * @brief 传输方向
 */
typedef enum {
    BSP_IIC_DIR_TX = 0, /* 主机发送 */
    BSP_IIC_DIR_RX = 1, /* 主机接收 */
} bsp_iic_dir_t;

/**
 * @brief 传输模式
 */
typedef enum {
    BSP_IIC_MODE_RAW = 0, /* 直接收发，没有寄存器子地址 */
    BSP_IIC_MODE_MEM = 1, /* 带寄存器子地址，repeated START 由 HAL 完成 */
} bsp_iic_mode_t;

/**
 * @brief BSP_IIC 返回码
 */
typedef enum {
    BSP_IIC_OK          =  0, /* 成功 */
    BSP_IIC_ERR_PARAM   = -1, /* 参数非法 */
    BSP_IIC_ERR_STATE   = -2, /* 驱动未初始化 */
    BSP_IIC_ERR_TIMEOUT = -3, /* 传输超时 */
    BSP_IIC_ERR_NACK    = -4, /* 从机 NACK（器件没接或地址错） */
    BSP_IIC_ERR_BUS     = -5, /* 总线错误（仲裁丢失/总线错/溢出） */
    BSP_IIC_ERR_HAL     = -7, /* HAL 启动失败或其他 HAL 错误 */
} bsp_iic_status_t;

/**
 * @brief 一次传输命令
 */
typedef struct {
    uint16_t dev_addr;      /* 7 bit 从机地址，未左移 */
    uint16_t mem_addr;      /* 寄存器子地址，RAW 模式忽略 */
    uint16_t mem_addr_size; /* 子地址字节数 1 或 2，RAW 模式忽略 */
    uint16_t len;           /* 数据字节数，不能为 0 */
    uint8_t  dir;           /* 方向，取 bsp_iic_dir_t */
    uint8_t  mode;          /* 模式，取 bsp_iic_mode_t */
    uint8_t *data;          /* 收发缓冲区，不能为 NULL */
    uint32_t timeout_ms;    /* 超时，0 表示用默认 50 ms */
} bsp_iic_msg_t;

/**
 * @brief IIC 总线句柄（不透明类型，成员定义在 bsp_iic.c）
 */
typedef struct bsp_iic bsp_iic_t;

/*==============================================================================
 * 外部变量
 *============================================================================*/
/** @brief I2C2 总线（OLED 所在的那一路） */
extern bsp_iic_t bsp_iic2;

/*==============================================================================
 * 函数声明
 *============================================================================*/
/**
 * @brief 初始化 IIC 驱动：校验 HAL 句柄与外设状态
 * @retval BSP_IIC_OK: 初始化成功（重复调用直接返回成功）
 * @retval BSP_IIC_ERR_STATE: HAL 句柄缺失、互斥量未创建或外设未就绪
 * @note 必须在 MX_I2C2_Init() 之后调用，建议放在 freertos.c 的
 *       USER CODE BEGIN RTOS_MUTEX（那里若配了 IIC_LOCK 互斥量也已建好）。
 *       本函数清运行时状态，可在复位流程里重复调用。
 */
bsp_iic_status_t BSP_IIC_Init(void);

/**
 * @brief 提交一次 IIC 传输并等它完成
 * @param bus 总线句柄，不能为 NULL
 * @param msg 传输命令，不能为 NULL；data 在返回前必须保持有效
 * @retval BSP_IIC_OK: 传输成功
 * @retval BSP_IIC_ERR_PARAM: 地址、方向、模式、长度或缓冲区非法
 * @retval BSP_IIC_ERR_STATE: 驱动未初始化
 * @retval BSP_IIC_ERR_TIMEOUT: 等锁或总线传输超时
 * @retval BSP_IIC_ERR_NACK: 从机 NACK
 * @retval BSP_IIC_ERR_BUS: 总线错误
 * @retval BSP_IIC_ERR_HAL: HAL 启动失败或其他 HAL 错误
 * @note 阻塞式：一次传输期间调用者一直持有总线（OLED 整屏刷新约 23 ms）。
 *       不能在中断里调用，也不能被同一任务重入。
 */
bsp_iic_status_t BSP_IIC_Transfer(bsp_iic_t *bus, const bsp_iic_msg_t *msg);

/**
 * @brief 直接向从机发送数据（RAW 模式）
 * @param bus      总线句柄，不能为 NULL
 * @param dev_addr 7 bit 从机地址，未左移
 * @param data     发送缓冲区，不能为 NULL
 * @param len      发送字节数，不能为 0
 * @retval BSP_IIC_OK: 发送成功
 * @retval 其他: 见 BSP_IIC_Transfer
 */
bsp_iic_status_t BSP_IIC_Write(bsp_iic_t *bus, uint16_t dev_addr,
                               const uint8_t *data, uint16_t len);

/**
 * @brief 直接从从机读取数据（RAW 模式）
 * @param bus      总线句柄，不能为 NULL
 * @param dev_addr 7 bit 从机地址，未左移
 * @param data     接收缓冲区，不能为 NULL
 * @param len      接收字节数，不能为 0
 * @retval BSP_IIC_OK: 读取成功
 * @retval 其他: 见 BSP_IIC_Transfer
 */
bsp_iic_status_t BSP_IIC_Read(bsp_iic_t *bus, uint16_t dev_addr,
                              uint8_t *data, uint16_t len);

/**
 * @brief 向从机的 8 bit 子地址写数据（MEM 模式）
 * @param bus      总线句柄，不能为 NULL
 * @param dev_addr 7 bit 从机地址，未左移
 * @param reg      8 bit 寄存器子地址
 * @param data     发送缓冲区，不能为 NULL
 * @param len      发送字节数，不能为 0
 * @retval BSP_IIC_OK: 写入成功
 * @retval 其他: 见 BSP_IIC_Transfer
 */
bsp_iic_status_t BSP_IIC_WriteReg(bsp_iic_t *bus, uint16_t dev_addr,
                                  uint8_t reg, const uint8_t *data,
                                  uint16_t len);

/**
 * @brief 从从机的 8 bit 子地址读数据（MEM 模式）
 * @param bus      总线句柄，不能为 NULL
 * @param dev_addr 7 bit 从机地址，未左移
 * @param reg      8 bit 寄存器子地址
 * @param data     接收缓冲区，不能为 NULL
 * @param len      接收字节数，不能为 0
 * @retval BSP_IIC_OK: 读取成功
 * @retval 其他: 见 BSP_IIC_Transfer
 */
bsp_iic_status_t BSP_IIC_ReadReg(bsp_iic_t *bus, uint16_t dev_addr,
                                 uint8_t reg, uint8_t *data,
                                 uint16_t len);

#endif /* __BSP_IIC_H__ */
