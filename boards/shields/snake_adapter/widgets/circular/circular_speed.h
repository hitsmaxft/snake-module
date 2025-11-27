/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <lvgl.h>
#include <zephyr/kernel.h>
#include "circular_layout.h"
#include "circular_rings.h"

// Speed display configuration
#define SPEED_MAX_WPM 200
#define SPEED_FONT_SIZE 48
#define SPEED_GAUGE_MAX_ANGLE 270
#define SPEED_GAUGE_START_ANGLE 135

// Speed animation structure
struct speed_animation {
    uint16_t current_speed;
    uint16_t target_speed;
    uint16_t animation_steps;
    uint8_t current_step;
    uint16_t animation_delay_ms;
    lv_timer_t *animation_timer;
};

// Speed display state
struct speed_display_state {
    uint8_t current_wpm;
    uint16_t peak_wpm;
    uint16_t average_wpm;
    uint32_t total_keystrokes;
    uint32_t session_start_time;
    bool display_active;
    struct speed_animation animation;
};

// Color scheme for speed levels
enum speed_level {
    SPEED_LEVEL_SLOW = 0,      // 0-40 WPM
    SPEED_LEVEL_NORMAL = 1,     // 41-80 WPM
    SPEED_LEVEL_FAST = 2,      // 81-120 WPM
    SPEED_LEVEL_VERY_FAST = 3, // 121-160 WPM
    SPEED_LEVEL_EXTREME = 4    // 161+ WPM
};

// === Speed Display Lifecycle ===

/**
 * Initialize circular speed display widget
 * @return 0 on success, error code on failure
 */
int circular_speed_init(void);

/**
 * Start speed display (begin showing speed)
 * @return 0 on success, error code on failure
 */
int circular_speed_start(void);

/**
 * Stop speed display (pause/stop showing speed)
 * @return 0 on success, error code on failure
 */
int circular_speed_stop(void);

/**
 * Update speed display with new WPM value
 * @param wpm New WPM value
 * @param animated Whether to animate the change
 * @return 0 on success, error code on failure
 */
int circular_speed_update_wpm(uint8_t wpm, bool animated);

// === Display Rendering Functions ===

/**
 * Render complete speed display (central WPM + gauge)
 * @param canvas LVGL canvas for drawing
 * @return 0 on success, error code on failure
 */
int circular_speed_render_display(lv_obj_t *canvas);

/**
 * Render central WPM text display
 * @param canvas LVGL canvas for drawing
 * @param wpm WPM value to display
 * @return 0 on success, error code on failure
 */
int circular_speed_render_wpm_text(lv_obj_t *canvas, uint8_t wpm);

/**
 * Render speed gauge arc
 * @param canvas LVGL canvas for drawing
 * @param current_wpm Current WPM value
 * @return 0 on success, error code on failure
 */
int circular_speed_render_gauge(lv_obj_t *canvas, uint8_t current_wpm);

/**
 * Render speed level indicator
 * @param canvas LVGL canvas for drawing
 * @param level Speed level enum
 * @return 0 on success, error code on failure
 */
int circular_speed_render_level_indicator(lv_obj_t *canvas, enum speed_level level);

// === Gauge and Animation Functions ===

/**
 * Update gauge position based on speed
 * @param current_wpm Current WPM value
 * @return Gauge angle in degrees
 */
uint16_t circular_speed_get_gauge_angle(uint8_t current_wpm);

/**
 * Get speed level for WPM value
 * @param wpm WPM value
 * @return Speed level enum
 */
enum speed_level circular_speed_get_level(uint8_t wpm);

/**
 * Get color for speed level
 * @param level Speed level enum
 * @return LVGL color
 */
lv_color_t circular_speed_get_level_color(enum speed_level level);

/**
 * Start speed animation from current to target value
 * @param target_wpm Target WPM value
 * @param animation_ms Animation duration in milliseconds
 * @return 0 on success, error code on failure
 */
int circular_speed_animate_to_wpm(uint8_t target_wpm, uint16_t animation_ms);

/**
 * Stop current speed animation
 */
void circular_speed_stop_animation(void);

// === Statistics and Tracking ===

/**
 * Update speed statistics
 * @param wpm Current WPM value
 */
void circular_speed_update_statistics(uint8_t wpm);

/**
 * Get current speed statistics
 * @param current_wpm Pointer to store current WPM
 * @param peak_wpm Pointer to store peak WPM
 * @param average_wpm Pointer to store average WPM
 * @return 0 on success, error code on failure
 */
int circular_speed_get_statistics(uint8_t *current_wpm,
                                  uint16_t *peak_wpm,
                                  uint16_t *average_wpm);

/**
 * Reset speed statistics (start new session)
 */
void circular_speed_reset_statistics(void);

/**
 * Get session duration in minutes
 * @return Session duration in minutes
 */
uint32_t circular_speed_get_session_duration(void);

// === Display Modes ===

/**
 * Set speed display mode
 * @param mode Display mode (text_only, gauge_only, combined)
 * @return 0 on success, error code on failure
 */
int circular_speed_set_display_mode(uint8_t mode);

/**
 * Get current display mode
 * @return Current display mode
 */
uint8_t circular_speed_get_display_mode(void);

/**
 * Toggle between display modes
 * @return New display mode
 */
uint8_t circular_speed_toggle_display_mode(void);

// === Event Integration ===

/**
 * Handle ZMK WPM state changed event
 * @param wpm New WPM value
 */
void circular_speed_on_wpm_changed(uint8_t wpm);

/**
 * Handle display visibility change
 * @param visible Whether display should be visible
 * @return 0 on success, error code on failure
 */
int circular_speed_set_visible(bool visible);

/**
 * Check if speed display is currently visible
 * @return True if visible, false otherwise
 */
bool circular_speed_is_visible(void);

// === Configuration and Theming ===

/**
 * Set speed display colors
 * @param text_color Color for WPM text
 * @param gauge_color Color for speed gauge
 * @param background_color Color for background
 * @return 0 on success, error code on failure
 */
int circular_speed_set_colors(lv_color_t text_color,
                              lv_color_t gauge_color,
                              lv_color_t background_color);

/**
 * Apply theme to speed display
 * @param theme_color Primary theme color
 * @return 0 on success, error code on failure
 */
int circular_speed_apply_theme(lv_color_t theme_color);

/**
 * Configure gauge parameters
 * @param max_wpm Maximum WPM for gauge calibration
 * @param start_angle Starting angle for gauge
 * @param max_angle Maximum gauge sweep angle
 * @return 0 on success, error code on failure
 */
int circular_speed_configure_gauge(uint16_t max_wpm,
                                   uint16_t start_angle,
                                   uint16_t max_angle);

// === Utility Functions ===

/**
 * Format WPM value for display
 * @param wpm WPM value to format
 * @param buffer Buffer to store formatted string
 * @param buffer_size Buffer size
 * @return 0 on success, error code on failure
 */
int circular_speed_format_wpm(uint8_t wpm, char *buffer, size_t buffer_size);

/**
 * Validate WPM value
 * @param wpm WPM value to validate
 * @return True if valid, false otherwise
 */
bool circular_speed_is_valid_wpm(uint8_t wpm);

/**
 * Get gauge position for specific angle
 * @param angle_deg Angle in degrees
 * @return Cartesian position on gauge
 */
struct cartesian_position circular_speed_get_gauge_position(uint16_t angle_deg);

// === Cleanup and Maintenance ===

/**
 * Cleanup speed display resources
 */
void circular_speed_cleanup(void);

/**
 * Check speed display health
 * @return 0 if healthy, error code if issues detected
 */
int circular_speed_health_check(void);