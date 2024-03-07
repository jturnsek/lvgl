/**
 * @file lv_gpu_nxp_pxp_osa.c
 *
 */

/**
 * MIT License
 *
 * Copyright 2020, 2022, 2023 NXP
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next paragraph)
 * shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

/*********************
 *      INCLUDES
 *********************/

#include "lv_gpu_nxp_pxp_osa.h"

#if LV_USE_GPU_NXP_PXP && LV_USE_GPU_NXP_PXP_AUTO_INIT
#include "../../../misc/lv_log.h"
#include "imxrt_pxp.h"

#if defined(SDK_OS_FREE_RTOS)
    #include "FreeRTOS.h"
    #include "semphr.h"
#endif

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**
 * PXP interrupt initialization.
 */
static lv_res_t _lv_gpu_nxp_pxp_interrupt_init(void);

/**
 * PXP interrupt de-initialization.
 */
static void _lv_gpu_nxp_pxp_interrupt_deinit(void);

/**
 * Start the PXP job.
 */
static void _lv_gpu_nxp_pxp_run(void);

/**
 * Wait for PXP completion.
 */
static void _lv_gpu_nxp_pxp_wait(void);

/**********************
 *  STATIC VARIABLES
 **********************/

#if defined(SDK_OS_FREE_RTOS)
    static SemaphoreHandle_t s_pxpIdleSem;
#endif
static volatile bool s_pxpIdle;

static lv_nxp_pxp_cfg_t pxp_default_cfg = {
    .pxp_interrupt_init = _lv_gpu_nxp_pxp_interrupt_init,
    .pxp_interrupt_deinit = _lv_gpu_nxp_pxp_interrupt_deinit,
    .pxp_run = _lv_gpu_nxp_pxp_run,
    .pxp_wait = _lv_gpu_nxp_pxp_wait,
};

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

static int imxrt_pxp_interrupt(int irq, void *context, void *arg)
{
#if defined(SDK_OS_FREE_RTOS)
    BaseType_t taskAwake = pdFALSE;
#endif

    if(PXP_COMPLETE_FLAG & pxp_get_status_flags()) {
        pxp_clear_status_flags(PXP_COMPLETE_FLAG);
#if defined(SDK_OS_FREE_RTOS)
        xSemaphoreGiveFromISR(s_pxpIdleSem, &taskAwake);
        portYIELD_FROM_ISR(taskAwake);
#else
        s_pxpIdle = true;
#endif
    }

    return 0;
}

lv_nxp_pxp_cfg_t * lv_gpu_nxp_pxp_get_cfg(void)
{
    return &pxp_default_cfg;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static lv_res_t _lv_gpu_nxp_pxp_interrupt_init(void)
{
#if defined(SDK_OS_FREE_RTOS)
    s_pxpIdleSem = xSemaphoreCreateBinary();
    if(s_pxpIdleSem == NULL)
        return LV_RES_INV;

    NVIC_SetPriority(LV_GPU_NXP_PXP_IRQ_ID, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 1);
#endif
    s_pxpIdle = true;

    irq_attach(LV_GPU_NXP_PXP_IRQ_ID, imxrt_pxp_interrupt, NULL);
    up_enable_irq(LV_GPU_NXP_PXP_IRQ_ID);

    return LV_RES_OK;
}

static void _lv_gpu_nxp_pxp_interrupt_deinit(void)
{
    up_disable_irq(LV_GPU_NXP_PXP_IRQ_ID);
#if defined(SDK_OS_FREE_RTOS)
    vSemaphoreDelete(s_pxpIdleSem);
#endif
}

/**
 * Function to start PXP job.
 */
static void _lv_gpu_nxp_pxp_run(void)
{
    s_pxpIdle = false;

    pxp_enable_interrupts(PXP_COMPLETE_INTERRUPT_ENABLE);
    pxp_start();
}

/**
 * Function to wait for PXP completion.
 */
static void _lv_gpu_nxp_pxp_wait(void)
{
#if defined(SDK_OS_FREE_RTOS)
    /* Return if PXP was never started, otherwise the semaphore will lock forever. */
    if(s_pxpIdle == true)
        return;

    if(xSemaphoreTake(s_pxpIdleSem, portMAX_DELAY) == pdTRUE)
        s_pxpIdle = true;
#else
    while(s_pxpIdle == false) {
    }
#endif
}

#endif /*LV_USE_GPU_NXP_PXP && LV_USE_GPU_NXP_PXP_AUTO_INIT*/
