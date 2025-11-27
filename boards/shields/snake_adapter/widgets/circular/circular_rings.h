/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <lvgl.h>
#include <zephyr/kernel.h>
#include "circular_layout.h"

// Ring style definitions
struct ring_style {
    lv_color_t color;          // Ring color
    uint8_t thickness;         // Ring thickness in pixels
    uint8_t tick_count;        // Number of tick marks (0 = no ticks)
    uint8_t tick_length;       // Tick mark length in pixels
    lv_color_t tick_color;     // Tick mark color
    bool filled;               // Whether ring should be filled
    lv_color_t fill_color;     // Fill color for filled rings
};

// Tick mark style
struct tick_style {
    uint8_t major_every;       // Frequency of major ticks
    uint8_t major_length;      // Major tick length
    uint8_t minor_length;      // Minor tick length
    lv_color_t major_color;    // Major tick color
    lv_color_t minor_color;    // Minor tick color
};

// Ring rendering context
struct ring_context {
    lv_obj_t *canvas;          // LVGL canvas object
    lv_draw_buf_t *draw_buf;  // Drawing buffer
    uint16_t width;           // Canvas width
    uint16_t height;          // Canvas height
    struct circular_position center;  // Ring center position
};

// Predefined ring styles
extern const struct ring_style RING_STYLE_THIN;
extern const struct ring_style RING_STYLE_MEDIUM;
extern const struct ring_style RING_STYLE_THICK;
extern const struct ring_style RING_STYLE_DECORATIVE;
extern const struct tick_style TICK_STYLE_CLOCK;
extern const struct tick_style TICK_STYLE_SPEEDOMETER;

// === Core Ring Drawing Functions ===

/**
 * Initialize ring rendering system
 * @param canvas LVGL canvas for drawing
 * @param center Center position for rings
 * @return Ring context for subsequent operations
 */
struct ring_context* ring_init(lv_obj_t *canvas, const struct circular_position *center);

/**
 * Draw a complete ring (full circle)
 * @param ctx Ring context
 * @param inner_radius Inner radius of ring
 * @param outer_radius Outer radius of ring
 * @param style Ring styling options
 * @return 0 on success, error code on failure
 */
int ring_draw_complete(const struct ring_context *ctx,
                       uint16_t inner_radius,
                       uint16_t outer_radius,
                       const struct ring_style *style);

/**
 * Draw a ring arc (partial circle)
 * @param ctx Ring context
 * @param arc Arc definition (angles and radii)
 * @param style Ring styling options
 * @return 0 on success, error code on failure
 */
int ring_draw_arc(const struct ring_context *ctx,
                  const struct circular_arc *arc,
                  const struct ring_style *style);

// === Tick Mark Functions ===

/**
 * Draw tick marks around a ring
 * @param ctx Ring context
 * @param radius Radius for tick marks
 * @param tick_style Tick mark styling
 * @return 0 on success, error code on failure
 */
int ring_draw_ticks(const struct ring_context *ctx,
                    uint16_t radius,
                    const struct tick_style *tick_style);

/**
 * Draw single tick mark
 * @param ctx Ring context
 * @param angle_deg Angle for tick mark
 * @param inner_radius Inner endpoint of tick
 * @param outer_radius Outer endpoint of tick
 * @param color Tick color
 * @return 0 on success, error code on failure
 */
int ring_draw_tick(const struct ring_context *ctx,
                   uint16_t angle_deg,
                   uint16_t inner_radius,
                   uint16_t outer_radius,
                   lv_color_t color);

// === Decorative Functions ===

/**
 * Draw gradient-filled ring
 * @param ctx Ring context
 * @param inner_radius Inner radius
 * @param outer_radius Outer radius
 * @param start_color Gradient start color
 * @param end_color Gradient end color
 * @param start_angle Optional gradient start angle (use -1 for full circle)
 * @return 0 on success, error code on failure
 */
int ring_draw_gradient(const struct ring_context *ctx,
                       uint16_t inner_radius,
                       uint16_t outer_radius,
                       lv_color_t start_color,
                       lv_color_t end_color,
                       int16_t start_angle);

/**
 * Draw animated ring segment
 * @param ctx Ring context
 * @param arc Arc to animate
 * @param target_style Target ring style
 * @param progress Animation progress (0-100)
 * @return 0 on success, error code on failure
 */
int ring_draw_animated_segment(const struct ring_context *ctx,
                                const struct circular_arc *arc,
                                const struct ring_style *target_style,
                                uint8_t progress);

/**
 * Draw ring with pattern fill
 * @param ctx Ring context
 * @param inner_radius Inner radius
 * @param outer_radius Outer radius
 * @param pattern Pattern type (stripes, dots, etc.)
 * @param primary_color Primary pattern color
 * @param secondary_color Secondary pattern color
 * @return 0 on success, error code on failure
 */
int ring_draw_pattern(const struct ring_context *ctx,
                      uint16_t inner_radius,
                      uint16_t outer_radius,
                      uint8_t pattern,
                      lv_color_t primary_color,
                      lv_color_t secondary_color);

// === Utility Functions ===

/**
 * Clear canvas in ring context
 * @param ctx Ring context
 * @param bg_color Background color
 * @return 0 on success, error code on failure
 */
int ring_clear_canvas(const struct ring_context *ctx, lv_color_t bg_color);

/**
 * Update ring context center position
 * @param ctx Ring context
 * @param new_center New center position
 */
void ring_update_center(struct ring_context *ctx, const struct circular_position *new_center);

/**
 * Get ring canvas for LVGL operations
 * @param ctx Ring context
 * @return LVGL canvas object
 */
lv_obj_t* ring_get_canvas(const struct ring_context *ctx);

/**
 * Refresh ring display
 * @param ctx Ring context
 * @return 0 on success, error code on failure
 */
int ring_refresh_display(const struct ring_context *ctx);

// === Predefined Ring Layouts ===

/**
 * Draw decorative outer ring
 * @param ctx Ring context
 * @param theme_color Color to use for ring
 * @return 0 on success, error code on failure
 */
int ring_draw_decorative_outer(const struct ring_context *ctx, lv_color_t theme_color);

/**
 * Draw speed gauge ring
 * @param ctx Ring context
 * @param current_value Current speed value
 * @param max_value Maximum speed value
 * @return 0 on success, error code on failure
 */
int ring_draw_speed_gauge(const struct ring_context *ctx,
                           uint16_t current_value,
                           uint16_t max_value);

/**
 * Draw battery level ring
 * @param ctx Ring context
 * @param quadrant Which quadrant to draw in
 * @param battery_level Battery level percentage (0-100)
 * @return 0 on success, error code on failure
 */
int ring_draw_battery_indicator(const struct ring_context *ctx,
                                 enum circular_quadrant quadrant,
                                 uint8_t battery_level);

/**
 * Draw BLE status ring
 * @param ctx Ring context
 * @param device_index Device index (0-3)
 * @param connection_status Connection status
 * @return 0 on success, error code on failure
 */
int ring_draw_ble_status(const struct ring_context *ctx,
                         uint8_t device_index,
                         uint8_t connection_status);

// === Cleanup Functions ===

/**
 * Cleanup ring context and free resources
 * @param ctx Ring context to cleanup
 */
void ring_cleanup(struct ring_context *ctx);