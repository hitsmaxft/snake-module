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

// Battery display configuration
#define BATTERY_MAX_DEVICES 4
#define BATTERY_MAX_PERCENTAGE 100
#define BATTERY_CRITICAL_PERCENTAGE 10
#define BATTERY_LOW_PERCENTAGE 25
#define BATTERY_GOOD_PERCENTAGE 50

// Battery visualization parameters
#define BATTERY_ARC_SPAN 60          // Degrees of arc for each battery
#define BATTERY_INNER_RADIUS 82      // Inner radius of battery arcs
#define BATTERY_OUTER_RADIUS 88      // Outer radius of battery arcs
#define BATTERY_TICK_COUNT 6         // Number of tick marks per battery

// Battery device information
struct battery_device {
    uint8_t device_id;              // Device identifier (0-3)
    uint8_t battery_percentage;     // Current battery level (0-100)
    bool is_charging;              // True if device is charging
    bool is_connected;             // True if device is connected
    enum circular_quadrant quadrant; // Display quadrant
    uint32_t last_update_time;      // Timestamp of last update
    char device_name[16];           // Optional device name
};

// Battery display state
struct battery_display_state {
    struct battery_device devices[BATTERY_MAX_DEVICES];
    uint8_t device_count;
    bool display_active;
    bool show_percentage;
    bool show_animation;
    struct ring_context *ring_ctx;
};

// Battery animation structure
struct battery_animation {
    struct battery_device *device;
    uint8_t target_percentage;
    uint8_t current_percentage;
    uint8_t animation_steps;
    uint8_t current_step;
    lv_timer_t *animation_timer;
};

// === Battery Display Lifecycle ===

/**
 * Initialize circular battery display
 * @return 0 on success, error code on failure
 */
int circular_battery_init(void);

/**
 * Start battery display
 * @return 0 on success, error code on failure
 */
int circular_battery_start(void);

/**
 * Stop battery display
 * @return 0 on success, error code on failure
 */
int circular_battery_stop(void);

/**
 * Update battery information for a device
 * @param device_id Device identifier (0-3)
 * @param battery_percentage Battery level (0-100)
 * @param is_charging True if charging
 * @param is_connected True if connected
 * @param device_name Optional device name (can be NULL)
 * @return 0 on success, error code on failure
 */
int circular_battery_update_device(uint8_t device_id,
                                   uint8_t battery_percentage,
                                   bool is_charging,
                                   bool is_connected,
                                   const char *device_name);

// === Display Rendering Functions ===

/**
 * Render all battery indicators
 * @param canvas LVGL canvas for drawing
 * @return 0 on success, error code on failure
 */
int circular_battery_render_all(lv_obj_t *canvas);

/**
 * Render battery indicator for specific device
 * @param canvas LVGL canvas for drawing
 * @param device Device information
 * @return 0 on success, error code on failure
 */
int circular_battery_render_device(lv_obj_t *canvas, const struct battery_device *device);

/**
 * Render battery arc segment
 * @param canvas LVGL canvas for drawing
 * @param device Device information
 * @return 0 on success, error code on failure
 */
int circular_battery_render_arc(lv_obj_t *canvas, const struct battery_device *device);

/**
 * Render battery percentage text
 * @param canvas LVGL canvas for drawing
 * @param device Device information
 * @return 0 on success, error code on failure
 */
int circular_battery_render_text(lv_obj_t *canvas, const struct battery_device *device);

/**
 * Render charging indicator
 * @param canvas LVGL canvas for drawing
 * @param device Device information
 * @return 0 on success, error code on failure
 */
int circular_battery_render_charging_indicator(lv_obj_t *canvas, const struct battery_device *device);

// === Battery Status Functions ===

/**
 * Get battery status color based on level
 * @param percentage Battery percentage (0-100)
 * @param is_charging True if charging
 * @return LVGL color
 */
lv_color_t circular_battery_get_status_color(uint8_t percentage, bool is_charging);

/**
 * Get battery status text
 * @param percentage Battery percentage (0-100)
 * @param is_charging True if charging
 * @param buffer Buffer to store status text
 * @param buffer_size Buffer size
 * @return 0 on success, error code on failure
 */
int circular_battery_get_status_text(uint8_t percentage,
                                    bool is_charging,
                                    char *buffer,
                                    size_t buffer_size);

/**
 * Check if battery level is critical
 * @param percentage Battery percentage
 * @return True if critical, false otherwise
 */
bool circular_battery_is_critical(uint8_t percentage);

/**
 * Check if battery level is low
 * @param percentage Battery percentage
 * @return True if low, false otherwise
 */
bool circular_battery_is_low(uint8_t percentage);

/**
 * Get battery device quadrant
 * @param device_id Device identifier
 * @return Quadrant enum
 */
enum circular_quadrant circular_battery_get_device_quadrant(uint8_t device_id);

// === Animation Functions ===

/**
 * Animate battery level change
 * @param device_id Device identifier
 * @param target_percentage Target battery level
 * @param animation_ms Animation duration in milliseconds
 * @return 0 on success, error code on failure
 */
int circular_battery_animate_level_change(uint8_t device_id,
                                           uint8_t target_percentage,
                                           uint16_t animation_ms);

/**
 * Stop all battery animations
 */
void circular_battery_stop_all_animations(void);

/**
 * Check if any battery animations are active
 * @return True if animations active, false otherwise
 */
bool circular_battery_has_active_animations(void);

// === Device Management ===

/**
 * Add new battery device
 * @param device_id Device identifier (0-3)
 * @param device_name Device name
 * @return 0 on success, error code on failure
 */
int circular_battery_add_device(uint8_t device_id, const char *device_name);

/**
 * Remove battery device
 * @param device_id Device identifier
 * @return 0 on success, error code on failure
 */
int circular_battery_remove_device(uint8_t device_id);

/**
 * Get battery device information
 * @param device_id Device identifier
 * @param device Pointer to store device information
 * @return 0 on success, error code on failure
 */
int circular_battery_get_device(uint8_t device_id, struct battery_device *device);

/**
 * Get number of active battery devices
 * @return Number of devices
 */
uint8_t circular_battery_get_device_count(void);

/**
 * Get average battery level across all devices
 * @return Average battery percentage (0-100)
 */
uint8_t circular_battery_get_average_level(void);

/**
 * Get device with lowest battery level
 * @return Device identifier, or 0xFF if no devices
 */
uint8_t circular_battery_get_lowest_battery_device(void);

// === Display Configuration ===

/**
 * Set whether to show percentage text
 * @param show_percentages True to show, false to hide
 * @return 0 on success, error code on failure
 */
int circular_battery_set_show_percentage(bool show_percentages);

/**
 * Set whether to show animations
 * @param show_animations True to show, false to hide
 * @return 0 on success, error code on failure
 */
int circular_battery_set_show_animations(bool show_animations);

/**
 * Get current display settings
 * @param show_percentages Pointer to store percentage setting
 * @param show_animations Pointer to store animation setting
 * @return 0 on success, error code on failure
 */
int circular_battery_get_display_settings(bool *show_percentages,
                                           bool *show_animations);

/**
 * Configure battery arc parameters
 * @param inner_radius Inner radius of battery arcs
 * @param outer_radius Outer radius of battery arcs
 * @param arc_span Degrees of arc per battery
 * @return 0 on success, error code on failure
 */
int circular_battery_configure_arcs(uint16_t inner_radius,
                                    uint16_t outer_radius,
                                    uint16_t arc_span);

// === Event Integration ===

/**
 * Handle ZMK battery state changed event
 * @param peripheral_id Peripheral identifier
 * @param battery_percentage New battery level
 * @param is_charging True if charging
 */
void circular_battery_on_state_changed(uint8_t peripheral_id,
                                        uint8_t battery_percentage,
                                        bool is_charging);

/**
 * Handle USB connection change (affects charging status)
 * @param usb_connected True if USB is connected
 */
void circular_battery_on_usb_changed(bool usb_connected);

// === Utility Functions ===

/**
 * Validate battery percentage
 * @param percentage Battery percentage to validate
 * @return True if valid, false otherwise
 */
bool circular_battery_is_valid_percentage(uint8_t percentage);

/**
 * Validate device ID
 * @param device_id Device identifier to validate
 * @return True if valid, false otherwise
 */
bool circular_battery_is_valid_device_id(uint8_t device_id);

/**
 * Format battery percentage for display
 * @param percentage Battery percentage
 * @param buffer Buffer to store formatted text
 * @param buffer_size Buffer size
 * @return 0 on success, error code on failure
 */
int circular_battery_format_percentage(uint8_t percentage,
                                       char *buffer,
                                       size_t buffer_size);

/**
 * Get battery level description
 * @param percentage Battery percentage
 * @return String description
 */
const char* circular_battery_get_level_description(uint8_t percentage);

// === Cleanup and Maintenance ===

/**
 * Cleanup battery display resources
 */
void circular_battery_cleanup(void);

/**
 * Check battery display health
 * @return 0 if healthy, error code if issues detected
 */
int circular_battery_health_check(void);

/**
 * Reset battery display state
 */
void circular_battery_reset(void);