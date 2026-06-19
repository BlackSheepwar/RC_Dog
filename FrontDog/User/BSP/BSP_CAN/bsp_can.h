/**
 * @file bsp_can.h
 * @brief CAN硬件接口
 * @author 李嘉图
 * @date 2026-06-01
 *
 * @note 硬件映射表（id → hcan）在此文件中静态定义，
 *       支持双 CAN（hcan1/hcan2），滤波器 28 个平均分配。
 *       BSP层仅负责硬件配置与数据收发，不处理业务逻辑。
 *
 *       滤波器配置通过 id1~id4 直接映射 bxCAN 的 4 个寄存器字段，
 *       配合 mode/scale 组合出 4 种模式:
 *       规则组+标准(16bit MASK)、规则组+扩展(32bit MASK)
 *       通道组+标准(16bit LIST)、通道组+扩展(32bit LIST)
 */

#ifndef __BSP_CAN_H__
#define __BSP_CAN_H__

/*==============================================================================
 * 头文件包含
 *============================================================================*/
// 固定包含
#include <stdint.h>
// 功能包含
#include "can.h"

/*==============================================================================
 * 宏定义与常量
 *============================================================================*/
#define BSP_CAN_MAX_NUM         2           // CAN实例最大数量 (CAN1 + CAN2)
#define BSP_CAN_DATA_LEN        8           // CAN数据帧最大长度

// 滤波器模式
#define BSP_CAN_FILTER_MODE_MASK   CAN_FILTERMODE_IDMASK    // 规则组
#define BSP_CAN_FILTER_MODE_LIST   CAN_FILTERMODE_IDLIST    // 通道组

// 滤波器位宽
#define BSP_CAN_FILTER_SCALE_16    CAN_FILTERSCALE_16BIT    // 标准帧 (4个16位槽位)
#define BSP_CAN_FILTER_SCALE_32    CAN_FILTERSCALE_32BIT    // 扩展帧 (2个32位槽位)

// FIFO选择
#define BSP_CAN_FIFO0           CAN_RX_FIFO0                // FIFO0
#define BSP_CAN_FIFO1           CAN_RX_FIFO1                // FIFO1

// ID类型
#define BSP_CAN_STDID           CAN_ID_STD                  // 标准帧ID
#define BSP_CAN_EXTID           CAN_ID_EXT                  // 扩展帧ID

// 帧类型
#define BSP_CAN_DATA_FRAME      CAN_RTR_DATA                // 数据帧类型
#define BSP_CAN_REMOTE_FRAME    CAN_RTR_REMOTE              // 遥控帧类型

/*==============================================================================
 * 标准帧 16-bit 槽位格式宏
 *   bxCAN 16-bit 寄存器: [15:5]=STDID, [3]=RTR
 *   IDE位标准帧恒为0，无需配置
 *============================================================================*/
#define BSP_CAN_STD16_ID(id)            ((uint16_t)(((id) & 0x7FF) << 5))
#define BSP_CAN_STD16_RTR(rtr)          ((uint16_t)((rtr) ? (1 << 3) : 0))
#define BSP_CAN_STD16_IDE(ide)          ((uint16_t)((ide) ? (1 << 2) : 0))
#define BSP_CAN_STD16(id, rtr, ide)     ((uint16_t)(BSP_CAN_STD16_ID(id) | BSP_CAN_STD16_RTR(rtr) | BSP_CAN_STD16_IDE(ide)))

/*==============================================================================
 * 扩展帧 32-bit 高低16位格式宏
 *   高位[15:5]=EXTID[28:18], [4]=SRR, [3]=IDE, [2]=RTR, [1:0]=EXTID[17:16]
 *   低位[15:0]=EXTID[15:0]
 *============================================================================*/
#define BSP_CAN_EXT32_HIGH(id, rtr)  ((uint16_t)( \
    ((((uint32_t)(id) >> 18) & 0x7FF) << 5) | \
    (1 << 4) | (1 << 3) | \
    (((rtr) ? 1 : 0) << 2) | \
    (((uint32_t)(id) >> 16) & 0x03) ))
#define BSP_CAN_EXT32_LOW(id)           ((uint16_t)((uint32_t)(id) & 0xFFFF))

/*==============================================================================
 * 结构体定义
 *============================================================================*/

/**
 * @brief CAN滤波器配置参数
 *
 * @note id1~id4 直接映射 bxCAN 滤波器的 4 个寄存器字段:
 *         id1 → FilterIdHigh    (CAN_FxR1高16位)
 *         id2 → FilterIdLow     (CAN_FxR1低16位)
 *         id3 → FilterMaskIdHigh (CAN_FxR2高16位)
 *         id4 → FilterMaskIdLow  (CAN_FxR2低16位)
 *
 *       配合 mode×scale 组合出4种模式:
 *
 *       规则组+标准 (MASK+16bit):
 *         id1=ID1, id2=MASK1, id3=ID2, id4=MASK2  (2条规则)
 *       规则组+扩展 (MASK+32bit):
 *         id1=ID高位, id2=ID低位, id3=Mask高位, id4=Mask低位
 *       通道组+标准 (LIST+16bit):
 *         id1=ID1, id2=ID2, id3=ID3, id4=ID4      (4个独立ID)
 *       通道组+扩展 (LIST+32bit):
 *         id1=ID1高位, id2=ID1低位, id3=ID2高位, id4=ID2低位
 */
typedef struct {
    uint8_t  filter_bank;   // 滤波器序号 (0-13=CAN1, 14-27=CAN2)
    uint8_t  fifo;          // 目标FIFO (BSP_CAN_FIFO0 / BSP_CAN_FIFO1)
    uint8_t  mode;          // BSP_CAN_FILTER_MODE_MASK / _LIST
    uint8_t  scale;         // BSP_CAN_FILTER_SCALE_16 / _32
    uint16_t id1;           // → FilterIdHigh  (CAN_FxR1[31:16])
    uint16_t id2;           // → FilterIdLow   (CAN_FxR1[15:0])
    uint16_t id3;           // → FilterMaskIdHigh (CAN_FxR2[31:16])
    uint16_t id4;           // → FilterMaskIdLow  (CAN_FxR2[15:0])
} BSP_CAN_FilterConfig_t;

/**
 * @brief can指令包结构体
 */
typedef struct{
    uint8_t  id;
    uint8_t data[8];
    CAN_RxHeaderTypeDef header;
} BSP_CAN_Packet_t;

/*==============================================================================
 * 初始化函数
 *============================================================================*/
/**
 * @brief 初始化BSP CAN层
 * @note 复位运行时启动状态。映射表为编译期常量，无需注册。
 */
void BSP_CAN_Init(void);

/**
 * @brief 根据CAN句柄获取实例ID
 * @param hcan CAN硬件句柄
 * @retval 0~BSP_CAN_MAX_NUM-1: 对应实例ID
 * @retval 0xFF: 未找到
 */
uint8_t BSP_CAN_GetIdByHcan(CAN_HandleTypeDef *hcan);

/*==============================================================================
 * 控制函数
 *============================================================================*/
/**
 * @brief 启动CAN通信
 * @param id CAN实例ID
 * @retval 1: 启动成功
 * @retval 0: 启动失败
 *
 * @note 启动后自动使能 FIFO0/FIFO1 消息挂起中断及溢出中断。
 *       重复调用安全，已启动则直接返回成功。
 */
uint8_t BSP_CAN_Start(uint8_t id);

/*==============================================================================
 * 滤波器配置
 *============================================================================*/
/**
 * @brief 配置CAN滤波器
 * @param id  CAN实例ID
 * @param cfg 滤波器配置参数（id1~id4直通寄存器，见 BSP_CAN_FilterConfig_t 说明）
 * @retval 1: 配置成功
 * @retval 0: 配置失败（实例未找到/cfg为空/滤波器序号>27/HAL配置失败）
 *
 * @note id1~id4 直接透传到 HAL，用户通过宏自行组装:
 *         标准帧 16bit → BSP_CAN_STD16(id, rtr)
 *         扩展帧 32bit → BSP_CAN_EXT32_HIGH(id, rtr) / BSP_CAN_EXT32_LOW(id)
 */
uint8_t BSP_CAN_FilterConfig(uint8_t id, const BSP_CAN_FilterConfig_t *cfg);

/*==============================================================================
 * 发送函数
 *============================================================================*/
/**
 * @brief 发送CAN帧（支持 标准/扩展 × 数据/遥控 四种组合）
 * @param id       CAN实例ID
 * @param can_id   ID值（标准帧11bit / 扩展帧29bit）
 * @param is_extid ID类型: BSP_CAN_STDID / BSP_CAN_EXTID
 * @param is_remote 帧类型: BSP_CAN_DATA_FRAME / BSP_CAN_REMOTE_FRAME
 * @param data     数据指针（遥控帧传 NULL）
 * @param len      数据长度 (0~8, 遥控帧时表示期望返回的DLC)
 * @retval 1: 发送成功
 * @retval 0: 发送失败（实例未找到/参数错误/邮箱满）
 *
 * @note 非阻塞发送，通过HAL_CAN_AddTxMessage将帧加入硬件发送邮箱。
 */
uint8_t BSP_CAN_SendMsg(uint8_t id, uint32_t can_id, uint8_t is_extid, uint8_t is_remote, uint8_t *data, uint8_t len);

#endif
