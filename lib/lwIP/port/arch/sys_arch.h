/* port/arch/sys_arch.h — lwIP FreeRTOS sys_arch types */
#pragma once

#include "lwip/opt.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SYS_MBOX_NULL  ((QueueHandle_t)NULL)
#define SYS_SEM_NULL   ((SemaphoreHandle_t)NULL)

typedef SemaphoreHandle_t sys_sem_t;
typedef SemaphoreHandle_t sys_mutex_t;
typedef QueueHandle_t     sys_mbox_t;
typedef TaskHandle_t      sys_thread_t;

#ifdef __cplusplus
}
#endif
