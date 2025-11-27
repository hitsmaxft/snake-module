/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include "circular_status_screen.h"
#include "widgets/circular/circular_ui.h"
#include "widgets/helpers/display.h"
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <string.h>

LOG_MODULE_DECLARE(circular_status_screen, CONFIG_ZMK_LOG_LEVEL);

// Global state
static struct circular_screen_controller screen_controller = {
    .main_screen = NULL,
    .main_canvas = NULL,
    .screen_active = false,
    .initialized = false,
    .current_screen_mode = SCREEN_MODE_AUTO,
    .current_ui_mode = UI_MODE_SPEED_FOCUSED,
    .last_mode_switch_time = 0,
    .auto_switch_interval_s = CIRCULAR_SCREEN_DEFAULT_INTERVAL,
    .current_layer = 0,
    .last_wpm = 0,
    .last_activity_time = 0,
    .user_active = false,
    .usb_connected = false,
    .ble_active = false,
    .event_count = 0,
    .last_performance_check = 0,
    .performance_score = 100,
    .auto_mode_switching = true,
    .experimental_features = false,
    .idle_timeout_s = CIRCULAR_SCREEN_DEFAULT_TIMEOUT,
    .low_power_mode = false
};

// Forward declarations
static int update_screen_mode_based_on_activity(void);
static void check_idle_timeout(void);
static void update_performance_score(void);

int circular_status_screen_init(void) {
    LOG_INF("Initializing circular status screen controller");

    // Clear state
    memset(&screen_controller, 0, sizeof(screen_controller));
    screen_controller.current_screen_mode = SCREEN_MODE_AUTO;
    screen_controller.current_ui_mode = UI_MODE_SPEED_FOCUSED;
    screen_controller.auto_switch_interval_s = CIRCULAR_SCREEN_DEFAULT_INTERVAL;
    screen_controller.auto_mode_switching = true;
    screen_controller.idle_timeout_s = CIRCULAR_SCREEN_DEFAULT_TIMEOUT;
    screen_controller.performance_score = 100;

    // Initialize circular UI
    int ret = circular_ui_init();
    if (ret != 0) {
        LOG_ERR("Failed to initialize circular UI");
        return ret;
    }

    screen_controller.initialized = true;
    LOG_INF("Circular status screen initialized successfully");
    return 0;
}

int circular_status_screen_start(void) {
    if (!screen_controller.initialized) {
        LOG_ERR("Screen controller not initialized");
        return -1;
    }

    LOG_INF("Starting circular status screen");

    screen_controller.screen_active = true;
    screen_controller.last_activity_time = k_uptime_get_32();
    screen_controller.last_mode_switch_time = k_uptime_get_32();

    // Initialize display
    ret = init_display();
    if (ret != 0) {
        LOG_ERR("Failed to initialize display");
        return ret;
    }

    // Create screen and canvas
    screen_controller.main_screen = lv_obj_create(NULL);
    if (screen_controller.main_screen == NULL) {
        LOG_ERR("Failed to create LVGL screen");
        return -1;
    }

    screen_controller.main_canvas = lv_canvas_create(screen_controller.main_screen);
    if (screen_controller.main_canvas == NULL) {
        LOG_ERR("Failed to create LVGL canvas");
        return -1;
    }

    // Set canvas size for ST7789V display
    lv_obj_set_size(screen_controller.main_canvas, 240, 240);

    // Start circular UI
    ret = circular_ui_start();
    if (ret != 0) {
        LOG_ERR("Failed to start circular UI");
        return ret;
    }

    // Update UI mode based on screen mode
    ret = circular_status_screen_update_ui_mode();
    if (ret != 0) {
        LOG_ERR("Failed to update UI mode");
        return ret;
    }

    // Load screen to display
    lv_scr_load(screen_controller.main_screen);

    // Start performance monitoring
    circular_status_screen_start_monitoring();

    LOG_INF("Circular status screen started successfully");
    return 0;
}

int circular_status_screen_stop(void) {
    if (!screen_controller.screen_active) {
        return 0; // Already stopped
    }

    LOG_INF("Stopping circular status screen");

    screen_controller.screen_active = false;

    // Stop circular UI
    circular_ui_stop();

    // Stop performance monitoring
    circular_status_screen_stop_monitoring();

    LOG_INF("Circular status screen stopped");
    return 0;
}

void circular_status_screen_cleanup(void) {
    LOG_INF("Cleaning up circular status screen resources");

    circular_status_screen_stop();
    circular_ui_cleanup();

    // Cleanup LVGL objects
    if (screen_controller.main_canvas != NULL) {
        lv_obj_del(screen_controller.main_canvas);
        screen_controller.main_canvas = NULL;
    }

    if (screen_controller.main_screen != NULL) {
        lv_obj_del(screen_controller.main_screen);
        screen_controller.main_screen = NULL;
    }

    // Clear state
    memset(&screen_controller, 0, sizeof(screen_controller));
}

// Event handlers
int circular_status_screen_on_layer_changed(const zmk_event_t *ev) {
    if (!screen_controller.screen_active || ev == NULL) {
        return 0;
    }

    // Update layer tracking
    // Note: This would need to access specific layer event data
    // screen_controller.current_layer = ev->layer;

    // Update activity
    circular_status_screen_update_activity(true);

    // Update UI if needed
    return circular_status_screen_refresh();
}

int circular_status_screen_on_wpm_changed(const zmk_event_t *ev) {
    if (!screen_controller.screen_active || ev == NULL) {
        return 0;
    }

    // Update WPM tracking
    // Note: This would need to access specific WPM event data
    // uint8_t new_wpm = ev->wpm;
    // circular_ui_on_wpm_changed(new_wpm);
    // screen_controller.last_wpm = new_wpm;

    // Update activity (typing activity)
    circular_status_screen_update_activity(true);

    // Update event count
    screen_controller.event_count++;

    return 0;
}

int circular_status_screen_on_battery_changed(const zmk_event_t *ev) {
    if (!screen_controller.screen_active || ev == NULL) {
        return 0;
    }

    // Update battery display
    // Note: This would need to access specific battery event data
    // uint8_t peripheral_id = ev->peripheral_id;
    // uint8_t battery_level = ev->battery_level;
    // bool is_charging = ev->is_charging;
    // circular_ui_on_battery_changed(peripheral_id, battery_level, is_charging);

    return 0;
}

int circular_status_screen_on_ble_changed(const zmk_event_t *ev) {
    if (!screen_controller.screen_active || ev == NULL) {
        return 0;
    }

    // Update BLE status
    // Note: This would need to access specific BLE event data
    // uint8_t profile_id = ev->profile_id;
    // bool connected = ev->connected;
    // circular_ui_on_ble_changed(profile_id, connected);

    // Update activity
    circular_status_screen_update_activity(true);

    return 0;
}

int circular_status_screen_on_usb_changed(const zmk_event_t *ev) {
    if (!screen_controller.screen_active || ev == NULL) {
        return 0;
    }

    // Update USB status
    // Note: This would need to access specific USB event data
    // bool connected = ev->connected;
    // screen_controller.usb_connected = connected;

    return 0;
}

// Mode management
int circular_status_screen_set_mode(enum circular_screen_mode mode) {
    if (!circular_status_screen_is_valid_mode(mode)) {
        LOG_ERR("Invalid screen mode: %d", mode);
        return -1;
    }

    if (screen_controller.current_screen_mode == mode) {
        return 0; // Already in this mode
    }

    LOG_INF("Switching screen mode: %d -> %d", screen_controller.current_screen_mode, mode);

    enum circular_screen_mode old_mode = screen_controller.current_screen_mode;
    screen_controller.current_screen_mode = mode;
    screen_controller.last_mode_switch_time = k_uptime_get_32();

    // Update UI mode
    int ret = circular_status_screen_update_ui_mode();
    if (ret != 0) {
        LOG_ERR("Failed to update UI mode, reverting");
        screen_controller.current_screen_mode = old_mode;
        circular_status_screen_update_ui_mode();
        return ret;
    }

    return 0;
}

int circular_status_screen_update_ui_mode(void) {
    enum circular_ui_mode new_ui_mode = UI_MODE_SPEED_FOCUSED;

    switch (screen_controller.current_screen_mode) {
        case SCREEN_MODE_AUTO:
            new_ui_mode = UI_MODE_BALANCED;
            break;

        case SCREEN_MODE_SPEED:
            new_ui_mode = UI_MODE_SPEED_FOCUSED;
            break;

        case SCREEN_MODE_STATUS:
            new_ui_mode = UI_MODE_INFORMATION;
            break;

        case SCREEN_MODE_MIXED:
            new_ui_mode = UI_MODE_BALANCED;
            break;

        case SCREEN_MODE_EXPERIMENTAL:
            new_ui_mode = UI_MODE_EXPERIMENTAL;
            break;

        default:
            LOG_ERR("Unknown screen mode: %d", screen_controller.current_screen_mode);
            return -1;
    }

    if (screen_controller.current_ui_mode == new_ui_mode) {
        return 0; // Already in this UI mode
    }

    LOG_DBG("Updating UI mode: %d", new_ui_mode);

    int ret = circular_ui_switch_mode(new_ui_mode);
    if (ret != 0) {
        LOG_ERR("Failed to switch UI mode");
        return ret;
    }

    screen_controller.current_ui_mode = new_ui_mode;
    return 0;
}

int circular_status_screen_set_auto_switching(bool enabled) {
    LOG_INF("Setting auto mode switching: %s", enabled ? "enabled" : "disabled");
    screen_controller.auto_mode_switching = enabled;
    return 0;
}

int circular_status_screen_set_switch_interval(uint16_t interval_s) {
    if (interval_s < CIRCULAR_SCREEN_MIN_INTERVAL ||
        interval_s > CIRCULAR_SCREEN_MAX_INTERVAL) {
        LOG_ERR("Invalid switch interval: %d (min: %d, max: %d)",
                interval_s, CIRCULAR_SCREEN_MIN_INTERVAL,
                CIRCULAR_SCREEN_MAX_INTERVAL);
        return -1;
    }

    LOG_INF("Setting auto-switch interval: %d seconds", interval_s);
    screen_controller.auto_switch_interval_s = interval_s;
    return 0;
}

// Display integration
lv_obj_t* circular_status_screen_get_lvgl_screen(void) {
    return screen_controller.main_screen;
}

lv_obj_t* circular_status_screen_get_canvas(void) {
    return screen_controller.main_canvas;
}

int circular_status_screen_refresh(void) {
    if (!screen_controller.screen_active) {
        return 0;
    }

    // Check idle timeout
    check_idle_timeout();

    // Auto mode switching
    if (screen_controller.auto_mode_switching) {
        int ret = update_screen_mode_based_on_activity();
        if (ret != 0) {
            LOG_WRN("Failed to update screen mode");
        }
    }

    // Update performance score
    update_performance_score();

    // Force circular UI refresh
    return circular_ui_force_refresh();
}

int circular_status_screen_force_refresh(void) {
    if (!screen_controller.screen_active) {
        return -1;
    }

    int ret = circular_ui_force_refresh();
    if (ret != 0) {
        LOG_ERR("Failed to force circular UI refresh");
        return ret;
    }

    // Invalidate LVGL display
    lv_obj_invalidate(screen_controller.main_canvas);

    return 0;
}

// Configuration
int circular_status_screen_set_idle_timeout(uint8_t timeout_s) {
    if (timeout_s < CIRCULAR_SCREEN_MIN_TIMEOUT ||
        timeout_s > CIRCULAR_SCREEN_MAX_TIMEOUT) {
        LOG_ERR("Invalid idle timeout: %d", timeout_s);
        return -1;
    }

    LOG_INF("Setting idle timeout: %d seconds", timeout_s);
    screen_controller.idle_timeout_s = timeout_s;
    return 0;
}

int circular_status_screen_set_low_power_mode(bool enabled) {
    LOG_INF("Setting low power mode: %s", enabled ? "enabled" : "disabled");
    screen_controller.low_power_mode = enabled;

    if (enabled) {
        // Reduce refresh rate for power saving
        circular_ui_set_refresh_interval(200); // 200ms instead of 100ms
    } else {
        // Restore normal refresh rate
        circular_ui_set_refresh_interval(CIRCULAR_UI_REFRESH_DEFAULT);
    }

    return 0;
}

int circular_status_screen_set_experimental_features(bool enabled) {
    LOG_INF("Setting experimental features: %s", enabled ? "enabled" : "disabled");
    screen_controller.experimental_features = enabled;

    return circular_ui_set_experimental_features(enabled);
}

// Performance monitoring
void circular_status_screen_start_monitoring(void) {
    screen_controller.event_count = 0;
    screen_controller.last_performance_check = k_uptime_get_32();
    screen_controller.performance_score = 100;

    LOG_DBG("Started performance monitoring");
}

void circular_status_screen_stop_monitoring(void) {
    LOG_DBG("Stopped performance monitoring");
}

int circular_status_screen_get_performance(uint8_t *performance_score,
                                          uint32_t *event_count) {
    if (performance_score != NULL) {
        *performance_score = screen_controller.performance_score;
    }

    if (event_count != NULL) {
        *event_count = screen_controller.event_count;
    }

    return 0;
}

bool circular_status_screen_is_user_active(void) {
    return screen_controller.user_active;
}

uint32_t circular_status_screen_get_idle_time(void) {
    uint32_t current_time = k_uptime_get_32();
    return current_time - screen_controller.last_activity_time;
}

// Health and diagnostics
int circular_status_screen_health_check(void) {
    int issues = 0;

    if (!screen_controller.initialized) {
        LOG_ERR("Screen controller not initialized");
        issues++;
    }

    if (screen_controller.screen_active && screen_controller.main_canvas == NULL) {
        LOG_ERR("Screen active but canvas NULL");
        issues++;
    }

    if (screen_controller.screen_active && !circular_ui_is_active()) {
        LOG_ERR("Screen active but circular UI not active");
        issues++;
    }

    // Check circular UI health
    int ui_health = circular_ui_health_check();
    if (ui_health != 0) {
        LOG_ERR("Circular UI health issues: %d", ui_health);
        issues++;
    }

    return (issues == 0) ? 0 : -1;
}

// Utility functions
bool circular_status_screen_is_initialized(void) {
    return screen_controller.initialized;
}

bool circular_status_screen_is_active(void) {
    return screen_controller.screen_active;
}

bool circular_status_screen_is_valid_mode(enum circular_screen_mode mode) {
    return (mode >= SCREEN_MODE_AUTO && mode <= SCREEN_MODE_EXPERIMENTAL);
}

const char* circular_status_screen_get_mode_description(enum circular_screen_mode mode) {
    switch (mode) {
        case SCREEN_MODE_AUTO: return "Auto";
        case SCREEN_MODE_SPEED: return "Speed";
        case SCREEN_MODE_STATUS: return "Status";
        case SCREEN_MODE_MIXED: return "Mixed";
        case SCREEN_MODE_EXPERIMENTAL: return "Experimental";
        default: return "Unknown";
    }
}

void circular_status_screen_update_activity(bool force_activity) {
    uint32_t current_time = k_uptime_get_32();

    if (force_activity || (current_time - screen_controller.last_activity_time) > 1000) {
        screen_controller.last_activity_time = current_time;
        screen_controller.user_active = true;
    }
}

// Internal functions
static int update_screen_mode_based_on_activity(void) {
    uint32_t current_time = k_uptime_get_32();
    uint32_t time_since_last_switch = current_time - screen_controller.last_mode_switch_time;

    if (time_since_last_switch < (screen_controller.auto_switch_interval_s * 1000)) {
        return 0; // Not time to switch yet
    }

    // Determine new mode based on activity
    enum circular_screen_mode new_mode = screen_controller.current_screen_mode;
    uint32_t idle_time = circular_status_screen_get_idle_time();

    if (idle_time > (screen_controller.idle_timeout_s * 1000)) {
        // User is idle, switch to status mode
        new_mode = SCREEN_MODE_STATUS;
    } else if (screen_controller.last_wpm > 80) {
        // High typing speed, switch to speed mode
        new_mode = SCREEN_MODE_SPEED;
    } else {
        // Normal activity, switch to balanced mode
        new_mode = SCREEN_MODE_BALANCED;
    }

    if (new_mode != screen_controller.current_screen_mode) {
        return circular_status_screen_set_mode(new_mode);
    }

    return 0;
}

static void check_idle_timeout(void) {
    uint32_t idle_time = circular_status_screen_get_idle_time();

    if (idle_time > (screen_controller.idle_timeout_s * 1000)) {
        if (!screen_controller.low_power_mode) {
            LOG_INF("User idle, enabling low power mode");
            circular_status_screen_set_low_power_mode(true);
            screen_controller.user_active = false;
        }
    } else {
        if (screen_controller.low_power_mode) {
            LOG_INF("User active, disabling low power mode");
            circular_status_screen_set_low_power_mode(false);
            screen_controller.user_active = true;
        }
    }
}

static void update_performance_score(void) {
    uint32_t current_time = k_uptime_get_32();

    if ((current_time - screen_controller.last_performance_check) < 5000) {
        return; // Only update every 5 seconds
    }

    // Get circular UI performance metrics
    uint8_t ui_fps;
    uint32_t ui_frame_time, ui_max_frame_time;
    int ret = circular_ui_get_performance_metrics(&ui_fps, &ui_frame_time, &ui_max_frame_time);

    if (ret == 0) {
        // Calculate performance score (0-100)
        if (ui_fps >= 15 && ui_frame_time <= 100) {
            screen_controller.performance_score = 100;
        } else if (ui_fps >= 10 && ui_frame_time <= 150) {
            screen_controller.performance_score = 80;
        } else if (ui_fps >= 5 && ui_frame_time <= 200) {
            screen_controller.performance_score = 60;
        } else {
            screen_controller.performance_score = 40;
        }

        LOG_DBG("Performance score: %d (FPS: %d, Frame time: %dms)",
                screen_controller.performance_score, ui_fps, ui_frame_time);
    }

    screen_controller.last_performance_check = current_time;
}