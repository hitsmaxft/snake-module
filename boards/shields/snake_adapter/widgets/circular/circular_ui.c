/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include "circular_ui.h"
#include "../helpers/display.h"
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <string.h>
#include <stdio.h>
#include <zmk/events/wpm_state_changed.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/layer_state_changed.h>

LOG_MODULE_DECLARE(circular_ui, CONFIG_ZMK_LOG_LEVEL);

// Predefined theme colors
const lv_color_t CIRCULAR_THEME_BLUE = lv_color_hex(0x0080FF);
const lv_color_t CIRCULAR_THEME_GREEN = lv_color_hex(0x00FF00);
const lv_color_t CIRCULAR_THEME_RED = lv_color_hex(0xFF0000);
const lv_color_t CIRCULAR_THEME_PURPLE = lv_color_hex(0x8000FF);
const lv_color_t CIRCULAR_THEME_ORANGE = lv_color_hex(0xFF8000);
const lv_color_t CIRCULAR_THEME_WHITE = lv_color_hex(0xFFFFFF);
const lv_color_t CIRCULAR_THEME_BLACK = lv_color_hex(0x000000);

// Global state
static struct circular_ui_state ui_state = {
    .display_active = false,
    .current_mode = UI_MODE_SPEED_FOCUSED,
    .main_canvas = NULL,
    .ring_context = NULL,
    .last_update_time = 0,
    .refresh_interval_ms = CIRCULAR_UI_REFRESH_DEFAULT,
    .animations_enabled = true,
    .refresh_timer = NULL,
    .primary_theme_color = CIRCULAR_THEME_BLUE,
    .secondary_theme_color = lv_color_hex(0x004080),
    .background_color = lv_color_hex(0x000000),
    .dark_mode_enabled = true,
    .frame_count = 0,
    .total_render_time_ms = 0,
    .max_frame_time_ms = 0,
    .current_fps = 0,
    .experimental_interactions = false,
    .interaction_sensitivity = CIRCULAR_UI_SENSITIVITY_DEFAULT,
    .gesture_support = false
};

// Theme cycling array
static const lv_color_t theme_colors[] = {
    CIRCULAR_THEME_BLUE,
    CIRCULAR_THEME_GREEN,
    CIRCULAR_THEME_RED,
    CIRCULAR_THEME_PURPLE,
    CIRCULAR_THEME_ORANGE
};

static const uint8_t theme_count = sizeof(theme_colors) / sizeof(theme_colors[0]);
static uint8_t current_theme_index = 0;

// Forward declarations
static void circular_ui_refresh_timer_cb(lv_timer_t *timer);
static void update_performance_metrics(uint32_t frame_time_ms);
static int render_speed_focused_mode(void);
static int render_balanced_mode(void);
static int render_information_mode(void);
static int render_experimental_mode(void);

int circular_ui_init(void) {
    LOG_INF("Initializing circular UI system");

    // Clear state
    memset(&ui_state, 0, sizeof(ui_state));
    ui_state.refresh_interval_ms = CIRCULAR_UI_REFRESH_DEFAULT;
    ui_state.current_mode = UI_MODE_SPEED_FOCUSED;
    ui_state.primary_theme_color = theme_colors[0];
    ui_state.secondary_theme_color = lv_color_hex(0x004080);
    ui_state.background_color = lv_color_hex(0x000000);
    ui_state.dark_mode_enabled = true;
    ui_state.animations_enabled = true;
    ui_state.interaction_sensitivity = CIRCULAR_UI_SENSITIVITY_DEFAULT;

    // Initialize components
    int ret = circular_ui_init_components();
    if (ret != 0) {
        LOG_ERR("Failed to initialize circular UI components");
        return ret;
    }

    // Initialize layout system
    circular_layout_init();

    LOG_INF("Circular UI initialized successfully");
    return 0;
}

int circular_ui_start(void) {
    LOG_INF("Starting circular UI display");

    ui_state.display_active = true;
    ui_state.last_update_time = k_uptime_get_32();

    // Get display canvas
    ui_state.main_canvas = get_display_canvas();
    if (ui_state.main_canvas == NULL) {
        LOG_ERR("Failed to get display canvas");
        return -1;
    }

    // Initialize ring context
    struct circular_position center_pos = {
        .angle_deg = 0,
        .radius = 0  // Use display center
    };

    ui_state.ring_context = ring_init(ui_state.main_canvas, &center_pos);
    if (ui_state.ring_context == NULL) {
        LOG_ERR("Failed to initialize ring context");
        return -1;
    }

    // Start components
    int ret = circular_ui_start_components();
    if (ret != 0) {
        LOG_ERR("Failed to start circular UI components");
        return ret;
    }

    // Start refresh timer
    ui_state.refresh_timer = lv_timer_create(circular_ui_refresh_timer_cb,
                                             ui_state.refresh_interval_ms, NULL);
    if (ui_state.refresh_timer == NULL) {
        LOG_ERR("Failed to create refresh timer");
        return -1;
    }

    // Initial render
    ret = circular_ui_render_display();
    if (ret != 0) {
        LOG_ERR("Failed to render initial display");
        return ret;
    }

    // Start performance monitoring
    circular_ui_start_performance_monitoring();

    LOG_INF("Circular UI started successfully");
    return 0;
}

int circular_ui_stop(void) {
    LOG_INF("Stopping circular UI display");

    ui_state.display_active = false;

    // Stop components
    circular_ui_stop_components();

    // Stop refresh timer
    if (ui_state.refresh_timer != NULL) {
        lv_timer_del(ui_state.refresh_timer);
        ui_state.refresh_timer = NULL;
    }

    // Cleanup ring context
    if (ui_state.ring_context != NULL) {
        ring_cleanup(ui_state.ring_context);
        ui_state.ring_context = NULL;
    }

    // Stop performance monitoring
    circular_ui_stop_performance_monitoring();

    return 0;
}

void circular_ui_cleanup(void) {
    LOG_INF("Cleaning up circular UI resources");

    circular_ui_stop();

    // Cleanup components
    circular_speed_cleanup();
    circular_battery_cleanup();
    circular_ble_cleanup();

    // Clear state
    memset(&ui_state, 0, sizeof(ui_state));
}

int circular_ui_render_display(void) {
    if (!ui_state.display_active || ui_state.main_canvas == NULL) {
        LOG_ERR("Display not active or canvas NULL");
        return -1;
    }

    uint32_t start_time = k_uptime_get_32();

    int ret = circular_ui_render_mode(ui_state.current_mode);

    uint32_t frame_time = k_uptime_get_32() - start_time;
    update_performance_metrics(frame_time);

    return ret;
}

int circular_ui_render_mode(enum circular_ui_mode mode) {
    if (!circular_ui_is_valid_mode(mode)) {
        LOG_ERR("Invalid UI mode: %d", mode);
        return -1;
    }

    LOG_DBG("Rendering circular UI mode: %d", mode);

    int ret = 0;

    // Clear canvas
    ring_clear_canvas(ui_state.ring_context, ui_state.background_color);

    switch (mode) {
        case UI_MODE_SPEED_FOCUSED:
            ret = render_speed_focused_mode();
            break;

        case UI_MODE_BALANCED:
            ret = render_balanced_mode();
            break;

        case UI_MODE_INFORMATION:
            ret = render_information_mode();
            break;

        case UI_MODE_EXPERIMENTAL:
            ret = render_experimental_mode();
            break;

        default:
            LOG_ERR("Unknown UI mode: %d", mode);
            return -1;
    }

    // Refresh display
    if (ret == 0) {
        ring_refresh_display(ui_state.ring_context);
    }

    return ret;
}

int circular_ui_switch_mode(enum circular_ui_mode mode) {
    if (!circular_ui_is_valid_mode(mode)) {
        LOG_ERR("Invalid UI mode: %d", mode);
        return -1;
    }

    if (ui_state.current_mode == mode) {
        return 0; // Already in this mode
    }

    LOG_INF("Switching from mode %d to mode %d", ui_state.current_mode, mode);

    enum circular_ui_mode old_mode = ui_state.current_mode;
    ui_state.current_mode = mode;

    // Render new mode
    int ret = circular_ui_render_mode(mode);
    if (ret != 0) {
        LOG_ERR("Failed to render new mode, reverting");
        ui_state.current_mode = old_mode;
        circular_ui_render_mode(old_mode);
        return ret;
    }

    return 0;
}

enum circular_ui_mode circular_ui_get_current_mode(void) {
    return ui_state.current_mode;
}

int circular_ui_init_components(void) {
    LOG_DBG("Initializing circular UI components");

    int ret = 0;

    // Initialize speed display
    ret = circular_speed_init();
    if (ret != 0) {
        LOG_ERR("Failed to initialize speed display");
        return ret;
    }

    // Initialize battery display
    ret = circular_battery_init();
    if (ret != 0) {
        LOG_ERR("Failed to initialize battery display");
        return ret;
    }

    // Initialize BLE display
    ret = circular_ble_init();
    if (ret != 0) {
        LOG_ERR("Failed to initialize BLE display");
        return ret;
    }

    // Apply current theme to components
    ret = circular_speed_apply_theme(ui_state.primary_theme_color);
    if (ret != 0) {
        LOG_WRN("Failed to apply theme to speed display");
    }

    return 0;
}

int circular_ui_start_components(void) {
    LOG_DBG("Starting circular UI components");

    int ret = 0;

    // Start speed display
    ret = circular_speed_start();
    if (ret != 0) {
        LOG_ERR("Failed to start speed display");
        return ret;
    }

    // Start battery display
    ret = circular_battery_start();
    if (ret != 0) {
        LOG_ERR("Failed to start battery display");
        return ret;
    }

    // Start BLE display
    ret = circular_ble_start();
    if (ret != 0) {
        LOG_ERR("Failed to start BLE display");
        return ret;
    }

    return 0;
}

int circular_ui_stop_components(void) {
    LOG_DBG("Stopping circular UI components");

    circular_speed_stop();
    circular_battery_stop();
    circular_ble_stop();

    return 0;
}

int circular_ui_update_components(void) {
    // Components update themselves based on ZMK events
    // This function can be used for periodic updates or refreshes

    int ret = 0;

    // Refresh displays if needed
    if (ui_state.display_active && ui_state.main_canvas != NULL) {
        ret = circular_ui_render_display();
    }

    return ret;
}

// Event handlers
void circular_ui_on_wpm_changed(uint8_t wpm) {
    LOG_DBG("WPM changed: %d", wpm);
    circular_speed_on_wpm_changed(wpm);
}

void circular_ui_on_battery_changed(uint8_t peripheral_id,
                                     uint8_t battery_percentage,
                                     bool is_charging) {
    LOG_DBG("Battery changed for device %d: %d%% (charging: %s)",
            peripheral_id, battery_percentage, is_charging ? "yes" : "no");
    circular_battery_on_state_changed(peripheral_id, battery_percentage, is_charging);
}

void circular_ui_on_ble_changed(uint8_t profile_id, bool connected) {
    LOG_DBG("BLE changed for profile %d: %s", profile_id, connected ? "connected" : "disconnected");
    circular_ble_on_connection_changed(profile_id, connected);
}

void circular_ui_on_layer_changed(uint8_t layer) {
    LOG_DBG("Layer changed: %d", layer);
    // Layer display could be added here as needed
}

void circular_ui_on_key_input(uint16_t keycode, bool pressed) {
    if (!ui_state.experimental_interactions || !ui_state.gesture_support) {
        return;
    }

    LOG_DBG("Key input: %d (pressed: %s)", keycode, pressed ? "yes" : "no");

    // Process key input for experimental interactions
    // This could include mode switching, theme changes, etc.
}

// Theme functions
int circular_ui_apply_theme(lv_color_t primary_color,
                             lv_color_t secondary_color,
                             bool dark_mode) {
    LOG_DBG("Applying theme: primary=%x, secondary=%x, dark=%s",
            lv_color_hex(primary_color), lv_color_hex(secondary_color),
            dark_mode ? "yes" : "no");

    ui_state.primary_theme_color = primary_color;
    ui_state.secondary_theme_color = secondary_color;
    ui_state.dark_mode_enabled = dark_mode;
    ui_state.background_color = dark_mode ? CIRCULAR_THEME_BLACK : CIRCULAR_THEME_WHITE;

    // Apply theme to components
    int ret = 0;
    ret = circular_speed_apply_theme(primary_color);
    if (ret != 0) {
        LOG_WRN("Failed to apply theme to speed display");
    }

    // Refresh display if active
    if (ui_state.display_active) {
        ret = circular_ui_render_display();
    }

    return ret;
}

int circular_ui_cycle_theme(void) {
    current_theme_index = (current_theme_index + 1) % theme_count;
    lv_color_t new_primary = theme_colors[current_theme_index];
    lv_color_t new_secondary = lv_color_hex(lv_color_hex(new_primary) / 2);

    return circular_ui_apply_theme(new_primary, new_secondary, ui_state.dark_mode_enabled);
}

int circular_ui_set_theme_colors(lv_color_t primary,
                                 lv_color_t secondary,
                                 lv_color_t background) {
    ui_state.primary_theme_color = primary;
    ui_state.secondary_theme_color = secondary;
    ui_state.background_color = background;

    if (ui_state.display_active) {
        return circular_ui_render_display();
    }

    return 0;
}

// Mode rendering implementations
static int render_speed_focused_mode(void) {
    LOG_DBG("Rendering speed-focused mode");

    int ret = 0;

    // Render decorative outer ring
    ret = ring_draw_decorative_outer(ui_state.ring_context, ui_state.primary_theme_color);
    if (ret != 0) {
        return ret;
    }

    // Render speed display (primary focus)
    ret = circular_speed_render_display(ui_state.main_canvas);
    if (ret != 0) {
        return ret;
    }

    // Render battery indicators (secondary)
    ret = circular_battery_render_all(ui_state.main_canvas);
    if (ret != 0) {
        LOG_WRN("Failed to render battery indicators");
        // Continue with BLE rendering
    }

    // Render BLE status (secondary)
    ret = circular_ble_render_all(ui_state.main_canvas);
    if (ret != 0) {
        LOG_WRN("Failed to render BLE status");
    }

    return 0;
}

static int render_balanced_mode(void) {
    LOG_DBG("Rendering balanced mode");

    int ret = 0;

    // Render speed gauge (medium size)
    struct ring_context speed_ctx;
    struct circular_position center = {0, 0};
    speed_ctx.ring_ctx = ring_init(ui_state.main_canvas, &center);

    struct circular_arc speed_arc = {
        .start_angle_deg = 135,
        .end_angle_deg = 405,  // 270-degree gauge
        .inner_radius = 60,
        .outer_radius = 70
    };

    struct ring_style speed_style = RING_STYLE_MEDIUM;
    speed_style.color = ui_state.primary_theme_color;
    ret = ring_draw_arc(&speed_ctx, &speed_arc, &speed_style);

    // Render battery indicators
    ret = circular_battery_render_all(ui_state.main_canvas);
    if (ret != 0) {
        return ret;
    }

    // Render BLE status
    ret = circular_ble_render_all(ui_state.main_canvas);
    if (ret != 0) {
        return ret;
    }

    return 0;
}

static int render_information_mode(void) {
    LOG_DBG("Rendering information mode");

    int ret = 0;

    // Render all components with detailed information
    ret = circular_battery_render_all(ui_state.main_canvas);
    if (ret != 0) {
        return ret;
    }

    ret = circular_ble_render_all(ui_state.main_canvas);
    if (ret != 0) {
        return ret;
    }

    // Smaller speed display
    ret = circular_speed_render_display(ui_state.main_canvas);
    if (ret != 0) {
        return ret;
    }

    return 0;
}

static int render_experimental_mode(void) {
    LOG_DBG("Rendering experimental mode");

    int ret = 0;

    // Experimental rendering with enhanced animations
    ret = circular_speed_render_display(ui_state.main_canvas);
    if (ret != 0) {
        return ret;
    }

    ret = circular_battery_render_all(ui_state.main_canvas);
    if (ret != 0) {
        return ret;
    }

    ret = circular_ble_render_all(ui_state.main_canvas);
    if (ret != 0) {
        return ret;
    }

    // Additional experimental elements could be added here

    return 0;
}

// Utility functions
static void circular_ui_refresh_timer_cb(lv_timer_t *timer) {
    if (!ui_state.display_active) {
        return;
    }

    circular_ui_refresh_display();
}

static void update_performance_metrics(uint32_t frame_time_ms) {
    ui_state.frame_count++;
    ui_state.total_render_time_ms += frame_time_ms;

    if (frame_time_ms > ui_state.max_frame_time_ms) {
        ui_state.max_frame_time_ms = frame_time_ms;
    }

    // Calculate FPS every 100 frames
    if (ui_state.frame_count % 100 == 0) {
        uint32_t avg_frame_time = ui_state.total_render_time_ms / 100;
        ui_state.current_fps = 1000 / avg_frame_time;

        // Reset counters
        ui_state.frame_count = 0;
        ui_state.total_render_time_ms = 0;

        LOG_DBG("Performance: %d FPS, avg frame: %dms, max frame: %dms",
                ui_state.current_fps, avg_frame_time, ui_state.max_frame_time_ms);
    }
}

bool circular_ui_is_active(void) {
    return ui_state.display_active;
}

bool circular_ui_is_valid_mode(enum circular_ui_mode mode) {
    return (mode >= UI_MODE_SPEED_FOCUSED && mode <= UI_MODE_EXPERIMENTAL);
}

int circular_ui_refresh_display(void) {
    return circular_ui_render_display();
}

int circular_ui_health_check(void) {
    int issues = 0;

    if (!ui_state.display_active && ui_state.refresh_timer != NULL) {
        LOG_ERR("Refresh timer active but display not active");
        issues++;
    }

    if (ui_state.display_active && ui_state.main_canvas == NULL) {
        LOG_ERR("Display active but canvas NULL");
        issues++;
    }

    // Check component health
    int speed_health = circular_speed_health_check();
    if (speed_health != 0) {
        LOG_ERR("Speed display health issues: %d", speed_health);
        issues++;
    }

    int battery_health = circular_battery_health_check();
    if (battery_health != 0) {
        LOG_ERR("Battery display health issues: %d", battery_health);
        issues++;
    }

    int ble_health = circular_ble_health_check();
    if (ble_health != 0) {
        LOG_ERR("BLE display health issues: %d", ble_health);
        issues++;
    }

    return (issues == 0) ? 0 : -1;
}

const char* circular_ui_get_mode_description(enum circular_ui_mode mode) {
    switch (mode) {
        case UI_MODE_SPEED_FOCUSED: return "Speed Focused";
        case UI_MODE_BALANCED: return "Balanced";
        case UI_MODE_INFORMATION: return "Information";
        case UI_MODE_EXPERIMENTAL: return "Experimental";
        default: return "Unknown";
    }
}

void circular_ui_start_performance_monitoring(void) {
    LOG_INF("Starting performance monitoring");
    circular_ui_reset_performance_metrics();
}

void circular_ui_stop_performance_monitoring(void) {
    LOG_INF("Stopping performance monitoring");
}

void circular_ui_reset_performance_metrics(void) {
    ui_state.frame_count = 0;
    ui_state.total_render_time_ms = 0;
    ui_state.max_frame_time_ms = 0;
    ui_state.current_fps = 0;
}

int circular_ui_get_performance_metrics(uint8_t *fps,
                                       uint32_t *frame_time,
                                       uint32_t *max_frame_time) {
    if (fps != NULL) *fps = ui_state.current_fps;
    if (frame_time != NULL) {
        *frame_time = (ui_state.frame_count > 0) ?
                       ui_state.total_render_time_ms / ui_state.frame_count : 0;
    }
    if (max_frame_time != NULL) *max_frame_time = ui_state.max_frame_time_ms;

    return 0;
}