/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include "circular_speed.h"
#include "../helpers/display.h"
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <string.h>
#include <stdio.h>
#include <zmk/events/wpm_state_changed.h>
#include <zmk/wpm.h>

LOG_MODULE_DECLARE(circular_speed, CONFIG_ZMK_LOG_LEVEL);

// Display modes
#define DISPLAY_MODE_TEXT_ONLY 0
#define DISPLAY_MODE_GAUGE_ONLY 1
#define DISPLAY_MODE_COMBINED 2

// Global state
static struct speed_display_state speed_state = {
    .current_wpm = 0,
    .peak_wpm = 0,
    .average_wpm = 0,
    .total_keystrokes = 0,
    .session_start_time = 0,
    .display_active = false
};

static uint8_t current_display_mode = DISPLAY_MODE_COMBINED;

// Theme colors (can be overridden)
static lv_color_t current_text_color = lv_color_hex(0xFFFFFF);
static lv_color_t current_gauge_color = lv_color_hex(0x00FF00);
static lv_color_t current_bg_color = lv_color_hex(0x000000);

// Forward declarations for internal functions
static void speed_animation_timer_cb(lv_timer_t *timer);
static int render_speed_text_internal(lv_obj_t *canvas, const char *text);
static lv_color_t interpolate_color_for_speed(uint8_t wpm);

int circular_speed_init(void) {
    LOG_INF("Initializing circular speed display");

    // Reset state
    memset(&speed_state, 0, sizeof(speed_state));
    speed_state.animation.target_speed = 0;
    speed_state.animation.animation_steps = 20;
    speed_state.animation.animation_delay_ms = 50;

    // Initialize session start time
    speed_state.session_start_time = k_uptime_get_32();

    LOG_DBG("Speed display initialized successfully");
    return 0;
}

int circular_speed_start(void) {
    LOG_INF("Starting circular speed display");

    speed_state.display_active = true;
    speed_state.session_start_time = k_uptime_get_32();

    // Render initial display
    lv_obj_t *canvas = get_display_canvas();
    if (canvas != NULL) {
        circular_speed_render_display(canvas);
    }

    return 0;
}

int circular_speed_stop(void) {
    LOG_INF("Stopping circular speed display");

    speed_state.display_active = false;
    circular_speed_stop_animation();

    return 0;
}

int circular_speed_update_wpm(uint8_t wpm, bool animated) {
    if (!speed_state.display_active) {
        return 0;  // silently ignore if display not active
    }

    if (!circular_speed_is_valid_wpm(wpm)) {
        LOG_WRN("Invalid WPM value: %d", wpm);
        return -1;
    }

    LOG_DBG("Updating WPM: %d -> %d (animated: %s)",
            speed_state.current_wpm, wpm, animated ? "yes" : "no");

    // Update statistics
    circular_speed_update_statistics(wpm);

    if (animated && speed_state.current_wpm != wpm) {
        return circular_speed_animate_to_wpm(wpm, 1000);  // 1 second animation
    } else {
        speed_state.current_wpm = wpm;
        speed_state.animation.current_speed = wpm;

        // Render immediately
        lv_obj_t *canvas = get_display_canvas();
        if (canvas != NULL) {
            return circular_speed_render_display(canvas);
        }
    }

    return 0;
}

int circular_speed_render_display(lv_obj_t *canvas) {
    if (canvas == NULL || !speed_state.display_active) {
        LOG_ERR("Invalid canvas or display not active");
        return -1;
    }

    // Clear background area (center circle)
    struct cartesian_position center = get_center_position(0);
    uint16_t clear_radius = 40;
    lv_draw_rect(canvas->draw_buf,
                 center.x - clear_radius, center.y - clear_radius,
                 center.x + clear_radius, center.y + clear_radius,
                 current_bg_color);

    int ret = 0;

    // Render based on display mode
    switch (current_display_mode) {
        case DISPLAY_MODE_TEXT_ONLY:
            ret = circular_speed_render_wpm_text(canvas, speed_state.current_wpm);
            break;

        case DISPLAY_MODE_GAUGE_ONLY:
            ret = circular_speed_render_gauge(canvas, speed_state.current_wpm);
            break;

        case DISPLAY_MODE_COMBINED:
        default:
            ret = circular_speed_render_gauge(canvas, speed_state.current_wpm);
            if (ret == 0) {
                ret = circular_speed_render_wpm_text(canvas, speed_state.current_wpm);
            }
            break;
    }

    // Render speed level indicator
    if (ret == 0) {
        enum speed_level level = circular_speed_get_level(speed_state.current_wpm);
        ret = circular_speed_render_level_indicator(canvas, level);
    }

    return ret;
}

int circular_speed_render_wpm_text(lv_obj_t *canvas, uint8_t wpm) {
    if (canvas == NULL) {
        LOG_ERR("NULL canvas");
        return -1;
    }

    char wpm_text[8];
    int ret = circular_speed_format_wpm(wpm, wpm_text, sizeof(wpm_text));
    if (ret != 0) {
        LOG_ERR("Failed to format WPM text");
        return ret;
    }

    return render_speed_text_internal(canvas, wpm_text);
}

int circular_speed_render_gauge(lv_obj_t *canvas, uint8_t current_wpm) {
    if (canvas == NULL) {
        LOG_ERR("NULL canvas");
        return -1;
    }

    // Initialize ring context for gauge
    struct circular_position center_pos = {
        .angle_deg = 0,
        .radius = 0  // Use display center
    };

    struct ring_context *ring_ctx = ring_init(canvas, &center_pos);
    if (ring_ctx == NULL) {
        LOG_ERR("Failed to initialize ring context");
        return -1;
    }

    int ret = 0;

    // Draw gauge background
    struct ring_style gauge_bg = RING_STYLE_MEDIUM;
    gauge_bg.color = lv_color_hex(0x333333);
    gauge_bg.thickness = 8;

    struct circular_arc gauge_bg_arc = {
        .start_angle_deg = SPEED_GAUGE_START_ANGLE,
        .end_angle_deg = SPEED_GAUGE_START_ANGLE + SPEED_GAUGE_MAX_ANGLE,
        .inner_radius = 55,
        .outer_radius = 70
    };

    ret = ring_draw_arc(ring_ctx, &gauge_bg_arc, &gauge_bg);
    if (ret != 0) {
        LOG_ERR("Failed to draw gauge background");
        return ret;
    }

    // Draw gauge progress if speed > 0
    if (current_wpm > 0) {
        uint16_t gauge_angle = circular_speed_get_gauge_angle(current_wpm);
        lv_color_t gauge_color = interpolate_color_for_speed(current_wpm);

        struct ring_style gauge_progress = RING_STYLE_MEDIUM;
        gauge_progress.color = gauge_color;
        gauge_progress.thickness = 6;

        struct circular_arc gauge_progress_arc = {
            .start_angle_deg = SPEED_GAUGE_START_ANGLE,
            .end_angle_deg = SPEED_GAUGE_START_ANGLE + gauge_angle,
            .inner_radius = 56,
            .outer_radius = 69
        };

        ret = ring_draw_arc(ring_ctx, &gauge_progress_arc, &gauge_progress);
        if (ret != 0) {
            LOG_ERR("Failed to draw gauge progress");
            return ret;
        }
    }

    // Draw tick marks for gauge
    struct tick_style gauge_ticks = TICK_STYLE_SPEEDOMETER;
    ret = ring_draw_ticks(ring_ctx, 75, &gauge_ticks);

    return ret;
}

int circular_speed_render_level_indicator(lv_obj_t *canvas, enum speed_level level) {
    if (canvas == NULL) {
        LOG_ERR("NULL canvas");
        return -1;
    }

    // Draw small indicator dots for speed level
    lv_color_t level_color = circular_speed_get_level_color(level);
    struct cartesian_position center = get_center_position(0);

    // Draw 5 dots at bottom of display to indicate level
    for (uint8_t i = 0; i <= 4; i++) {
        lv_color_t dot_color = (i <= level) ? level_color : lv_color_hex(0x333333);
        uint16_t dot_x = center.x - 12 + (i * 6);
        uint16_t dot_y = center.y + 50;

        lv_draw_rect(canvas->draw_buf, dot_x - 1, dot_y - 1, dot_x + 1, dot_y + 1, dot_color);
    }

    return 0;
}

uint16_t circular_speed_get_gauge_angle(uint8_t current_wpm) {
    if (current_wpm == 0) {
        return 0;
    }

    // Scale WPM to gauge angle
    uint32_t angle = (current_wpm * SPEED_GAUGE_MAX_ANGLE) / SPEED_MAX_WPM;
    return (uint16_t)angle;
}

enum speed_level circular_speed_get_level(uint8_t wpm) {
    if (wpm <= 40) return SPEED_LEVEL_SLOW;
    if (wpm <= 80) return SPEED_LEVEL_NORMAL;
    if (wpm <= 120) return SPEED_LEVEL_FAST;
    if (wpm <= 160) return SPEED_LEVEL_VERY_FAST;
    return SPEED_LEVEL_EXTREME;
}

lv_color_t circular_speed_get_level_color(enum speed_level level) {
    switch (level) {
        case SPEED_LEVEL_SLOW:     return lv_color_hex(0x0080FF);  // Blue
        case SPEED_LEVEL_NORMAL:   return lv_color_hex(0x00FF00);  // Green
        case SPEED_LEVEL_FAST:     return lv_color_hex(0xFFFF00);  // Yellow
        case SPEED_LEVEL_VERY_FAST:return lv_color_hex(0xFF8000);  // Orange
        case SPEED_LEVEL_EXTREME:  return lv_color_hex(0xFF0000);  // Red
        default:                   return lv_color_hex(0xFFFFFF);  // White
    }
}

int circular_speed_animate_to_wpm(uint8_t target_wpm, uint16_t animation_ms) {
    if (!circular_speed_is_valid_wpm(target_wpm)) {
        return -1;
    }

    LOG_DBG("Starting animation: %d -> %d (%dms)",
            speed_state.current_wpm, target_wpm, animation_ms);

    // Stop any existing animation
    circular_speed_stop_animation();

    // Setup new animation
    speed_state.animation.target_speed = target_wpm;
    speed_state.animation.current_speed = speed_state.current_wpm;
    speed_state.animation.current_step = 0;
    speed_state.animation.animation_steps = animation_ms / speed_state.animation.animation_delay_ms;

    // Create animation timer
    speed_state.animation.animation_timer =
        lv_timer_create(speed_animation_timer_cb, speed_state.animation.animation_delay_ms, NULL);

    if (speed_state.animation.animation_timer == NULL) {
        LOG_ERR("Failed to create animation timer");
        return -1;
    }

    return 0;
}

void circular_speed_stop_animation(void) {
    if (speed_state.animation.animation_timer != NULL) {
        lv_timer_del(speed_state.animation.animation_timer);
        speed_state.animation.animation_timer = NULL;
    }
}

void circular_speed_update_statistics(uint8_t wpm) {
    speed_state.current_wpm = wpm;

    // Update peak
    if (wpm > speed_state.peak_wpm) {
        speed_state.peak_wpm = wpm;
    }

    // Update running average (simple implementation)
    static uint32_t sample_count = 0;
    speed_state.average_wpm = ((speed_state.average_wpm * sample_count) + wpm) / (sample_count + 1);
    sample_count++;
}

int circular_speed_get_statistics(uint8_t *current_wpm,
                                  uint16_t *peak_wpm,
                                  uint16_t *average_wpm) {
    if (current_wpm != NULL) *current_wpm = speed_state.current_wpm;
    if (peak_wpm != NULL) *peak_wpm = speed_state.peak_wpm;
    if (average_wpm != NULL) *average_wpm = speed_state.average_wpm;

    return 0;
}

void circular_speed_reset_statistics(void) {
    speed_state.peak_wpm = 0;
    speed_state.average_wpm = 0;
    speed_state.total_keystrokes = 0;
    speed_state.session_start_time = k_uptime_get_32();
}

uint32_t circular_speed_get_session_duration(void) {
    if (speed_state.session_start_time == 0) {
        return 0;
    }

    uint32_t current_time = k_uptime_get_32();
    return (current_time - speed_state.session_start_time) / 1000 / 60;  // Convert to minutes
}

int circular_speed_set_colors(lv_color_t text_color,
                              lv_color_t gauge_color,
                              lv_color_t background_color) {
    current_text_color = text_color;
    current_gauge_color = gauge_color;
    current_bg_color = background_color;

    // Re-render if display is active
    if (speed_state.display_active) {
        lv_obj_t *canvas = get_display_canvas();
        if (canvas != NULL) {
            return circular_speed_render_display(canvas);
        }
    }

    return 0;
}

// Internal helper functions
static void speed_animation_timer_cb(lv_timer_t *timer) {
    struct speed_animation *anim = &speed_state.animation;

    if (anim->current_step >= anim->animation_steps) {
        // Animation complete
        speed_state.current_wpm = anim->target_speed;
        circular_speed_stop_animation();
        LOG_DBG("Animation complete: final speed %d", speed_state.current_wpm);
        return;
    }

    // Calculate intermediate value
    int16_t speed_diff = (int16_t)anim->target_speed - (int16_t)anim->current_speed;
    int16_t step_size = speed_diff / (anim->animation_steps - anim->current_step);

    anim->current_speed += step_size;
    anim->current_step++;

    // Update display
    speed_state.current_wpm = (uint8_t)anim->current_speed;

    lv_obj_t *canvas = get_display_canvas();
    if (canvas != NULL) {
        circular_speed_render_display(canvas);
    }
}

static int render_speed_text_internal(lv_obj_t *canvas, const char *text) {
    if (canvas == NULL || text == NULL) {
        return -1;
    }

    // Center the text
    struct cartesian_position center = get_center_position(0);
    lv_point_t text_pos = {
        .x = center.x,
        .y = center.y
    };

    // Draw WPM text with large font
    lv_draw_text(canvas->draw_buf, text, &text_pos, current_text_color);

    // Draw "WPM" label below
    lv_point_t label_pos = {
        .x = center.x,
        .y = center.y + 25
    };

    lv_draw_text(canvas->draw_buf, "WPM", &label_pos, lv_color_hex(0xCCCCCC));

    return 0;
}

static lv_color_t interpolate_color_for_speed(uint8_t wpm) {
    enum speed_level level = circular_speed_get_level(wpm);
    lv_color_t base_color = circular_speed_get_level_color(level);

    // Add some intensity variation based on exact WPM
    uint8_t intensity = 200 + (wpm % 56);  // 200-255 range
    return lv_color_make(intensity, lv_color_green(base_color), lv_color_blue(base_color));
}

// Event handlers
void circular_speed_on_wpm_changed(uint8_t wpm) {
    circular_speed_update_wpm(wpm, true);  // Always animate WPM changes
}

int circular_speed_set_visible(bool visible) {
    if (visible && !speed_state.display_active) {
        return circular_speed_start();
    } else if (!visible && speed_state.display_active) {
        return circular_speed_stop();
    }
    return 0;
}

bool circular_speed_is_visible(void) {
    return speed_state.display_active;
}

bool circular_speed_is_valid_wpm(uint8_t wpm) {
    return wpm <= SPEED_MAX_WPM;
}

int circular_speed_format_wpm(uint8_t wpm, char *buffer, size_t buffer_size) {
    if (buffer == NULL || buffer_size < 4) {
        return -1;
    }

    snprintf(buffer, buffer_size, "%d", wpm);
    return 0;
}

struct cartesian_position circular_speed_get_gauge_position(uint16_t angle_deg) {
    struct circular_position gauge_pos = {
        .angle_deg = angle_deg,
        .radius = 62  // Midpoint of gauge range
    };

    return polar_to_cartesian(&gauge_pos);
}

void circular_speed_cleanup(void) {
    circular_speed_stop_animation();
    memset(&speed_state, 0, sizeof(speed_state));
}

int circular_speed_health_check(void) {
    int issues = 0;

    if (!circular_speed_is_valid_wpm(speed_state.current_wpm)) {
        LOG_ERR("Invalid current WPM: %d", speed_state.current_wpm);
        issues++;
    }

    if (speed_state.animation.animation_timer != NULL && !speed_state.display_active) {
        LOG_ERR("Animation timer active but display not active");
        issues++;
    }

    return (issues == 0) ? 0 : -1;
}