/**
 * @file cmsis_os2.h
 * @brief 主机端测试替身：只提供被测文件用到的 CMSIS-RTOS2 接口
 * @author 李嘉图
 * @date 2026-10-31
 *
 * @note 仅用于在 PC 上编译检查 Service 层，不参与固件构建。
 *       固件编的是 CubeMX 带进来的真实 cmsis_os2.h。
 */

#ifndef __CMSIS_OS2_H__
#define __CMSIS_OS2_H__

#include <stdint.h>

typedef void *osMessageQueueId_t;
typedef void *osSemaphoreId_t;
typedef void *osMutexId_t;

typedef enum {
    osOK             =  0,
    osError          = -1,
    osErrorTimeout   = -2,
    osErrorResource  = -3,
    osErrorParameter = -4
} osStatus_t;

#define osWaitForever   0xFFFFFFFFU

osStatus_t osMessageQueuePut(osMessageQueueId_t mq_id, const void *msg_ptr,
                             uint8_t msg_prio, uint32_t timeout);
osStatus_t osMessageQueueGet(osMessageQueueId_t mq_id, void *msg_ptr,
                             uint8_t *msg_prio, uint32_t timeout);
osStatus_t osSemaphoreAcquire(osSemaphoreId_t sem_id, uint32_t timeout);
osStatus_t osSemaphoreRelease(osSemaphoreId_t sem_id);
osStatus_t osMutexAcquire(osMutexId_t mutex_id, uint32_t timeout);
osStatus_t osMutexRelease(osMutexId_t mutex_id);
uint32_t   osKernelGetTickCount(void);

#endif /* __CMSIS_OS2_H__ */
