/* port/sys_arch.c — lwIP FreeRTOS system layer */
#include "lwip/opt.h"
#include "lwip/debug.h"
#include "lwip/def.h"
#include "lwip/sys.h"
#include "lwip/mem.h"
#include "lwip/stats.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#if !NO_SYS

/* ── Mailboxes ──────────────────────────────────────────────────────────── */

err_t sys_mbox_new(sys_mbox_t *mbox, int size)
{
    *mbox = xQueueCreate((UBaseType_t)size, sizeof(void *));
    if (*mbox == NULL) return ERR_MEM;
    return ERR_OK;
}

void sys_mbox_free(sys_mbox_t *mbox)
{
    vQueueDelete(*mbox);
}

void sys_mbox_post(sys_mbox_t *mbox, void *msg)
{
    while (xQueueSendToBack(*mbox, &msg, portMAX_DELAY) != pdTRUE);
}

err_t sys_mbox_trypost(sys_mbox_t *mbox, void *msg)
{
    if (xQueueSendToBack(*mbox, &msg, 0) == pdTRUE) return ERR_OK;
    return ERR_MEM;
}

err_t sys_mbox_trypost_fromisr(sys_mbox_t *mbox, void *msg)
{
    BaseType_t woken = pdFALSE;
    if (xQueueSendToBackFromISR(*mbox, &msg, &woken) == pdTRUE) {
        portYIELD_FROM_ISR(woken);
        return ERR_OK;
    }
    return ERR_MEM;
}

u32_t sys_arch_mbox_fetch(sys_mbox_t *mbox, void **msg, u32_t timeout_ms)
{
    void *dummy;
    if (msg == NULL) msg = &dummy;

    TickType_t ticks = (timeout_ms == 0) ? portMAX_DELAY
                                         : pdMS_TO_TICKS(timeout_ms);
    TickType_t t0 = xTaskGetTickCount();

    if (xQueueReceive(*mbox, msg, ticks) == pdTRUE) {
        return (u32_t)((xTaskGetTickCount() - t0) * portTICK_PERIOD_MS);
    }
    *msg = NULL;
    return SYS_ARCH_TIMEOUT;
}

u32_t sys_arch_mbox_tryfetch(sys_mbox_t *mbox, void **msg)
{
    void *dummy;
    if (msg == NULL) msg = &dummy;
    if (xQueueReceive(*mbox, msg, 0) == pdTRUE) return 0;
    *msg = NULL;
    return SYS_MBOX_EMPTY;
}

int sys_mbox_valid(sys_mbox_t *mbox) { return (*mbox != NULL) ? 1 : 0; }
void sys_mbox_set_invalid(sys_mbox_t *mbox) { *mbox = NULL; }

/* ── Semaphores ─────────────────────────────────────────────────────────── */

err_t sys_sem_new(sys_sem_t *sem, u8_t count)
{
    *sem = xSemaphoreCreateCounting(255, (UBaseType_t)count);
    if (*sem == NULL) return ERR_MEM;
    return ERR_OK;
}

void sys_sem_free(sys_sem_t *sem)   { vSemaphoreDelete(*sem); }
void sys_sem_signal(sys_sem_t *sem) { xSemaphoreGive(*sem); }

u32_t sys_arch_sem_wait(sys_sem_t *sem, u32_t timeout_ms)
{
    TickType_t ticks = (timeout_ms == 0) ? portMAX_DELAY
                                         : pdMS_TO_TICKS(timeout_ms);
    TickType_t t0 = xTaskGetTickCount();
    if (xSemaphoreTake(*sem, ticks) == pdTRUE) {
        return (u32_t)((xTaskGetTickCount() - t0) * portTICK_PERIOD_MS);
    }
    return SYS_ARCH_TIMEOUT;
}

int sys_sem_valid(sys_sem_t *sem) { return (*sem != NULL) ? 1 : 0; }
void sys_sem_set_invalid(sys_sem_t *sem) { *sem = NULL; }

/* ── Mutexes ─────────────────────────────────────────────────────────────── */

err_t sys_mutex_new(sys_mutex_t *mutex)
{
    *mutex = xSemaphoreCreateMutex();
    if (*mutex == NULL) return ERR_MEM;
    return ERR_OK;
}

void sys_mutex_free(sys_mutex_t *mutex)  { vSemaphoreDelete(*mutex); }
void sys_mutex_lock(sys_mutex_t *mutex)  { xSemaphoreTake(*mutex, portMAX_DELAY); }
void sys_mutex_unlock(sys_mutex_t *mutex){ xSemaphoreGive(*mutex); }
int  sys_mutex_valid(sys_mutex_t *mutex) { return (*mutex != NULL) ? 1 : 0; }
void sys_mutex_set_invalid(sys_mutex_t *mutex) { *mutex = NULL; }

/* ── Threads ─────────────────────────────────────────────────────────────── */

sys_thread_t sys_thread_new(const char *name, lwip_thread_fn fn,
                             void *arg, int stacksize, int prio)
{
    TaskHandle_t h = NULL;
    xTaskCreate(fn, name, (configSTACK_DEPTH_TYPE)stacksize, arg,
                (UBaseType_t)prio, &h);
    return h;
}

/* ── Time ────────────────────────────────────────────────────────────────── */

void sys_init(void)
{
    /* Nothing to initialise — FreeRTOS already running */
}

u32_t sys_now(void)
{
    return (u32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
}

/* ── Critical section ────────────────────────────────────────────────────── */

sys_prot_t sys_arch_protect(void)
{
    taskENTER_CRITICAL();
    return 0;
}

void sys_arch_unprotect(sys_prot_t pval)
{
    (void)pval;
    taskEXIT_CRITICAL();
}

#endif /* !NO_SYS */
