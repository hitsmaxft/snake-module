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
#include "circular_speed.h"
#include "circular_battery.h"
#include "circular_ble_status.h"

// Circular UI Display Mode
enum circular_ui_mode {
    UI_MODE_SPEED_FOCUSED = 0,    // Large speed display + indicators
    UI_MODE_BALANCED = 1,          // Equal emphasis on all elements
    UI_MODE_INFORMATION = 2,         // Detailed information display
    UI_MODE_EXPERIMENTAL = 3       // Experimental interaction modes
};

// Circular UI State
struct circular_ui_state {
    // Display configuration
    bool display_active;
    enum circular_ui_mode current_mode;
    lv_obj_t *main_canvas;
    struct ring_context *ring_context;

    // Component states
    struct speed_display_state speed_state;
    struct battery_display_state battery_state;
    struct ble_display_state ble_state;

    // Animation and timing
    uint32_t last_update_time;
    uint16_t refresh_interval_ms;
    bool animations_enabled;
    lv_timer_t *refresh_timer;

    // Theme and appearance
    lv_color_t primary_theme_color;
    lv_color_t secondary_theme_color;
    lv_color_t background_color;
    bool dark_mode_enabled;

    // Performance monitoring
    uint32_t frame_count;
    uint32_t total_render_time_ms;
    uint32_t max_frame_time_ms;
    uint8_t current_fps;

    // Experimental features
    bool experimental_interactions;
    uint8_t interaction_sensitivity;
    bool gesture_support;
};

// === Circular UI Lifecycle ===

/**
 * Initialize the complete circular UI system
 * @return 0 on success, error code on failure
 */
int circular_ui_init(void);

/**
 * Start the circular UI display
 * @return 0 on success, error code on failure
 */
int circular_ui_start(void);

/**
 * Stop the circular UI display
 * @return 0 on success, error code on failure
 */
int circular_ui_stop(void);

/**
 * Cleanup all circular UI resources
 */
void circular_ui_cleanup(void);

// === Main Display Functions ===

/**
 * Render the complete circular UI display
 * @return 0 on success, error code on failure
 */
int circular_ui_render_display(void);

/**
 * Refresh the display (called periodically)
 * @return 0 on success, error code on failure
 */
int circular_ui_refresh_display(void);

/**
 * Render based on current UI mode
 * @param mode UI display mode
 * @return 0 on success, error code on failure
 */
int circular_ui_render_mode(enum circular_ui_mode mode);

/**
 * Switch to different UI mode
 * @param mode New UI mode to switch to
 * @return 0 on success, error code on failure
 */
int circular_ui_switch_mode(enum circular_ui_mode mode);

/**
 * Get current UI mode
 * @return Current UI mode
 */
enum circular_ui_mode circular_ui_get_current_mode(void);

// === Component Integration ===

/**
 * Initialize all circular UI components
 * @return 0 on success, error code on failure
 */
int circular_ui_init_components(void);

/**
 * Start all UI components
 * @return 0 on success, error code on failure
 */
int circular_ui_start_components(void);

/**
 * Stop all UI components
 * @return 0 on success, error code on failure
 */
int circular_ui_stop_components(void);

/**
 * Update all UI components with new data
 * @return 0 on success, error code on failure
 */
int circular_ui_update_components(void);

// === Event Integration ===

/**
 * Handle ZMK WPM state changed event
 * @param wpm New WPM value
 */
void circular_ui_on_wpm_changed(uint8_t wpm);

/**
 * Handle ZMK battery state changed event
 * @param peripheral_id Peripheral identifier
 * @param battery_percentage New battery level
 * @param is_charging True if charging
 */
void circular_ui_on_battery_changed(uint8_t peripheral_id,
                                     uint8_t battery_percentage,
                                     bool is_charging);

/**
 * Handle ZMK BLE connection state changed event
 * @param profile_id Profile identifier
 * @param connected True if connected
 */
void circular_ui_on_ble_changed(uint8_t profile_id, bool connected);

/**
 * Handle ZMK layer changed event
 * @param layer New layer number
 */
void circular_ui_on_layer_changed(uint8_t layer);

/**
 * Handle keyboard input for experimental interactions
 * @param keycode Key code that was pressed
 * @param pressed True if key was pressed, false if released
 */
void circular_ui_on_key_input(uint16_t keycode, bool pressed);

// === Theme and Appearance ===

/**
 * Apply a theme to the circular UI
 * @param primary_color Primary theme color
 * @param secondary_color Secondary theme color
 * @param dark_mode True for dark theme, false for light
 * @return 0 on success, error code on failure
 */
int circular_ui_apply_theme(lv_color_t primary_color,
                             lv_color_t secondary_color,
                             bool dark_mode);

/**
 * Cycle through predefined themes
 * @return 0 on success, error code on failure
 */
int circular_ui_cycle_theme(void);

/**
 * Set theme colors explicitly
 * @param primary Primary theme color
 * @param secondary Secondary theme color
 * @param background Background color
 * @return 0 on success, error code on failure
 */
int circular_ui_set_theme_colors(lv_color_t primary,
                                 lv_color_t secondary,
                                 lv_color_t background);

/**
 * Get current theme colors
 * @param primary Pointer to store primary color
 * @param secondary Pointer to store secondary color
 * @param background Pointer to store background color
 * @return 0 on success, error code on failure
 */
int circular_ui_get_theme_colors(lv_color_t *primary,
                                 lv_color_t *secondary,
                                 lv_color_t *background);

// === Configuration and Settings ===

/**
 * Set refresh interval for display updates
 * @param interval_ms Refresh interval in milliseconds
 * @return 0 on success, error code on failure
 */
int circular_ui_set_refresh_interval(uint16_t interval_ms);

/**
 * Enable or disable animations
 * @param enabled True to enable animations
 * @return 0 on success, error code on failure
 */
int circular_ui_set_animations_enabled(bool enabled);

/**
 * Enable or disable experimental features
 * @param enabled True to enable experimental features
 * @return 0 on success, error code on failure
 */
int circular_ui_set_experimental_features(bool enabled);

/**
 * Set interaction sensitivity for experimental features
 * @param sensitivity Sensitivity level (1-10)
 * @return 0 on success, error code on failure
 */
int circular_ui_set_interaction_sensitivity(uint8_t sensitivity);

/**
 * Get current UI configuration
 * @param refresh_interval Pointer to store refresh interval
 * @param animations_enabled Pointer to store animation setting
 * @param experimental_enabled Pointer to store experimental setting
 * @return 0 on success, error code on failure
 */
int circular_ui_get_configuration(uint16_t *refresh_interval,
                                   bool *animations_enabled,
                                   bool *experimental_enabled);

// === Performance Monitoring ===

/**
 * Start performance monitoring
 */
void circular_ui_start_performance_monitoring(void);

/**
 * Stop performance monitoring
 */
void circular_ui_stop_performance_monitoring(void);

/**
 * Get current performance metrics
 * @param fps Pointer to store current FPS
 * @param frame_time Pointer to store average frame time
 * @param max_frame_time Pointer to store maximum frame time
 * @return 0 on success, error code on failure
 */
int circular_ui_get_performance_metrics(uint8_t *fps,
                                       uint32_t *frame_time,
                                       uint32_t *max_frame_time);

/**
 * Reset performance metrics
 */
void circular_ui_reset_performance_metrics(void);

// === Experimental Features ===

/**
 * Enable experimental gesture support
 * @param enabled True to enable gestures
 * @return 0 on success, error code on failure
 */
int circular_ui_enable_gesture_support(bool enabled);

/**
 * Process gesture input
 * @param gesture_type Type of gesture detected
 * @param gesture_data Gesture-specific data
 * @return 0 on success, error code on failure
 */
int circular_ui_process_gesture(uint8_t gesture_type, void *gesture_data);

/**
 * Get current experimental interaction state
 * @param gesture_support Pointer to store gesture support status
 * @param sensitivity Pointer to store sensitivity level
 * @return 0 on success, error code on failure
 */
int circular_ui_get_experimental_state(bool *gesture_support,
                                       uint8_t *sensitivity);

// === Utility Functions ===

/**
 * Check if circular UI is currently active
 * @return True if active, false otherwise
 */
bool circular_ui_is_active(void);

/**
 * Validate UI mode
 * @param mode UI mode to validate
 * @return True if valid, false otherwise
 */
bool circular_ui_is_valid_mode(enum circular_ui_mode mode);

/**
 * Get UI mode description
 * @param mode UI mode
 * @return String description
 */
const char* circular_ui_get_mode_description(enum circular_ui_mode mode);

/**
 * Check if performance is within acceptable limits
 * @return True if performance is acceptable, false otherwise
 */
bool circular_ui_is_performance_acceptable(void);

/**
 * Force immediate refresh of display
 * @return 0 on success, error code on failure
 */
int circular_ui_force_refresh(void);

// === Health and Diagnostics ===

/**
 * Check health of entire circular UI system
 * @return 0 if healthy, error code if issues detected
 */
int circular_ui_health_check(void);

/**
 * Run comprehensive diagnostics
 * @return 0 if all tests pass, error codes for failures
 */
int circular_ui_run_diagnostics(void);

/**
 * Get system status summary
 * @param buffer Buffer to store status summary
 * @param buffer_size Buffer size
 * @return 0 on success, error code on failure
 */
int circular_ui_get_status_summary(char *buffer, size_t buffer_size);

/**
 * Reset circular UI to safe state
 * @return 0 on success, error code on failure
 */
int circular_ui_reset_to_safe_state(void);

// === Constants and Macros ===

#define CIRCULAR_UI_REFRESH_DEFAULT 100    // 100ms refresh interval
#define CIRCULAR_UI_MIN_REFRESH 50          // 50ms minimum refresh
#define CIRCULAR_UI_MAX_REFRESH 500         // 500ms maximum refresh
#define CIRCULAR_UI_SENSITIVITY_MIN 1       // Minimum sensitivity
#define CIRCULAR_UI_SENSITIVITY_MAX 10      // Maximum sensitivity
#define CIRCULAR_UI_SENSITIVITY_DEFAULT 5   // Default sensitivity

// Predefined theme colors
extern const lv_color_t CIRCULAR_THEME_BLUE;
extern const lv_color_t CIRCULAR_THEME_GREEN;
extern const lv_color_t CIRCULAR_THEME_RED;
extern const lv_color_t CIRCULAR_THEME_PURPLE;
extern const lv_color_t CIRCULAR_THEME_ORANGE;
extern const lv_color_t CIRCULAR_THEME_WHITE;
extern const lv_color_t CIRCULAR_THEME_BLACK;

// Gesture types for experimental features
#define GESTURE_TYPE_SWIPE_UP 1
#define GESTURE_TYPE_SWIPE_DOWN 2
#define GESTURE_TYPE_SWIPE_LEFT 3
#define GESTURE_TYPE_SWIPE_RIGHT 4
#define GESTURE_TYPE_TAP 5
#define GESTURE_TYPE_DOUBLE_TAP 6
#define GESTURE_TYPE_PINCH 7
#define GESTURE_TYPE_SPREAD 8