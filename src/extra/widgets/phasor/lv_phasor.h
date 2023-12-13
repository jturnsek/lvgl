/**
 * @file lv_phasor.h
 *
 */

#ifndef LV_PHASOR_H
#define LV_PHASOR_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "../../../lvgl.h"

#if LV_USE_PHASOR != 0

/*Testing of dependencies*/
#if LV_DRAW_COMPLEX == 0
#error "lv_phasor: Complex drawing is required. Enable it in lv_conf.h (LV_DRAW_COMPLEX 1)"
#endif

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

typedef struct {
    lv_color_t tick_color;
    uint16_t tick_cnt;
    uint16_t tick_length;
    uint16_t tick_width;

    lv_color_t tick_major_color;
    uint16_t tick_major_nth;
    uint16_t tick_major_length;
    uint16_t tick_major_width;

    int16_t label_gap;

    int32_t min;
    int32_t max;
    int16_t r_mod;
} lv_phasor_scale_t;

enum {
    LV_PHASOR_INDICATOR_TYPE_PHASOR_LINE,
    LV_PHASOR_INDICATOR_TYPE_SCALE_LINES,
    LV_PHASOR_INDICATOR_TYPE_ARC,
};
typedef uint8_t lv_phasor_indicator_type_t;

typedef struct {
    lv_phasor_scale_t * scale;
    lv_phasor_indicator_type_t type;
    lv_opa_t opa;
    int32_t start_value;
    int32_t end_value;
    union {
        struct {
            uint16_t width;
            int16_t r_mod;
            lv_color_t color;
        } phasor_line;
        struct {
            uint16_t width;
            const void * src;
            lv_color_t color;
            int16_t r_mod;
        } arc;
        struct {
            int16_t width_mod;
            lv_color_t color_start;
            lv_color_t color_end;
            uint8_t local_grad : 1;
        } scale_lines;
    } type_data;
} lv_phasor_indicator_t;

/*Data of line phasor*/
typedef struct {
    lv_obj_t obj;
    lv_ll_t scale_ll;
    lv_ll_t indicator_ll;
} lv_phasor_t;

extern const lv_obj_class_t lv_phasor_class;

/**
 * `type` field in `lv_obj_draw_part_dsc_t` if `class_p = lv_phasor_class`
 * Used in `LV_EVENT_DRAW_PART_BEGIN` and `LV_EVENT_DRAW_PART_END`
 */
typedef enum {
    LV_PHASOR_DRAW_PART_ARC,             /**< The arc indicator*/
    LV_PHASOR_DRAW_PART_PHASOR_LINE,     /**< The phasor lines*/
    LV_PHASOR_DRAW_PART_TICK,            /**< The tick lines and labels*/
} lv_phasor_draw_part_type_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Create a Phasor object
 * @param parent pointer to an object, it will be the parent of the new bar.
 * @return pointer to the created phasor
 */
lv_obj_t * lv_phasor_create(lv_obj_t * parent);

/*=====================
 * Add scale
 *====================*/

/**
 * Add a new scale to the phasor.
 * @param obj   pointer to a phasor object
 * @return      the new scale
 * @note        Indicators can be attached to scales.
 */
lv_phasor_scale_t * lv_phasor_add_scale(lv_obj_t * obj);

/**
 * Set the properties of the ticks of a scale
 * @param obj       pointer to a phasor object
 * @param scale     pointer to scale (added to `phasor`)
 * @param cnt       number of tick lines
 * @param width     width of tick lines
 * @param len       length of tick lines
 * @param color     color of tick lines
 */
void lv_phasor_set_scale_ticks(lv_obj_t * obj, lv_phasor_scale_t * scale, uint16_t cnt, uint16_t width, uint16_t len,
                              lv_color_t color);

/**
 * Make some "normal" ticks major ticks and set their attributes.
 * Texts with the current value are also added to the major ticks.
 * @param obj           pointer to a phasor object
 * @param scale         pointer to scale (added to `phasor`)
 * @param nth           make every Nth normal tick major tick. (start from the first on the left)
 * @param width         width of the major ticks
 * @param len           length of the major ticks
 * @param color         color of the major ticks
 * @param label_gap     gap between the major ticks and the labels
 */
void lv_phasor_set_scale_major_ticks(lv_obj_t * obj, lv_phasor_scale_t * scale, uint16_t nth, uint16_t width,
                                    uint16_t len, lv_color_t color, int16_t label_gap);

/*=====================
 * Add indicator
 *====================*/

/**
 * Add a phasor line indicator the scale
 * @param obj           pointer to a phasor object
 * @param scale         pointer to scale (added to `phasor`)
 * @param width         width of the line
 * @param color         color of the line
 * @param r_mod         the radius modifier (added to the scale's radius) to get the lines length
 * @return              the new indicator
 */
lv_phasor_indicator_t * lv_phasor_add_phasor_line(lv_obj_t * obj, lv_phasor_scale_t * scale, lv_phasor_indicator_type_t type, uint16_t width,
                                                lv_color_t color, int16_t r_mod);

/**
 * Add an arc indicator the scale
 * @param obj           pointer to a phasor object
 * @param scale         pointer to scale (added to `phasor`)
 * @param width         width of the arc
 * @param color         color of the arc
 * @param r_mod         the radius modifier (added to the scale's radius) to get the outer radius of the arc
 * @return              the new indicator
 */
lv_phasor_indicator_t * lv_phasor_add_arc(lv_obj_t * obj, lv_phasor_scale_t * scale, uint16_t width, lv_color_t color,
                                        int16_t r_mod);


/**
 * Add a scale line indicator the scale. It will modify the ticks.
 * @param obj           pointer to a phasor object
 * @param scale         pointer to scale (added to `phasor`)
 * @param color_start   the start color
 * @param color_end     the end color
 * @param local         tell how to map start and end color. true: the indicator's start and end_value; false: the scale's min max value
 * @param width_mod     add this the affected tick's width
 * @return              the new indicator
 */
lv_phasor_indicator_t * lv_phasor_add_scale_lines(lv_obj_t * obj, lv_phasor_scale_t * scale, lv_color_t color_start,
                                                lv_color_t color_end, bool local, int16_t width_mod);

/*=====================
 * Set indicator value
 *====================*/

/**
 * Set the value of the indicator. It will set start and and value to the same value
 * @param obj           pointer to a phasor object
 * @param indic         pointer to an indicator
 * @param value         the new value
 */
void lv_phasor_set_indicator_value(lv_obj_t * obj, lv_phasor_indicator_t * indic, int32_t value);

/**
 * Set the start value of the indicator.
 * @param obj           pointer to a phasor object
 * @param indic         pointer to an indicator
 * @param value         the new value
 */
void lv_phasor_set_indicator_start_value(lv_obj_t * obj, lv_phasor_indicator_t * indic, int32_t value);

/**
 * Set the start value of the indicator.
 * @param obj           pointer to a phasor object
 * @param indic         pointer to an indicator
 * @param value         the new value
 */
void lv_phasor_set_indicator_end_value(lv_obj_t * obj, lv_phasor_indicator_t * indic, int32_t value);

/**
 * Set the value of the indicator. It will set start and and value to the same value
 * @param obj           pointer to a phasor object
 * @param indic         pointer to an indicator
 * @param r_mod         the new value
 */
void lv_phasor_set_phasor_r_mod(lv_obj_t * obj, lv_phasor_indicator_t * indic, int16_t r_mod);


/**********************
 *      MACROS
 **********************/

#endif /*LV_USE_PHASOR*/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_PHASOR_H*/
