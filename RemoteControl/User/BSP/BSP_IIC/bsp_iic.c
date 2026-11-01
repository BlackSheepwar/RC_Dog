/**
 * @file bsp_iic.c
 * @brief IIC 硬件驱动层实现：轮询传输 + 可选互斥量
 * @author 李嘉图
 * @date 2026-10-31
 *
 * @note 设计模型（与 frame 工程一致，只把 DMA 换成了轮询）：
 *       1. 上层填一个 bsp_iic_msg_t，描述"发给谁、发什么、带不带子地址"；
 *       2. 本层把 7 bit 地址左移成 HAL 要的形式，按方向/模式选 HAL 接口；
 *       3. 一次调用阻塞到传输完成或超时，返回统一的状态码；
 *       4. 总线可能被多个任务使用时，顶层先抢锁再传输（见 BSP_IIC_Transfer）。
 *
 *       为什么不是 DMA：本工程 I2C2 在 .ioc 里没开 DMA（i2c.c 里没有
 *       hdmatx/hdmarx），HAL 的 *_DMA 版本会在启动时直接失败。OLED 整屏
 *       刷新 1024 字节 @400 kHz 约 23 ms，对 100 ms 级的显示任务可以接受。
 *       日后在 .ioc 打开 I2C2 TX DMA 后，只需：
 *         BSP_IIC_StartHal 换成 HAL_I2C_*_DMA；
 *         补 HAL_I2C_MasterTxCplt/MemTxCplt/ErrorCallback 唤醒等待者。
 *       上层调用方式不变。
 *
 *       OS 对象归属：互斥量 IIC_LOCK 定义在 .ioc，由 CubeMX 生成到
 *       freertos.c；本文件只 extern 使用，不创建任何同步对象。
 *       因为是带优先级继承的互斥量，BSP_IIC_Init() 必须放在 freertos.c 的
 *       USER CODE BEGIN RTOS_MUTEX：那里 IIC_LOCKHandle 刚刚建好。
 *
 *       调用上下文：只能在任务里调用，不能在中断里调用。
 */

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include <stddef.h>
#include <stdint.h>
#include "main.h"
#include "i2c.h"
#include "bsp_iic.h"

/*==============================================================================
 * 外部变量声明
 *============================================================================*/
/** @brief CubeMX 生成的互斥量句柄（.ioc 里配置的 IIC_LOCK） */
extern osMutexId_t IIC_LOCKHandle;

/*==============================================================================
 * 宏定义与常量
 *============================================================================*/
/** @brief 单笔传输默认超时（ms） */
#define BSP_IIC_DEFAULT_TIMEOUT_MS  50U
/** @brief 7 bit 地址上限 */
#define BSP_IIC_ADDR_MAX            0x7FU

/*==============================================================================
 * 类型定义
 *============================================================================*/
/**
 * @brief IIC 总线对象
 * @note  hal 是 void *，避免公共头文件引入 HAL 类型；
 *        lock 存互斥量句柄的地址（句柄值要运行时才产生，存地址才能在
 *        实例定义处一次配好）；lock 为 NULL 表示本总线不需要排队。
 */
struct bsp_iic {
    void             *hal;         /* HAL I2C 句柄（I2C_HandleTypeDef *） */
    osMutexId_t      *lock;        /* 指向 freertos.c 生成的互斥量句柄，可为 NULL */
    volatile uint8_t  initialized; /* 1: 已完成初始化 */
};

/*==============================================================================
 * 内部变量
 *============================================================================*/
/**
 * @brief I2C2 总线对象（OLED 挂在这一路）
 * @note lock 存的是互斥量句柄的地址而不是句柄值：句柄要等
 *       MX_FREERTOS_Init() 跑到 osMutexNew 才有值，地址却是链接期常量，
 *       于是能在实例定义处一次配好，初始化时只做校验。
 */
bsp_iic_t bsp_iic2 = {
    .hal         = &hi2c2,
    .lock        = &IIC_LOCKHandle,
    .initialized = 0U,
};

/** @brief 驱动管理的总线表；新增 I2C1 时只在这里追加实例 */
static bsp_iic_t *const s_buses[] = {
    &bsp_iic2,
};

/*==============================================================================
 * 内部函数
 *============================================================================*/
/**
 * @brief 把 HAL 的返回码与错误码翻译成 BSP 状态码
 * @param hi2c HAL IIC 句柄，不可为 NULL
 * @param st   HAL 返回值
 * @retval BSP_IIC_OK: 成功
 * @retval BSP_IIC_ERR_TIMEOUT: 超时
 * @retval BSP_IIC_ERR_NACK: 从机无应答
 * @retval BSP_IIC_ERR_BUS: 总线错误
 * @retval BSP_IIC_ERR_HAL: 其他 HAL 错误
 */
static bsp_iic_status_t BSP_IIC_MapStatus(const I2C_HandleTypeDef *hi2c,
                                          HAL_StatusTypeDef st)
{
    if (st == HAL_OK)
        return BSP_IIC_OK;
    if (st == HAL_TIMEOUT)
        return BSP_IIC_ERR_TIMEOUT;

    if ((hi2c->ErrorCode & HAL_I2C_ERROR_AF) != 0U)
        return BSP_IIC_ERR_NACK;

    if ((hi2c->ErrorCode & (HAL_I2C_ERROR_BERR | HAL_I2C_ERROR_ARLO |
                            HAL_I2C_ERROR_OVR)) != 0U)
        return BSP_IIC_ERR_BUS;

    if ((hi2c->ErrorCode & HAL_I2C_ERROR_TIMEOUT) != 0U)
        return BSP_IIC_ERR_TIMEOUT;

    return BSP_IIC_ERR_HAL;
}

/**
 * @brief 按命令模式启动一次 HAL 轮询传输
 * @param bus     总线对象，不可为 NULL
 * @param msg     传输命令，不可为 NULL
 * @param timeout 超时（ms）
 * @retval HAL 状态
 * @note 7 bit 地址在这里左移成 HAL 需要的 8 bit 形式；
 *       MEM 模式用 HAL_I2C_Mem_*，子地址写与 repeated START 由 HAL 完成。
 */
static HAL_StatusTypeDef BSP_IIC_StartHal(bsp_iic_t *bus,
                                          const bsp_iic_msg_t *msg,
                                          uint32_t timeout)
{
    I2C_HandleTypeDef *hi2c = (I2C_HandleTypeDef *)bus->hal;
    uint16_t dev_addr = (uint16_t)(msg->dev_addr << 1);
    uint16_t mem_size;

    if (msg->mode == (uint8_t)BSP_IIC_MODE_MEM)
    {
        mem_size = (msg->mem_addr_size == 2U) ? I2C_MEMADD_SIZE_16BIT
                                              : I2C_MEMADD_SIZE_8BIT;
        if (msg->dir == (uint8_t)BSP_IIC_DIR_TX)
            return HAL_I2C_Mem_Write(hi2c, dev_addr, msg->mem_addr, mem_size,
                                     msg->data, msg->len, timeout);
        return HAL_I2C_Mem_Read(hi2c, dev_addr, msg->mem_addr, mem_size,
                                msg->data, msg->len, timeout);
    }

    if (msg->dir == (uint8_t)BSP_IIC_DIR_TX)
        return HAL_I2C_Master_Transmit(hi2c, dev_addr, msg->data, msg->len,
                                       timeout);
    return HAL_I2C_Master_Receive(hi2c, dev_addr, msg->data, msg->len,
                                  timeout);
}

/*==============================================================================
 * 接口实现
 *============================================================================*/
/**
 * @brief 初始化 IIC 驱动
 * @retval BSP_IIC_OK: 初始化成功
 * @retval BSP_IIC_ERR_STATE: HAL 句柄缺失、互斥量未创建或外设未就绪
 */
bsp_iic_status_t BSP_IIC_Init(void)
{
    size_t i;

    /* ---------- 逐路登记：句柄、锁（可选）、外设状态 ---------- */
    for (i = 0U; i < (sizeof(s_buses) / sizeof(s_buses[0])); i++)
    {
        bsp_iic_t *bus = s_buses[i];

        if (bus->initialized != 0U)
            continue;

        if (bus->hal == NULL)
            return BSP_IIC_ERR_STATE;

        /* 配了锁就必须已经创建；没配锁则不加锁 */
        if ((bus->lock != NULL) && (*bus->lock == NULL))
            return BSP_IIC_ERR_STATE;

        if (HAL_I2C_GetState((I2C_HandleTypeDef *)bus->hal) !=
            HAL_I2C_STATE_READY)
            return BSP_IIC_ERR_STATE;

        bus->initialized = 1U;
    }

    return BSP_IIC_OK;
}

/**
 * @brief 提交一次 IIC 传输并等它完成
 * @param bus 总线句柄，不能为 NULL
 * @param msg 传输命令，不能为 NULL
 * @retval BSP_IIC_OK: 传输成功
 * @retval BSP_IIC_ERR_PARAM: 参数非法
 * @retval BSP_IIC_ERR_STATE: 驱动未初始化
 * @retval BSP_IIC_ERR_TIMEOUT: 等锁或传输超时
 * @retval BSP_IIC_ERR_NACK: 从机 NACK
 * @retval BSP_IIC_ERR_BUS: 总线错误
 * @retval BSP_IIC_ERR_HAL: 其他 HAL 错误
 */
bsp_iic_status_t BSP_IIC_Transfer(bsp_iic_t *bus, const bsp_iic_msg_t *msg)
{
    I2C_HandleTypeDef *hi2c;
    HAL_StatusTypeDef  st;
    bsp_iic_status_t   status;
    uint32_t           timeout_ms;
    uint8_t            locked = 0U;

    if ((bus == NULL) || (msg == NULL))
        return BSP_IIC_ERR_PARAM;
    if (bus->initialized == 0U)
        return BSP_IIC_ERR_STATE;

    if ((msg->data == NULL) || (msg->len == 0U))
        return BSP_IIC_ERR_PARAM;
    if (msg->dev_addr > BSP_IIC_ADDR_MAX)
        return BSP_IIC_ERR_PARAM;
    if ((msg->dir > (uint8_t)BSP_IIC_DIR_RX) ||
        (msg->mode > (uint8_t)BSP_IIC_MODE_MEM))
        return BSP_IIC_ERR_PARAM;
    if ((msg->mode == (uint8_t)BSP_IIC_MODE_MEM) &&
        (msg->mem_addr_size != 1U) && (msg->mem_addr_size != 2U))
        return BSP_IIC_ERR_PARAM;

    timeout_ms = (msg->timeout_ms != 0U) ? msg->timeout_ms
                                         : BSP_IIC_DEFAULT_TIMEOUT_MS;

    /* ---------- 1. 抢总线：抢不到说明排队已经超时 ---------- */
    if ((bus->lock != NULL) && (*bus->lock != NULL))
    {
        if (osMutexAcquire(*bus->lock, pdMS_TO_TICKS(timeout_ms)) != osOK)
            return BSP_IIC_ERR_TIMEOUT;
        locked = 1U;
    }

    /* ---------- 2. 传输 ---------- */
    hi2c   = (I2C_HandleTypeDef *)bus->hal;
    st     = BSP_IIC_StartHal(bus, msg, timeout_ms);
    status = BSP_IIC_MapStatus(hi2c, st);

    /* ---------- 3. 放总线 ---------- */
    if (locked != 0U)
        (void)osMutexRelease(*bus->lock);

    return status;
}

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
                               const uint8_t *data, uint16_t len)
{
    bsp_iic_msg_t msg;

    msg.dev_addr      = dev_addr;
    msg.mem_addr      = 0U;
    msg.mem_addr_size = 1U;
    msg.len           = len;
    msg.dir           = (uint8_t)BSP_IIC_DIR_TX;
    msg.mode          = (uint8_t)BSP_IIC_MODE_RAW;
    msg.data          = (uint8_t *)data;   /* HAL 接口不收 const，写入方向不改数据 */
    msg.timeout_ms    = 0U;

    return BSP_IIC_Transfer(bus, &msg);
}

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
                              uint8_t *data, uint16_t len)
{
    bsp_iic_msg_t msg;

    msg.dev_addr      = dev_addr;
    msg.mem_addr      = 0U;
    msg.mem_addr_size = 1U;
    msg.len           = len;
    msg.dir           = (uint8_t)BSP_IIC_DIR_RX;
    msg.mode          = (uint8_t)BSP_IIC_MODE_RAW;
    msg.data          = data;
    msg.timeout_ms    = 0U;

    return BSP_IIC_Transfer(bus, &msg);
}

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
                                  uint16_t len)
{
    bsp_iic_msg_t msg;

    msg.dev_addr      = dev_addr;
    msg.mem_addr      = reg;
    msg.mem_addr_size = 1U;
    msg.len           = len;
    msg.dir           = (uint8_t)BSP_IIC_DIR_TX;
    msg.mode          = (uint8_t)BSP_IIC_MODE_MEM;
    msg.data          = (uint8_t *)data;
    msg.timeout_ms    = 0U;

    return BSP_IIC_Transfer(bus, &msg);
}

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
                                 uint16_t len)
{
    bsp_iic_msg_t msg;

    msg.dev_addr      = dev_addr;
    msg.mem_addr      = reg;
    msg.mem_addr_size = 1U;
    msg.len           = len;
    msg.dir           = (uint8_t)BSP_IIC_DIR_RX;
    msg.mode          = (uint8_t)BSP_IIC_MODE_MEM;
    msg.data          = data;
    msg.timeout_ms    = 0U;

    return BSP_IIC_Transfer(bus, &msg);
}
