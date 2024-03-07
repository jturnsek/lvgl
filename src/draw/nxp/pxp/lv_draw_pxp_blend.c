/**
 * @file lv_draw_pxp_blend.c
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

#include "lv_draw_pxp_blend.h"

#if LV_USE_GPU_NXP_PXP
//#include "lvgl_support.h"

/*********************
 *      DEFINES
 *********************/

#if LV_COLOR_16_SWAP
    #error Color swap not implemented. Disable LV_COLOR_16_SWAP feature.
#endif

#if LV_COLOR_DEPTH == 16
    #define PXP_OUT_PIXEL_FORMAT PXP_OUTPUT_PIXEL_FORMAT_RGB565
    #define PXP_AS_PIXEL_FORMAT PXP_AS_PIXEL_FORMAT_RGB565
    #define PXP_PS_PIXEL_FORMAT PXP_PS_PIXEL_FORMAT_RGB565
#elif LV_COLOR_DEPTH == 32
    #define PXP_OUT_PIXEL_FORMAT PXP_OUTPUT_PIXEL_FORMAT_ARGB8888
    #define PXP_AS_PIXEL_FORMAT PXP_AS_PIXEL_FORMAT_ARGB8888
    #if (!(defined(FSL_FEATURE_PXP_HAS_NO_EXTEND_PIXEL_FORMAT) && FSL_FEATURE_PXP_HAS_NO_EXTEND_PIXEL_FORMAT)) && \
        (!(defined(FSL_FEATURE_PXP_V3) && FSL_FEATURE_PXP_V3))
        #define PXP_PS_PIXEL_FORMAT PXP_PS_PIXEL_FORMAT_ARGB8888
    #else
        #define PXP_PS_PIXEL_FORMAT PXP_PS_PIXEL_FORMAT_RGB888
    #endif
#elif
    #error Only 16bit and 32bit color depth are supported. Set LV_COLOR_DEPTH to 16 or 32.
#endif

#define ALIGN_SIZE 4

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**
 * BLock Image Transfer - copy rectangular image from src buffer to dst buffer
 * with combination of transformation (rotation, scale, recolor) and opacity, alpha channel
 * or color keying. This requires two steps. First step is used for transformation into
 * a temporary buffer and the second one will handle the color format or opacity.
 *
 * @param[in/out] dest_buf Destination buffer
 * @param[in] dest_area Area with relative coordinates of destination buffer
 * @param[in] dest_stride Stride of destination buffer in pixels
 * @param[in] src_buf Source buffer
 * @param[in] src_area Area with relative coordinates of source buffer
 * @param[in] src_stride Stride of source buffer in pixels
 * @param[in] dsc Image descriptor
 * @param[in] cf Color format
 */
static void lv_pxp_blit_opa(lv_color_t * dest_buf, const lv_area_t * dest_area, lv_coord_t dest_stride,
                            const lv_color_t * src_buf, const lv_area_t * src_area, lv_coord_t src_stride,
                            const lv_draw_img_dsc_t * dsc, lv_img_cf_t cf);

/**
 * BLock Image Transfer - copy rectangular image from src buffer to dst buffer
 * with transformation and full opacity.
 *
 * @param[in/out] dest_buf Destination buffer
 * @param[in] dest_area Area with relative coordinates of destination buffer
 * @param[in] dest_stride Stride of destination buffer in pixels
 * @param[in] src_buf Source buffer
 * @param[in] src_area Area with relative coordinates of source buffer
 * @param[in] src_stride Stride of source buffer in pixels
 * @param[in] dsc Image descriptor
 * @param[in] cf Color format
 */
static void lv_pxp_blit_cover(lv_color_t * dest_buf, lv_area_t * dest_area, lv_coord_t dest_stride,
                              const lv_color_t * src_buf, const lv_area_t * src_area, lv_coord_t src_stride,
                              const lv_draw_img_dsc_t * dsc, lv_img_cf_t cf);

/**
 * BLock Image Transfer - copy rectangular image from src buffer to dst buffer
 * without transformation but handling color format or opacity.
 *
 * @param[in/out] dest_buf Destination buffer
 * @param[in] dest_area Area with relative coordinates of destination buffer
 * @param[in] dest_stride Stride of destination buffer in pixels
 * @param[in] src_buf Source buffer
 * @param[in] src_area Area with relative coordinates of source buffer
 * @param[in] src_stride Stride of source buffer in pixels
 * @param[in] dsc Image descriptor
 * @param[in] cf Color format
 */
static void lv_pxp_blit_cf(lv_color_t * dest_buf, const lv_area_t * dest_area, lv_coord_t dest_stride,
                           const lv_color_t * src_buf, const lv_area_t * src_area, lv_coord_t src_stride,
                           const lv_draw_img_dsc_t * dsc, lv_img_cf_t cf);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_gpu_nxp_pxp_fill(lv_color_t * dest_buf, const lv_area_t * dest_area, lv_coord_t dest_stride,
                         lv_color_t color, lv_opa_t opa)
{
    lv_coord_t dest_w = lv_area_get_width(dest_area);
    lv_coord_t dest_h = lv_area_get_height(dest_area);

    lv_gpu_nxp_pxp_reset();

    /*OUT buffer configure*/
    struct pxp_output_buffer_config_s outputConfig = {
        .pixel_format = PXP_OUT_PIXEL_FORMAT,
        .interlaced_mode = PXP_OUTPUT_PROGRESSIVE,
        .buffer0_addr = (uint32_t)(dest_buf + dest_stride * dest_area->y1 + dest_area->x1),
        .buffer1_addr = (uint32_t)NULL,
        .pitch_bytes = dest_stride * sizeof(lv_color_t),
        .width = dest_w,
        .height = dest_h
    };

    pxp_set_output_buffer_config(&outputConfig);

    if(opa >= (lv_opa_t)LV_OPA_MAX) {
        /*Simple color fill without opacity - AS disabled*/
        pxp_set_alpha_surface_position(0xFFFFU, 0xFFFFU, 0U, 0U);

    }
    else {
        /*Fill with opacity - AS used as source (same as OUT)*/
        struct pxp_as_buffer_config_s asBufferConfig = {
            .pixel_format = PXP_AS_PIXEL_FORMAT,
            .buffer_addr = (uint32_t)outputConfig.buffer0_addr,
            .pitch_bytes = outputConfig.pitch_bytes
        };

        pxp_set_alpha_surface_buffer_config(&asBufferConfig);
        pxp_set_alpha_surface_position(0U, 0U, dest_w - 1U, dest_h - 1U);
    }

    /*Disable PS, use as color generator*/
    pxp_set_process_surface_position(0xFFFFU, 0xFFFFU, 0U, 0U);
    pxp_set_process_surface_back_ground_color(lv_color_to32(color));

    /**
     * Configure Porter-Duff blending - src settings are unused for fill without opacity (opa = 0xff).
     *
     * Note: srcFactorMode and dstFactorMode are inverted in fsl_pxp.h:
     * srcFactorMode is actually applied on PS alpha value
     * dstFactorMode is actually applied on AS alpha value
     */
    struct pxp_porter_duff_config_s pdConfig = {
        .enable = 1,
        .dst_color_mode = PXP_PORTER_DUFF_COLOR_NO_ALPHA,
        .src_color_mode = PXP_PORTER_DUFF_COLOR_NO_ALPHA,
        .dst_global_alpha_mode = PXP_PORTER_DUFF_GLOBAL_ALPHA,
        .src_global_alpha_mode = PXP_PORTER_DUFF_GLOBAL_ALPHA,
        .dst_factor_mode = PXP_PORTER_DUFF_FACTOR_STRAIGHT,
        .src_factor_mode = (opa >= (lv_opa_t)LV_OPA_MAX) ? PXP_PORTER_DUFF_FACTOR_STRAIGHT : PXP_PORTER_DUFF_FACTOR_INVERSED,
        .dst_global_alpha = opa,
        .src_global_alpha = opa,
        .dst_alpha_mode = PXP_PORTER_DUFF_ALPHA_STRAIGHT, /*don't care*/
        .src_alpha_mode = PXP_PORTER_DUFF_ALPHA_STRAIGHT  /*don't care*/
    };

    pxp_set_porter_duff_config(&pdConfig);

    lv_gpu_nxp_pxp_run();
}

void lv_gpu_nxp_pxp_blit(lv_color_t * dest_buf, const lv_area_t * dest_area, lv_coord_t dest_stride,
                         const lv_color_t * src_buf, const lv_area_t * src_area, lv_coord_t src_stride,
                         lv_opa_t opa, lv_disp_rot_t angle)
{
    lv_coord_t dest_w = lv_area_get_width(dest_area);
    lv_coord_t dest_h = lv_area_get_height(dest_area);
    lv_coord_t src_w = lv_area_get_width(src_area);
    lv_coord_t src_h = lv_area_get_height(src_area);

    lv_gpu_nxp_pxp_reset();

    /* convert rotation angle */
    enum pxp_rotate_degree_e pxp_rot;
    switch(angle) {
        case LV_DISP_ROT_NONE:
            pxp_rot = PXP_ROTATE0;
            break;
        case LV_DISP_ROT_90:
            pxp_rot = PXP_ROTATE90;
            break;
        case LV_DISP_ROT_180:
            pxp_rot = PXP_ROTATE180;
            break;
        case LV_DISP_ROT_270:
            pxp_rot = PXP_ROTATE270;
            break;
        default:
            pxp_rot = PXP_ROTATE0;
            break;
    }
    pxp_set_rotate_config(PXP_ROTATE_OUTPUT_BUFFER, pxp_rot, PXP_FLIP_DISABLE);

    struct pxp_as_blend_config_s asBlendConfig = {
        .alpha = opa,
        .invert_alpha = false,
        .alpha_mode = PXP_ALPHA_ROP,
        .rop_mode = PXP_ROP_MERGE_AS
    };

    if(opa >= (lv_opa_t)LV_OPA_MAX) {
        /*Simple blit, no effect - Disable PS buffer*/
        pxp_set_process_surface_position(0xFFFFU, 0xFFFFU, 0U, 0U);
    }
    else {
        struct pxp_ps_buffer_config_s psBufferConfig = {
            .pixel_format = PXP_PS_PIXEL_FORMAT,
            .swap_byte = false,
            .buffer_addr = (uint32_t)(dest_buf + dest_stride * dest_area->y1 + dest_area->x1),
            .buffer_addr_u = 0U,
            .buffer_addr_v = 0U,
            .pitch_bytes = dest_stride * sizeof(lv_color_t)
        };

        asBlendConfig.alpha_mode = PXP_ALPHA_OVERRIDE;

        pxp_set_process_surface_buffer_config(&psBufferConfig);
        pxp_set_process_surface_position(0U, 0U, dest_w - 1U, dest_h - 1U);
    }

    /*AS buffer - source image*/
    struct pxp_as_buffer_config_s asBufferConfig = {
        .pixel_format = PXP_AS_PIXEL_FORMAT,
        .buffer_addr = (uint32_t)(src_buf + src_stride * src_area->y1 + src_area->x1),
        .pitch_bytes = src_stride * sizeof(lv_color_t)
    };
    pxp_set_alpha_surface_buffer_config(&asBufferConfig);
    pxp_set_alpha_surface_position(0U, 0U, src_w - 1U, src_h - 1U);
    pxp_set_alpha_surface_blend_config(&asBlendConfig);
    pxp_enable_alpha_surface_overlay_color_key(false);

    /*Output buffer.*/
    struct pxp_output_buffer_config_s outputBufferConfig = {
        .pixel_format = (enum pxp_output_pixel_format_e)PXP_OUT_PIXEL_FORMAT,
        .interlaced_mode = PXP_OUTPUT_PROGRESSIVE,
        .buffer0_addr = (uint32_t)(dest_buf + dest_stride * dest_area->y1 + dest_area->x1),
        .buffer1_addr = (uint32_t)0U,
        .pitch_bytes = dest_stride * sizeof(lv_color_t),
        .width = dest_w,
        .height = dest_h
    };
    pxp_set_output_buffer_config(&outputBufferConfig);

    lv_gpu_nxp_pxp_run();
}

void lv_gpu_nxp_pxp_blit_transform(lv_color_t * dest_buf, lv_area_t * dest_area, lv_coord_t dest_stride,
                                   const lv_color_t * src_buf, const lv_area_t * src_area, lv_coord_t src_stride,
                                   const lv_draw_img_dsc_t * dsc, lv_img_cf_t cf)
{
    bool has_recolor = (dsc->recolor_opa != LV_OPA_TRANSP);
    bool has_rotation = (dsc->angle != 0);

    if(has_recolor || has_rotation) {
        if(dsc->opa >= (lv_opa_t)LV_OPA_MAX && !lv_img_cf_has_alpha(cf) && !lv_img_cf_is_chroma_keyed(cf)) {
            lv_pxp_blit_cover(dest_buf, dest_area, dest_stride, src_buf, src_area, src_stride, dsc, cf);
            return;
        }
        else {
            /*Recolor and/or rotation with alpha or opacity is done in two steps.*/
            lv_pxp_blit_opa(dest_buf, dest_area, dest_stride, src_buf, src_area, src_stride, dsc, cf);
            return;
        }
    }

    lv_pxp_blit_cf(dest_buf, dest_area, dest_stride, src_buf, src_area, src_stride, dsc, cf);
}

void lv_gpu_nxp_pxp_buffer_copy(lv_color_t * dest_buf, const lv_area_t * dest_area, lv_coord_t dest_stride,
                                const lv_color_t * src_buf, const lv_area_t * src_area, lv_coord_t src_stride)
{
    lv_coord_t src_width = lv_area_get_width(src_area);
    lv_coord_t src_height = lv_area_get_height(src_area);

    lv_gpu_nxp_pxp_reset();

    const struct pxp_pic_copy_config_s picCopyConfig = {
        .src_pic_base_addr = (uint32_t)src_buf,
        .src_pitch_bytes = src_stride * sizeof(lv_color_t),
        .src_offset_x = src_area->x1,
        .src_offset_y = src_area->y1,
        .dest_pic_base_addr = (uint32_t)dest_buf,
        .dest_pitch_bytes = dest_stride * sizeof(lv_color_t),
        .dest_offset_x = dest_area->x1,
        .dest_offset_y = dest_area->y1,
        .width = src_width,
        .height = src_height,
        .pixel_format = PXP_AS_PIXEL_FORMAT
    };

    pxp_start_picture_copy(&picCopyConfig);

    lv_gpu_nxp_pxp_wait();
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void lv_pxp_blit_opa(lv_color_t * dest_buf, const lv_area_t * dest_area, lv_coord_t dest_stride,
                            const lv_color_t * src_buf, const lv_area_t * src_area, lv_coord_t src_stride,
                            const lv_draw_img_dsc_t * dsc, lv_img_cf_t cf)
{
    lv_area_t temp_area;
    lv_area_copy(&temp_area, dest_area);
    lv_coord_t temp_stride = dest_stride;
    lv_coord_t temp_w = lv_area_get_width(&temp_area);
    lv_coord_t temp_h = lv_area_get_height(&temp_area);

    uint32_t size = temp_w * temp_h * sizeof(lv_color_t);

    lv_color_t * temp_buf = (lv_color_t *)lv_mem_buf_get(size);

    /*Step 1: Transform with full opacity to temporary buffer*/
    lv_pxp_blit_cover((lv_color_t *)temp_buf, &temp_area, temp_stride, src_buf, src_area, src_stride, dsc, cf);

    /*Switch width and height if angle requires it*/
    if(dsc->angle == 900 || dsc->angle == 2700) {
        temp_area.x2 = temp_area.x1 + temp_h - 1;
        temp_area.y2 = temp_area.y1 + temp_w - 1;
    }

    /*Step 2: Blit temporary result with required opacity to output*/
    lv_pxp_blit_cf(dest_buf, &temp_area, dest_stride, (lv_color_t *)temp_buf, &temp_area, temp_stride, dsc, cf);
}

static void lv_pxp_blit_cover(lv_color_t * dest_buf, lv_area_t * dest_area, lv_coord_t dest_stride,
                              const lv_color_t * src_buf, const lv_area_t * src_area, lv_coord_t src_stride,
                              const lv_draw_img_dsc_t * dsc, lv_img_cf_t cf)
{
    lv_coord_t dest_w = lv_area_get_width(dest_area);
    lv_coord_t dest_h = lv_area_get_height(dest_area);
    lv_coord_t src_w = lv_area_get_width(src_area);
    lv_coord_t src_h = lv_area_get_height(src_area);

    bool has_recolor = (dsc->recolor_opa != LV_OPA_TRANSP);
    bool has_rotation = (dsc->angle != 0);

    lv_point_t pivot = dsc->pivot;
    lv_coord_t piv_offset_x;
    lv_coord_t piv_offset_y;

    lv_gpu_nxp_pxp_reset();

    if(has_rotation) {
        /*Convert rotation angle and calculate offsets caused by pivot*/
        enum pxp_rotate_degree_e pxp_angle;
        switch(dsc->angle) {
            case 0:
                pxp_angle = PXP_ROTATE0;
                piv_offset_x = 0;
                piv_offset_y = 0;
                break;
            case 900:
                piv_offset_x = pivot.x + pivot.y - dest_h;
                piv_offset_y = pivot.y - pivot.x;
                pxp_angle = PXP_ROTATE90;
                break;
            case 1800:
                piv_offset_x = 2 * pivot.x - dest_w;
                piv_offset_y = 2 * pivot.y - dest_h;
                pxp_angle = PXP_ROTATE180;
                break;
            case 2700:
                piv_offset_x = pivot.x - pivot.y;
                piv_offset_y = pivot.x + pivot.y - dest_w;
                pxp_angle = PXP_ROTATE270;
                break;
            default:
                piv_offset_x = 0;
                piv_offset_y = 0;
                pxp_angle = PXP_ROTATE0;
        }
        pxp_set_rotate_config(PXP_ROTATE_OUTPUT_BUFFER, pxp_angle, PXP_FLIP_DISABLE);
        lv_area_move(dest_area, piv_offset_x, piv_offset_y);
    }

    /*AS buffer - source image*/
    struct pxp_as_buffer_config_s asBufferConfig = {
        .pixel_format = PXP_AS_PIXEL_FORMAT,
        .buffer_addr = (uint32_t)(src_buf + src_stride * src_area->y1 + src_area->x1),
        .pitch_bytes = src_stride * sizeof(lv_color_t)
    };
    pxp_set_alpha_surface_buffer_config(&asBufferConfig);
    pxp_set_alpha_surface_position(0U, 0U, src_w - 1U, src_h - 1U);

    /*Disable PS buffer*/
    pxp_set_process_surface_position(0xFFFFU, 0xFFFFU, 0U, 0U);
    if(has_recolor)
        /*Use as color generator*/
        pxp_set_process_surface_back_ground_color(lv_color_to32(dsc->recolor));

    /*Output buffer*/
    struct pxp_output_buffer_config_s outputBufferConfig = {
        .pixel_format = (enum pxp_output_pixel_format_e)PXP_OUT_PIXEL_FORMAT,
        .interlaced_mode = PXP_OUTPUT_PROGRESSIVE,
        .buffer0_addr = (uint32_t)(dest_buf + dest_stride * dest_area->y1 + dest_area->x1),
        .buffer1_addr = (uint32_t)0U,
        .pitch_bytes = dest_stride * sizeof(lv_color_t),
        .width = dest_w,
        .height = dest_h
    };
    pxp_set_output_buffer_config(&outputBufferConfig);

    if(has_recolor || lv_img_cf_has_alpha(cf)) {
        /**
         * Configure Porter-Duff blending.
         *
         * Note: srcFactorMode and dstFactorMode are inverted in fsl_pxp.h:
         * srcFactorMode is actually applied on PS alpha value
         * dstFactorMode is actually applied on AS alpha value
         */
        struct pxp_porter_duff_config_s pdConfig = {
            .enable = 1,
            .dst_color_mode = PXP_PORTER_DUFF_COLOR_WITH_ALPHA,
            .src_color_mode = PXP_PORTER_DUFF_COLOR_NO_ALPHA,
            .dst_global_alpha_mode = PXP_PORTER_DUFF_GLOBAL_ALPHA,
            .src_global_alpha_mode = lv_img_cf_has_alpha(cf) ? PXP_PORTER_DUFF_LOCAL_ALPHA : PXP_PORTER_DUFF_GLOBAL_ALPHA,
            .dst_factor_mode = PXP_PORTER_DUFF_FACTOR_STRAIGHT,
            .src_factor_mode = PXP_PORTER_DUFF_FACTOR_INVERSED,
            .dst_global_alpha = has_recolor ? dsc->recolor_opa : 0x00,
            .src_global_alpha = 0xff,
            .dst_alpha_mode = PXP_PORTER_DUFF_ALPHA_STRAIGHT, /*don't care*/
            .src_alpha_mode = PXP_PORTER_DUFF_ALPHA_STRAIGHT
        };
        pxp_set_porter_duff_config(&pdConfig);
    }

    lv_gpu_nxp_pxp_run();
}

static void lv_pxp_blit_cf(lv_color_t * dest_buf, const lv_area_t * dest_area, lv_coord_t dest_stride,
                           const lv_color_t * src_buf, const lv_area_t * src_area, lv_coord_t src_stride,
                           const lv_draw_img_dsc_t * dsc, lv_img_cf_t cf)
{
    lv_coord_t dest_w = lv_area_get_width(dest_area);
    lv_coord_t dest_h = lv_area_get_height(dest_area);
    lv_coord_t src_w = lv_area_get_width(src_area);
    lv_coord_t src_h = lv_area_get_height(src_area);

    lv_gpu_nxp_pxp_reset();

    struct pxp_as_blend_config_s asBlendConfig = {
        .alpha = dsc->opa,
        .invert_alpha = false,
        .alpha_mode = PXP_ALPHA_ROP,
        .rop_mode = PXP_ROP_MERGE_AS
    };

    if(dsc->opa >= (lv_opa_t)LV_OPA_MAX && !lv_img_cf_is_chroma_keyed(cf) && !lv_img_cf_has_alpha(cf)) {
        /*Simple blit, no effect - Disable PS buffer*/
        pxp_set_process_surface_position(0xFFFFU, 0xFFFFU, 0U, 0U);
    }
    else {
        /*PS must be enabled to fetch background pixels.
          PS and OUT buffers are the same, blend will be done in-place*/
        struct pxp_ps_buffer_config_s psBufferConfig = {
            .pixel_format = PXP_PS_PIXEL_FORMAT,
            .swap_byte = false,
            .buffer_addr = (uint32_t)(dest_buf + dest_stride * dest_area->y1 + dest_area->x1),
            .buffer_addr_u = 0U,
            .buffer_addr_v = 0U,
            .pitch_bytes = dest_stride * sizeof(lv_color_t)
        };
        if(dsc->opa >= (lv_opa_t)LV_OPA_MAX) {
            asBlendConfig.alpha_mode = lv_img_cf_has_alpha(cf) ? PXP_ALPHA_EMBEDDED : PXP_ALPHA_OVERRIDE;
        }
        else {
            asBlendConfig.alpha_mode = lv_img_cf_has_alpha(cf) ? PXP_ALPHA_MULTIPLY : PXP_ALPHA_OVERRIDE;
        }
        pxp_set_process_surface_buffer_config(&psBufferConfig);
        pxp_set_process_surface_position(0U, 0U, dest_w - 1U, dest_h - 1U);
    }

    /*AS buffer - source image*/
    struct pxp_as_buffer_config_s asBufferConfig = {
        .pixel_format = PXP_AS_PIXEL_FORMAT,
        .buffer_addr = (uint32_t)(src_buf + src_stride * src_area->y1 + src_area->x1),
        .pitch_bytes = src_stride * sizeof(lv_color_t)
    };
    pxp_set_alpha_surface_buffer_config(&asBufferConfig);
    pxp_set_alpha_surface_position(0U, 0U, src_w - 1U, src_h - 1U);
    pxp_set_alpha_surface_blend_config(&asBlendConfig);

    if(lv_img_cf_is_chroma_keyed(cf)) {
        lv_color_t colorKeyLow = LV_COLOR_CHROMA_KEY;
        lv_color_t colorKeyHigh = LV_COLOR_CHROMA_KEY;

        bool has_recolor = (dsc->recolor_opa != LV_OPA_TRANSP);

        if(has_recolor) {
            /* New color key after recoloring */
            lv_color_t colorKey =  lv_color_mix(dsc->recolor, LV_COLOR_CHROMA_KEY, dsc->recolor_opa);

            LV_COLOR_SET_R(colorKeyLow, colorKey.ch.red != 0 ? colorKey.ch.red - 1 : 0);
            LV_COLOR_SET_G(colorKeyLow, colorKey.ch.green != 0 ? colorKey.ch.green - 1 : 0);
            LV_COLOR_SET_B(colorKeyLow, colorKey.ch.blue != 0 ? colorKey.ch.blue - 1 : 0);

#if LV_COLOR_DEPTH == 16
            LV_COLOR_SET_R(colorKeyHigh, colorKey.ch.red != 0x1f ? colorKey.ch.red + 1 : 0x1f);
            LV_COLOR_SET_G(colorKeyHigh, colorKey.ch.green != 0x3f ? colorKey.ch.green + 1 : 0x3f);
            LV_COLOR_SET_B(colorKeyHigh, colorKey.ch.blue != 0x1f ? colorKey.ch.blue + 1 : 0x1f);
#else /*LV_COLOR_DEPTH == 32*/
            LV_COLOR_SET_R(colorKeyHigh, colorKey.ch.red != 0xff ? colorKey.ch.red + 1 : 0xff);
            LV_COLOR_SET_G(colorKeyHigh, colorKey.ch.green != 0xff ? colorKey.ch.green + 1 : 0xff);
            LV_COLOR_SET_B(colorKeyHigh, colorKey.ch.blue != 0xff ? colorKey.ch.blue + 1 : 0xff);
#endif
        }

        pxp_set_alpha_surface_overlay_color_key(lv_color_to32(colorKeyLow),
                                           lv_color_to32(colorKeyHigh));
    }

    pxp_enable_alpha_surface_overlay_color_key(lv_img_cf_is_chroma_keyed(cf));

    /*Output buffer.*/
    struct pxp_output_buffer_config_s outputBufferConfig = {
        .pixel_format = (enum pxp_output_pixel_format_e)PXP_OUT_PIXEL_FORMAT,
        .interlaced_mode = PXP_OUTPUT_PROGRESSIVE,
        .buffer0_addr = (uint32_t)(dest_buf + dest_stride * dest_area->y1 + dest_area->x1),
        .buffer1_addr = (uint32_t)0U,
        .pitch_bytes = dest_stride * sizeof(lv_color_t),
        .width = dest_w,
        .height = dest_h
    };
    pxp_set_output_buffer_config(&outputBufferConfig);

    lv_gpu_nxp_pxp_run();
}

#endif /*LV_USE_GPU_NXP_PXP*/
