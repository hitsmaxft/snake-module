/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include "circular_rings.h"
#include "../helpers/display.h"
#include <zephyr/logging/log.h>
#include <math.h>

LOG_MODULE_DECLARE(circular_rings, CONFIG_ZMK_LOG_LEVEL);

// Predefined ring styles
const struct ring_style RING_STYLE_THIN = {
    .color = lv_color_hex(0x00FF00),
    .thickness = 2,
    .tick_count = 0,
    .tick_length = 0,
    .tick_color = lv_color_hex(0xFFFFFF),
    .filled = false,
    .fill_color = lv_color_hex(0x000000)
};

const struct ring_style RING_STYLE_MEDIUM = {
    .color = lv_color_hex(0x00AA00),
    .thickness = 4,
    .tick_count = 12,
    .tick_length = 8,
    .tick_color = lv_color_hex(0xFFFFFF),
    .filled = false,
    .fill_color = lv_color_hex(0x000000)
};

const struct ring_style RING_STYLE_THICK = {
    .color = lv_color_hex(0x005500),
    .thickness = 8,
    .tick_count = 0,
    .tick_length = 0,
    .tick_color = lv_color_hex(0xFFFFFF),
    .filled = false,
    .fill_color = lv_color_hex(0x000000)
};

const struct ring_style RING_STYLE_DECORATIVE = {
    .color = lv_color_hex(0x444444),
    .thickness = 1,
    .tick_count = 24,
    .tick_length = 4,
    .tick_color = lv_color_hex(0x666666),
    .filled = false,
    .fill_color = lv_color_hex(0x000000)
};

const struct tick_style TICK_STYLE_CLOCK = {
    .major_every = 3,
    .major_length = 10,
    .minor_length = 5,
    .major_color = lv_color_hex(0xFFFFFF),
    .minor_color = lv_color_hex(0x888888)
};

const struct tick_style TICK_STYLE_SPEEDOMETER = {
    .major_every = 5,
    .major_length = 8,
    .minor_length = 4,
    .major_color = lv_color_hex(0xFF0000),
    .minor_color = lv_color_hex(0x888888)
};

// Static ring context for efficiency
static struct ring_context g_ring_ctx;

// Internal helper functions
static int draw_ring_segment(const struct ring_context *ctx,
                             uint16_t center_x, uint16_t center_y,
                             uint16_t radius, uint16_t thickness,
                             uint16_t start_angle, uint16_t end_angle,
                             lv_color_t color);

static lv_point_t get_point_on_circle(uint16_t center_x, uint16_t center_y,
                                     uint16_t radius, uint16_t angle_deg);

struct ring_context* ring_init(lv_obj_t *canvas, const struct circular_position *center) {
    if (canvas == NULL || center == NULL) {
        LOG_ERR("Invalid parameters for ring_init");
        return NULL;
    }

    g_ring_ctx.canvas = canvas;
    g_ring_ctx.draw_buf = lv_canvas_get_buf(canvas);
    g_ring_ctx.width = lv_obj_get_width(canvas);
    g_ring_ctx.height = lv_obj_get_height(canvas);
    g_ring_ctx.center = *center;

    LOG_DBG("Ring context initialized: %dx%d canvas, center at (%d, %d) r=%d",
            g_ring_ctx.width, g_ring_ctx.height,
            center->angle_deg, center->radius);

    return &g_ring_ctx;
}

int ring_draw_complete(const struct ring_context *ctx,
                       uint16_t inner_radius,
                       uint16_t outer_radius,
                       const struct ring_style *style) {
    if (ctx == NULL || style == NULL) {
        LOG_ERR("NULL parameters");
        return -1;
    }

    struct cartesian_position center_pos = polar_to_cartesian(&ctx->center);

    // Draw filled ring if requested
    if (style->filled) {
        for (uint16_t r = inner_radius; r <= outer_radius; r++) {
            lv_draw_arc(ctx->draw_buf, &center_pos, r, 0, 360, style->fill_color);
        }
    } else {
        // Draw ring segments for thickness
        for (uint16_t offset = 0; offset < style->thickness; offset++) {
            uint16_t radius = inner_radius + offset;
            if (radius <= outer_radius) {
                lv_draw_arc(ctx->draw_buf, &center_pos, radius, 0, 360, style->color);
            }
        }
    }

    // Draw tick marks if specified
    if (style->tick_count > 0) {
        struct tick_style tick_style = {
            .major_every = 1,
            .major_length = style->tick_length,
            .minor_length = style->tick_length / 2,
            .major_color = style->tick_color,
            .minor_color = style->tick_color
        };

        ring_draw_ticks(ctx, inner_radius + style->thickness/2, &tick_style);
    }

    return 0;
}

int ring_draw_arc(const struct ring_context *ctx,
                  const struct circular_arc *arc,
                  const struct ring_style *style) {
    if (ctx == NULL || arc == NULL || style == NULL) {
        LOG_ERR("NULL parameters");
        return -1;
    }

    struct cartesian_position center_pos = polar_to_cartesian(&ctx->center);

    // Calculate arc span
    uint16_t arc_span = get_arc_span(arc);

    // Draw filled arc if requested
    if (style->filled) {
        for (uint16_t r = arc->inner_radius; r <= arc->outer_radius; r++) {
            lv_draw_arc(ctx->draw_buf, &center_pos, r,
                       arc->start_angle_deg, arc->end_angle_deg, style->fill_color);
        }
    } else {
        // Draw arc segments for thickness
        for (uint16_t offset = 0; offset < style->thickness; offset++) {
            uint16_t radius = arc->inner_radius + offset;
            if (radius <= arc->outer_radius) {
                lv_draw_arc(ctx->draw_buf, &center_pos, radius,
                           arc->start_angle_deg, arc->end_angle_deg, style->color);
            }
        }
    }

    return 0;
}

int ring_draw_ticks(const struct ring_context *ctx,
                    uint16_t radius,
                    const struct tick_style *tick_style) {
    if (ctx == NULL || tick_style == NULL) {
        LOG_ERR("NULL parameters");
        return -1;
    }

    struct cartesian_position center_pos = polar_to_cartesian(&ctx->center);

    for (uint16_t i = 0; i < 360; i += (360 / 12)) {  // 12 hour marks
        bool is_major = (i % (tick_style->major_every * (360 / 12))) == 0;
        uint16_t tick_length = is_major ? tick_style->major_length : tick_style->minor_length;
        lv_color_t tick_color = is_major ? tick_style->major_color : tick_style->minor_color;

        uint16_t inner_radius = radius - tick_length;
        uint16_t outer_radius = radius;

        lv_point_t start = get_point_on_circle(center_pos.x, center_pos.y,
                                               inner_radius, i);
        lv_point_t end = get_point_on_circle(center_pos.x, center_pos.y,
                                             outer_radius, i);

        lv_draw_line(ctx->draw_buf, &start, &end, tick_color);
    }

    return 0;
}

int ring_draw_tick(const struct ring_context *ctx,
                   uint16_t angle_deg,
                   uint16_t inner_radius,
                   uint16_t outer_radius,
                   lv_color_t color) {
    if (ctx == NULL) {
        LOG_ERR("NULL context");
        return -1;
    }

    struct cartesian_position center_pos = polar_to_cartesian(&ctx->center);

    lv_point_t start = get_point_on_circle(center_pos.x, center_pos.y,
                                           inner_radius, angle_deg);
    lv_point_t end = get_point_on_circle(center_pos.x, center_pos.y,
                                         outer_radius, angle_deg);

    lv_draw_line(ctx->draw_buf, &start, &end, color);

    return 0;
}

int ring_draw_gradient(const struct ring_context *ctx,
                       uint16_t inner_radius,
                       uint16_t outer_radius,
                       lv_color_t start_color,
                       lv_color_t end_color,
                       int16_t start_angle) {
    if (ctx == NULL) {
        LOG_ERR("NULL context");
        return -1;
    }

    struct cartesian_position center_pos = polar_to_cartesian(&ctx->center);

    uint16_t gradient_steps = 16;
    for (uint16_t step = 0; step < gradient_steps; step++) {
        uint8_t progress = (step * 255) / gradient_steps;

        lv_color_t current_color = lv_color_mix(end_color, start_color, progress);

        uint16_t current_radius = inner_radius +
            ((outer_radius - inner_radius) * step) / gradient_steps;

        if (start_angle == -1) {
            // Full circle gradient
            lv_draw_arc(ctx->draw_buf, &center_pos, current_radius, 0, 360, current_color);
        } else {
            // Arc gradient
            lv_draw_arc(ctx->draw_buf, &center_pos, current_radius,
                       start_angle, start_angle + 90, current_color);
        }
    }

    return 0;
}

int ring_draw_animated_segment(const struct ring_context *ctx,
                                const struct circular_arc *arc,
                                const struct ring_style *target_style,
                                uint8_t progress) {
    if (ctx == NULL || arc == NULL || target_style == NULL) {
        LOG_ERR("NULL parameters");
        return -1;
    }

    if (progress > 100) progress = 100;
    if (progress == 0) return 0;  // Nothing to draw

    uint16_t arc_span = get_arc_span(arc);
    uint16_t current_span = (arc_span * progress) / 100;

    struct circular_arc animated_arc = *arc;
    animated_arc.end_angle_deg = arc->start_angle_deg + current_span;

    return ring_draw_arc(ctx, &animated_arc, target_style);
}

int ring_draw_pattern(const struct ring_context *ctx,
                      uint16_t inner_radius,
                      uint16_t outer_radius,
                      uint8_t pattern,
                      lv_color_t primary_color,
                      lv_color_t secondary_color) {
    if (ctx == NULL) {
        LOG_ERR("NULL context");
        return -1;
    }

    struct cartesian_position center_pos = polar_to_cartesian(&ctx->center);

    switch (pattern) {
        case 0: // Stripes
            for (uint16_t angle = 0; angle < 360; angle += 15) {
                lv_color_t color = (angle % 30 == 0) ? primary_color : secondary_color;
                lv_draw_arc(ctx->draw_buf, &center_pos, inner_radius,
                           angle, angle + 10, color);
            }
            break;

        case 1: // Dots
            for (uint16_t angle = 0; angle < 360; angle += 30) {
                lv_color_t color = (angle % 60 == 0) ? primary_color : secondary_color;
                uint16_t radius = (inner_radius + outer_radius) / 2;
                lv_point_t dot_pos = get_point_on_circle(center_pos.x, center_pos.y,
                                                         radius, angle);
                lv_draw_rect(ctx->draw_buf, dot_pos.x - 2, dot_pos.y - 2,
                             dot_pos.x + 2, dot_pos.y + 2, color);
            }
            break;

        default:
            LOG_ERR("Unknown pattern: %d", pattern);
            return -1;
    }

    return 0;
}

int ring_clear_canvas(const struct ring_context *ctx, lv_color_t bg_color) {
    if (ctx == NULL) {
        LOG_ERR("NULL context");
        return -1;
    }

    lv_canvas_fill_bg(ctx->canvas, bg_color, LV_OPA_COVER);
    return 0;
}

void ring_update_center(struct ring_context *ctx, const struct circular_position *new_center) {
    if (ctx != NULL && new_center != NULL) {
        ctx->center = *new_center;
    }
}

lv_obj_t* ring_get_canvas(const struct ring_context *ctx) {
    return (ctx != NULL) ? ctx->canvas : NULL;
}

int ring_refresh_display(const struct ring_context *ctx) {
    if (ctx == NULL) {
        LOG_ERR("NULL context");
        return -1;
    }

    lv_obj_invalidate(ctx->canvas);
    return 0;
}

int ring_draw_decorative_outer(const struct ring_context *ctx, lv_color_t theme_color) {
    if (ctx == NULL) {
        LOG_ERR("NULL context");
        return -1;
    }

    struct ring_style decorative = RING_STYLE_DECORATIVE;
    decorative.color = theme_color;

    // Draw outer decorative ring
    return ring_draw_complete(ctx, 100, 110, &decorative);
}

int ring_draw_speed_gauge(const struct ring_context *ctx,
                           uint16_t current_value,
                           uint16_t max_value) {
    if (ctx == NULL || max_value == 0) {
        LOG_ERR("Invalid parameters");
        return -1;
    }

    // Draw gauge background
    struct ring_style gauge_bg = RING_STYLE_MEDIUM;
    gauge_bg.color = lv_color_hex(0x333333);
    ring_draw_complete(ctx, 60, 75, &gauge_bg);

    // Draw speed progress
    if (current_value > 0) {
        struct ring_style gauge_progress = RING_STYLE_MEDIUM;
        gauge_progress.color = lv_color_hex(0x00FF00);

        uint16_t progress_angle = (current_value * 270) / max_value;  // 270-degree gauge
        struct circular_arc speed_arc = {
            .start_angle_deg = 135,  // Start at bottom-left
            .end_angle_deg = 135 + progress_angle,
            .inner_radius = 60,
            .outer_radius = 75
        };

        ring_draw_arc(ctx, &speed_arc, &gauge_progress);
    }

    return 0;
}

int ring_draw_battery_indicator(const struct ring_context *ctx,
                                 enum circular_quadrant quadrant,
                                 uint8_t battery_level) {
    if (ctx == NULL || battery_level > 100) {
        LOG_ERR("Invalid parameters");
        return -1;
    }

    struct cartesian_position battery_center = get_battery_position(quadrant, 85, 0);
    struct ring_context local_ctx = *ctx;
    local_ctx.center.radius = 85;

    // Draw battery background arc
    struct ring_style battery_bg = {
        .color = lv_color_hex(0x333333),
        .thickness = 6,
        .tick_count = 0,
        .filled = false
    };

    struct circular_arc battery_arc = {
        .start_angle_deg = quadrant * 90,
        .end_angle_deg = (quadrant * 90) + 60,
        .inner_radius = 82,
        .outer_radius = 88
    };

    ring_draw_arc(&local_ctx, &battery_arc, &battery_bg);

    // Draw battery level
    if (battery_level > 0) {
        lv_color_t battery_color;
        if (battery_level < 20) {
            battery_color = lv_color_hex(0xFF0000);  // Red
        } else if (battery_level < 50) {
            battery_color = lv_color_hex(0xFFAA00);  // Orange
        } else {
            battery_color = lv_color_hex(0x00FF00);  // Green
        }

        struct ring_style battery_fill = {
            .color = battery_color,
            .thickness = 6,
            .tick_count = 0,
            .filled = false
        };

        uint16_t fill_angle = (battery_level * 60) / 100;
        struct circular_arc fill_arc = {
            .start_angle_deg = quadrant * 90,
            .end_angle_deg = (quadrant * 90) + fill_angle,
            .inner_radius = 82,
            .outer_radius = 88
        };

        ring_draw_arc(&local_ctx, &fill_arc, &battery_fill);
    }

    return 0;
}

int ring_draw_ble_status(const struct ring_context *ctx,
                         uint8_t device_index,
                         uint8_t connection_status) {
    if (ctx == NULL || device_index > 3) {
        LOG_ERR("Invalid parameters");
        return -1;
    }

    struct circular_arc ble_arc = get_ble_arc(device_index);

    lv_color_t status_color;
    switch (connection_status) {
        case 0: // Disconnected
            status_color = lv_color_hex(0xFF0000);
            break;
        case 1: // Connecting
            status_color = lv_color_hex(0xFFAA00);
            break;
        case 2: // Connected
            status_color = lv_color_hex(0x00FF00);
            break;
        default:
            status_color = lv_color_hex(0x666666);
            break;
    }

    struct ring_style ble_style = {
        .color = status_color,
        .thickness = 5,
        .tick_count = 0,
        .filled = false
    };

    return ring_draw_arc(ctx, &ble_arc, &ble_style);
}

void ring_cleanup(struct ring_context *ctx) {
    if (ctx != NULL) {
        ctx->canvas = NULL;
        ctx->draw_buf = NULL;
    }
}

// Internal helper functions
static lv_point_t get_point_on_circle(uint16_t center_x, uint16_t center_y,
                                     uint16_t radius, uint16_t angle_deg) {
    lv_point_t point;

    float angle_rad = (angle_deg * 3.14159f) / 180.0f;
    point.x = center_x + (int16_t)(radius * cos(angle_rad));
    point.y = center_y + (int16_t)(radius * sin(angle_rad));

    return point;
}