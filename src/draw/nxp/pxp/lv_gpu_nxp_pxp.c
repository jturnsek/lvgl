/**
 * @file lv_gpu_nxp_pxp.c
 *
 */

/**
 * MIT License
 *
 * Copyright 2020-2023 NXP
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

#include "lv_gpu_nxp_pxp.h"

#if LV_USE_GPU_NXP_PXP
#include "lv_gpu_nxp_pxp_osa.h"
#include "../../../core/lv_refr.h"

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
 * Clean and invalidate cache.
 */
static inline void invalidate_cache(void);

/**********************
 *  STATIC VARIABLES
 **********************/

static lv_nxp_pxp_cfg_t * pxp_cfg;

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_res_t lv_gpu_nxp_pxp_init(void)
{
#if LV_USE_GPU_NXP_PXP_AUTO_INIT
    pxp_cfg = lv_gpu_nxp_pxp_get_cfg();
#endif

    if(!pxp_cfg || !pxp_cfg->pxp_interrupt_deinit || !pxp_cfg->pxp_interrupt_init ||
       !pxp_cfg->pxp_run || !pxp_cfg->pxp_wait)
        PXP_RETURN_INV("PXP configuration error.");

    pxp_init();

    pxp_enable_csc1(false); /*Disable CSC1, it is enabled by default.*/
    pxp_set_process_block_size(PXP_BLOCK_SIZE16); /*Block size 16x16 for higher performance*/

    pxp_enable_interrupts(PXP_COMPLETE_INTERRUPT_ENABLE);

    if(pxp_cfg->pxp_interrupt_init() != LV_RES_OK) {
        pxp_disable_interrupts(PXP_COMPLETE_INTERRUPT_ENABLE);
        PXP_RETURN_INV("PXP interrupt init failed.");
    }

    return LV_RES_OK;
}

void lv_gpu_nxp_pxp_deinit(void)
{
    pxp_cfg->pxp_interrupt_deinit();
    pxp_disable_interrupts(PXP_COMPLETE_INTERRUPT_ENABLE);
}

void lv_gpu_nxp_pxp_reset(void)
{
    /* Wait for previous command to complete before resetting the registers. */
    lv_gpu_nxp_pxp_wait();

    pxp_reset_control();

    pxp_enable_csc1(false); /*Disable CSC1, it is enabled by default.*/
    pxp_set_process_block_size(PXP_BLOCK_SIZE16); /*Block size 16x16 for higher performance*/
}

void lv_gpu_nxp_pxp_run(void)
{
    //invalidate_cache();

    pxp_cfg->pxp_run();
}

void lv_gpu_nxp_pxp_wait(void)
{
    pxp_cfg->pxp_wait();
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static inline void invalidate_cache(void)
{
    lv_disp_t * disp = _lv_refr_get_disp_refreshing();
    if(disp->driver->clean_dcache_cb)
        disp->driver->clean_dcache_cb(disp->driver);
}

#endif /*LV_USE_GPU_NXP_PXP*/
