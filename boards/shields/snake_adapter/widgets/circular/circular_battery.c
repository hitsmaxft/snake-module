/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include "circular_battery.h"
#include "../helpers/display.h"
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <string.h>
#include <stdio.h>

LOG_MODULE_DECLARE(circular_battery, CONFIG_ZMK_LOG_LEVEL);

// Global state
static struct battery_display_state battery_state = {
    .device_count = 0,
    .display_active = false,
    .show_percentage = true,
    .show_animation = true,
    .ring_ctx = NULL
};

// Active animations (support multiple)
static struct battery_animation active_animations[BATTERY_MAX_DEVICES];

// Forward declarations
static void battery_animation_timer_cb(lv_timer_t *timer);
static struct battery_animation* find_free_animation_slot(void);
static struct battery_animation* find_animation_for_device(uint8_t device_id);

int circular_battery_init(void) {
    LOG_INF("Initializing circular battery display");

    // Clear state
    memset(&battery_state, 0, sizeof(battery_state));
    memset(active_animations, 0, sizeof(active_animations));

    // Initialize devices to default state
    for (uint8_t i = 0; i < BATTERY_MAX_DEVICES; i++) {
        battery_state.devices[i].device_id = i;
        battery_state.devices[i].battery_percentage = 0;
        battery_state.devices[i].is_charging = false;
        battery_state.devices[i].is_connected = false;
        battery_state.devices[i].quadrant = (enum circular_quadrant)(i % 4);
        battery_state.devices[i].last_update_time = k_uptime_get_32();
        snprintf(battery_state.devices[i].device_name,
                sizeof(battery_state.devices[i].device_name), "Device %d", i);
    }

    LOG_DBG("Battery display initialized successfully");
    return 0;
}

int circular_battery_start(void) {
    LOG_INF("Starting circular battery display");

    battery_state.display_active = true;

    // Initialize ring context if needed
    lv_obj_t *canvas = get_display_canvas();
    if (canvas != NULL) {
        struct circular_position center_pos = {
            .angle_deg = 0,
            .radius = 0  // Use display center
        };

        battery_state.ring_ctx = ring_init(canvas, &center_pos);
        if (battery_state.ring_ctx == NULL) {
            LOG_ERR("Failed to initialize ring context");
            return -1;
        }

        return circular_battery_render_all(canvas);
    }

    return 0;
}

int circular_battery_stop(void) {
    LOG_INF("Stopping circular battery display");

    battery_state.display_active = false;
    circular_battery_stop_all_animations();

    if (battery_state.ring_ctx != NULL) {
        ring_cleanup(battery_state.ring_ctx);
        battery_state.ring_ctx = NULL;
    }

    return 0;
}

int circular_battery_update_device(uint8_t device_id,
                                   uint8_t battery_percentage,
                                   bool is_charging,
                                   bool is_connected,
                                   const char *device_name) {
    if (!circular_battery_is_valid_device_id(device_id)) {
        LOG_ERR("Invalid device ID: %d", device_id);
        return -1;
    }

    if (!circular_battery_is_valid_percentage(battery_percentage)) {
        LOG_ERR("Invalid battery percentage: %d", battery_percentage);
        return -1;
    }

    struct battery_device *device = &battery_state.devices[device_id];

    LOG_DBG("Updating device %d: %d%% (charging: %s, connected: %s)",
            device_id, battery_percentage, is_charging ? "yes" : "no",
            is_connected ? "yes" : "no");

    // Update device state
    uint8_t old_percentage = device->battery_percentage;
    device->battery_percentage = battery_percentage;
    device->is_charging = is_charging;
    device->is_connected = is_connected;
    device->last_update_time = k_uptime_get_32();

    if (device_name != NULL) {
        strncpy(device->device_name, device_name, sizeof(device->device_name) - 1);
        device->device_name[sizeof(device->device_name) - 1] = '\0';
    }

    // Update device count
    if (is_connected && device_id >= battery_state.device_count) {
        battery_state.device_count = device_id + 1;
    }

    // Animate level change if enabled
    if (battery_state.show_animation && battery_state.display_active &&
        old_percentage != battery_percentage) {
        return circular_battery_animate_level_change(device_id, battery_percentage, 1000);
    }

    // Render immediately if display is active
    if (battery_state.display_active) {
        lv_obj_t *canvas = get_display_canvas();
        if (canvas != NULL) {
            return circular_battery_render_device(canvas, device);
        }
    }

    return 0;
}

int circular_battery_render_all(lv_obj_t *canvas) {
    if (canvas == NULL || !battery_state.display_active) {
        LOG_ERR("Invalid canvas or display not active");
        return -1;
    }

    int ret = 0;

    // Render each connected device
    for (uint8_t i = 0; i < battery_state.device_count; i++) {
        struct battery_device *device = &battery_state.devices[i];
        if (device->is_connected) {
            int device_ret = circular_battery_render_device(canvas, device);
            if (device_ret != 0) {
                LOG_WRN("Failed to render device %d", i);
                ret = device_ret;  // Continue rendering other devices
            }
        }
    }

    return ret;
}

int circular_battery_render_device(lv_obj_t *canvas, const struct battery_device *device) {
    if (canvas == NULL || device == NULL) {
        LOG_ERR("NULL parameters");
        return -1;
    }

    if (!device->is_connected) {
        return 0;  // Don't render disconnected devices
    }

    int ret = 0;

    // Render battery arc
    ret = circular_battery_render_arc(canvas, device);
    if (ret != 0) {
        LOG_ERR("Failed to render battery arc for device %d", device->device_id);
        return ret;
    }

    // Render percentage text if enabled
    if (battery_state.show_percentage) {
        ret = circular_battery_render_text(canvas, device);
        if (ret != 0) {
            LOG_WRN("Failed to render battery text for device %d", device->device_id);
            // Don't return error for text rendering failure
        }
    }

    // Render charging indicator if charging
    if (device->is_charging) {
        ret = circular_battery_render_charging_indicator(canvas, device);
        if (ret != 0) {
            LOG_WRN("Failed to render charging indicator for device %d", device->device_id);
            // Don't return error for charging indicator failure
        }
    }

    return 0;
}

int circular_battery_render_arc(lv_obj_t *canvas, const struct battery_device *device) {
    if (canvas == NULL || device == NULL) {
        LOG_ERR("NULL parameters");
        return -1;
    }

    // Create ring context for this device's quadrant
    struct circular_position device_center = {
        .angle_deg = 0,
        .radius = 0  // Use display center, position will be calculated
    };

    struct ring_context *ring_ctx = ring_init(canvas, &device_center);
    if (ring_ctx == NULL) {
        LOG_ERR("Failed to initialize ring context for device %d", device->device_id);
        return -1;
    }

    // Draw background arc
    struct ring_style bg_style = {
        .color = lv_color_hex(0x333333),
        .thickness = 6,
        .tick_count = 0,
        .filled = false
    };

    struct circular_arc bg_arc = {
        .start_angle_deg = device->quadrant * 90,
        .end_angle_deg = (device->quadrant * 90) + BATTERY_ARC_SPAN,
        .inner_radius = BATTERY_INNER_RADIUS,
        .outer_radius = BATTERY_OUTER_RADIUS
    };

    int ret = ring_draw_arc(ring_ctx, &bg_arc, &bg_style);
    if (ret != 0) {
        LOG_ERR("Failed to draw battery background for device %d", device->device_id);
        return ret;
    }

    // Draw battery level arc if percentage > 0
    if (device->battery_percentage > 0) {
        struct ring_style level_style = {
            .color = circular_battery_get_status_color(device->battery_percentage,
                                                       device->is_charging),
            .thickness = 6,
            .tick_count = 0,
            .filled = false
        };

        uint16_t fill_span = (device->battery_percentage * BATTERY_ARC_SPAN) / 100;
        struct circular_arc level_arc = {
            .start_angle_deg = device->quadrant * 90,
            .end_angle_deg = (device->quadrant * 90) + fill_span,
            .inner_radius = BATTERY_INNER_RADIUS,
            .outer_radius = BATTERY_OUTER_RADIUS
        };

        ret = ring_draw_arc(ring_ctx, &level_arc, &level_style);
        if (ret != 0) {
            LOG_ERR("Failed to draw battery level for device %d", device->device_id);
            return ret;
        }
    }

    return 0;
}

int circular_battery_render_text(lv_obj_t *canvas, const struct battery_device *device) {
    if (canvas == NULL || device == NULL) {
        LOG_ERR("NULL parameters");
        return -1;
    }

    // Calculate text position in quadrant
    struct cartesian_position text_pos = get_battery_position(device->quadrant, 95, 0);

    char percentage_text[8];
    int ret = circular_battery_format_percentage(device->battery_percentage,
                                                 percentage_text,
                                                 sizeof(percentage_text));
    if (ret != 0) {
        return ret;
    }

    // Draw percentage text
    lv_point_t text_lv_pos = {
        .x = text_pos.x,
        .y = text_pos.y
    };

    lv_draw_text(canvas->draw_buf, percentage_text, &text_lv_pos,
                 lv_color_hex(0xFFFFFF));

    return 0;
}

int circular_battery_render_charging_indicator(lv_obj_t *canvas, const struct battery_device *device) {
    if (canvas == NULL || device == NULL) {
        LOG_ERR("NULL parameters");
        return -1;
    }

    // Calculate charging indicator position
    struct cartesian_position charge_pos = get_battery_position(device->quadrant, 75, 30);

    // Draw small charging bolt symbol (simplified as a small filled rectangle)
    lv_color_t charge_color = lv_color_hex(0xFFFF00);  // Yellow for charging

    lv_draw_rect(canvas->draw_buf,
                 charge_pos.x - 2, charge_pos.y - 4,
                 charge_pos.x + 2, charge_pos.y + 4,
                 charge_color);

    return 0;
}

lv_color_t circular_battery_get_status_color(uint8_t percentage, bool is_charging) {
    if (is_charging) {
        return lv_color_hex(0xFFFF00);  // Yellow for charging
    }

    if (circular_battery_is_critical(percentage)) {
        return lv_color_hex(0xFF0000);  // Red for critical
    } else if (circular_battery_is_low(percentage)) {
        return lv_color_hex(0xFF8000);  // Orange for low
    } else if (percentage < BATTERY_GOOD_PERCENTAGE) {
        return lv_color_hex(0x00FF00);  // Green for good
    } else {
        return lv_color_hex(0x00FF80);  // Light green for full
    }
}

int circular_battery_get_status_text(uint8_t percentage,
                                    bool is_charging,
                                    char *buffer,
                                    size_t buffer_size) {
    if (buffer == NULL || buffer_size < 8) {
        return -1;
    }

    if (is_charging) {
        snprintf(buffer, buffer_size, "CHG");
    } else if (circular_battery_is_critical(percentage)) {
        snprintf(buffer, buffer_size, "LOW");
    } else if (circular_battery_is_low(percentage)) {
        snprintf(buffer, buffer_size, "LOW");
    } else {
        snprintf(buffer, buffer_size, "OK");
    }

    return 0;
}

bool circular_battery_is_critical(uint8_t percentage) {
    return percentage < BATTERY_CRITICAL_PERCENTAGE;
}

bool circular_battery_is_low(uint8_t percentage) {
    return percentage < BATTERY_LOW_PERCENTAGE;
}

enum circular_quadrant circular_battery_get_device_quadrant(uint8_t device_id) {
    return (enum circular_quadrant)(device_id % 4);
}

int circular_battery_animate_level_change(uint8_t device_id,
                                           uint8_t target_percentage,
                                           uint16_t animation_ms) {
    if (!circular_battery_is_valid_device_id(device_id) ||
        !circular_battery_is_valid_percentage(target_percentage)) {
        return -1;
    }

    // Find existing animation for this device
    struct battery_animation *anim = find_animation_for_device(device_id);

    if (anim == NULL) {
        // Find free animation slot
        anim = find_free_animation_slot();
        if (anim == NULL) {
            LOG_ERR("No free animation slots available");
            return -1;
        }

        // Initialize new animation
        anim->device = &battery_state.devices[device_id];
    }

    // Stop existing animation
    if (anim->animation_timer != NULL) {
        lv_timer_del(anim->animation_timer);
        anim->animation_timer = NULL;
    }

    // Setup animation parameters
    anim->target_percentage = target_percentage;
    anim->current_percentage = anim->device->battery_percentage;
    anim->animation_steps = animation_ms / 50;  // 50ms steps
    anim->current_step = 0;

    LOG_DBG("Starting battery animation for device %d: %d%% -> %d%%",
            device_id, anim->current_percentage, target_percentage);

    // Create animation timer
    anim->animation_timer = lv_timer_create(battery_animation_timer_cb, 50, anim);
    if (anim->animation_timer == NULL) {
        LOG_ERR("Failed to create battery animation timer");
        return -1;
    }

    return 0;
}

void circular_battery_stop_all_animations(void) {
    for (uint8_t i = 0; i < BATTERY_MAX_DEVICES; i++) {
        struct battery_animation *anim = &active_animations[i];
        if (anim->animation_timer != NULL) {
            lv_timer_del(anim->animation_timer);
            anim->animation_timer = NULL;
        }
        memset(anim, 0, sizeof(*anim));
    }
}

bool circular_battery_has_active_animations(void) {
    for (uint8_t i = 0; i < BATTERY_MAX_DEVICES; i++) {
        if (active_animations[i].animation_timer != NULL) {
            return true;
        }
    }
    return false;
}

int circular_battery_add_device(uint8_t device_id, const char *device_name) {
    if (!circular_battery_is_valid_device_id(device_id)) {
        return -1;
    }

    struct battery_device *device = &battery_state.devices[device_id];

    device->device_id = device_id;
    device->is_connected = true;
    device->quadrant = circular_battery_get_device_quadrant(device_id);
    device->last_update_time = k_uptime_get_32();

    if (device_name != NULL) {
        strncpy(device->device_name, device_name, sizeof(device->device_name) - 1);
        device->device_name[sizeof(device->device_name) - 1] = '\0';
    }

    if (device_id >= battery_state.device_count) {
        battery_state.device_count = device_id + 1;
    }

    LOG_INF("Added battery device %d: %s", device_id, device_name ?: "Unknown");
    return 0;
}

int circular_battery_remove_device(uint8_t device_id) {
    if (!circular_battery_is_valid_device_id(device_id)) {
        return -1;
    }

    struct battery_device *device = &battery_state.devices[device_id];
    device->is_connected = false;
    device->battery_percentage = 0;
    device->is_charging = false;

    LOG_INF("Removed battery device %d", device_id);
    return 0;
}

// Utility functions
bool circular_battery_is_valid_percentage(uint8_t percentage) {
    return percentage <= BATTERY_MAX_PERCENTAGE;
}

bool circular_battery_is_valid_device_id(uint8_t device_id) {
    return device_id < BATTERY_MAX_DEVICES;
}

int circular_battery_format_percentage(uint8_t percentage,
                                       char *buffer,
                                       size_t buffer_size) {
    if (buffer == NULL || buffer_size < 5) {
        return -1;
    }

    snprintf(buffer, buffer_size, "%d%%", percentage);
    return 0;
}

const char* circular_battery_get_level_description(uint8_t percentage) {
    if (circular_battery_is_critical(percentage)) {
        return "Critical";
    } else if (circular_battery_is_low(percentage)) {
        return "Low";
    } else if (percentage < BATTERY_GOOD_PERCENTAGE) {
        return "Good";
    } else {
        return "Full";
    }
}

// Internal helper functions
static void battery_animation_timer_cb(lv_timer_t *timer) {
    struct battery_animation *anim = (struct battery_animation *)timer->user_data;
    if (anim == NULL || anim->device == NULL) {
        return;
    }

    if (anim->current_step >= anim->animation_steps) {
        // Animation complete
        anim->device->battery_percentage = anim->target_percentage;
        lv_timer_del(anim->animation_timer);
        anim->animation_timer = NULL;
        memset(anim, 0, sizeof(*anim));
        return;
    }

    // Calculate intermediate percentage
    int16_t percentage_diff = (int16_t)anim->target_percentage - (int16_t)anim->current_percentage;
    int16_t step_size = percentage_diff / (anim->animation_steps - anim->current_step);

    anim->device->battery_percentage = (uint8_t)(anim->current_percentage + step_size);
    anim->current_step++;

    // Update display
    if (battery_state.display_active) {
        lv_obj_t *canvas = get_display_canvas();
        if (canvas != NULL) {
            circular_battery_render_device(canvas, anim->device);
        }
    }
}

static struct battery_animation* find_free_animation_slot(void) {
    for (uint8_t i = 0; i < BATTERY_MAX_DEVICES; i++) {
        if (active_animations[i].animation_timer == NULL) {
            return &active_animations[i];
        }
    }
    return NULL;
}

static struct battery_animation* find_animation_for_device(uint8_t device_id) {
    for (uint8_t i = 0; i < BATTERY_MAX_DEVICES; i++) {
        if (active_animations[i].device != NULL &&
            active_animations[i].device->device_id == device_id) {
            return &active_animations[i];
        }
    }
    return NULL;
}

void circular_battery_cleanup(void) {
    circular_battery_stop_all_animations();
    memset(&battery_state, 0, sizeof(battery_state));
}

int circular_battery_health_check(void) {
    int issues = 0;

    for (uint8_t i = 0; i < BATTERY_MAX_DEVICES; i++) {
        struct battery_device *device = &battery_state.devices[i];
        if (device->is_connected && !circular_battery_is_valid_percentage(device->battery_percentage)) {
            LOG_ERR("Device %d has invalid battery percentage: %d", i, device->battery_percentage);
            issues++;
        }
    }

    if (battery_state.ring_ctx == NULL && battery_state.display_active) {
        LOG_ERR("Ring context NULL but display active");
        issues++;
    }

    return (issues == 0) ? 0 : -1;
}

void circular_battery_reset(void) {
    circular_battery_stop_all_animations();
    memset(&battery_state.devices, 0, sizeof(battery_state.devices));
    battery_state.device_count = 0;
}