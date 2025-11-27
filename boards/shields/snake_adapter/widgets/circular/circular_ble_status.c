/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include "circular_ble_status.h"
#include "../helpers/display.h"
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <string.h>
#include <stdio.h>
#include <zmk/ble.h>

LOG_MODULE_DECLARE(circular_ble, CONFIG_ZMK_LOG_LEVEL);

// Global state
static struct ble_display_state ble_state = {
    .active_device_count = 0,
    .display_active = false,
    .show_signal_strength = true,
    .show_animations = true,
    .ring_ctx = NULL
};

// Active animations
static struct ble_animation active_animations[BLE_MAX_DEVICES];

// Forward declarations
static void ble_animation_timer_cb(lv_timer_t *timer);
static struct ble_animation* find_free_animation_slot(void);
static struct ble_animation* find_animation_for_device(uint8_t device_id);
static void draw_ble_icon(lv_draw_buf_t *draw_buf, lv_point_t center,
                            lv_color_t color, enum ble_connection_state state);

int circular_ble_init(void) {
    LOG_INF("Initializing circular BLE status display");

    // Clear state
    memset(&ble_state, 0, sizeof(ble_state));
    memset(active_animations, 0, sizeof(active_animations));

    // Initialize devices to default state
    for (uint8_t i = 0; i < BLE_MAX_DEVICES; i++) {
        ble_state.devices[i].device_id = i;
        ble_state.devices[i].is_active = false;
        ble_state.devices[i].state = BLE_STATE_DISCONNECTED;
        ble_state.devices[i].signal_strength = 0;
        ble_state.devices[i].last_state_change = k_uptime_get_32();
        ble_state.devices[i].is_host_connection = false;
        ble_state.devices[i].profile_type = 0; // HID default
        snprintf(ble_state.devices[i].device_name,
                sizeof(ble_state.devices[i].device_name), "BLE %d", i);
    }

    LOG_DBG("BLE display initialized successfully");
    return 0;
}

int circular_ble_start(void) {
    LOG_INF("Starting circular BLE status display");

    ble_state.display_active = true;

    // Initialize ring context
    lv_obj_t *canvas = get_display_canvas();
    if (canvas != NULL) {
        struct circular_position center_pos = {
            .angle_deg = 0,
            .radius = 0  // Use display center
        };

        ble_state.ring_ctx = ring_init(canvas, &center_pos);
        if (ble_state.ring_ctx == NULL) {
            LOG_ERR("Failed to initialize ring context");
            return -1;
        }

        return circular_ble_render_all(canvas);
    }

    return 0;
}

int circular_ble_stop(void) {
    LOG_INF("Stopping circular BLE status display");

    ble_state.display_active = false;
    circular_ble_stop_all_animations();

    if (ble_state.ring_ctx != NULL) {
        ring_cleanup(ble_state.ring_ctx);
        ble_state.ring_ctx = NULL;
    }

    return 0;
}

int circular_ble_update_device(uint8_t device_id,
                              enum ble_connection_state state,
                              uint8_t signal_strength,
                              const char *device_name) {
    if (!circular_ble_is_valid_device_id(device_id)) {
        LOG_ERR("Invalid device ID: %d", device_id);
        return -1;
    }

    if (!circular_ble_is_valid_signal_strength(signal_strength)) {
        LOG_ERR("Invalid signal strength: %d", signal_strength);
        return -1;
    }

    struct ble_device *device = &ble_state.devices[device_id];

    LOG_DBG("Updating BLE device %d: state=%d, signal=%d%%",
            device_id, state, signal_strength);

    // Update device state
    enum ble_connection_state old_state = device->state;
    device->state = state;
    device->signal_strength = signal_strength;
    device->last_state_change = k_uptime_get_32();

    if (device_name != NULL) {
        strncpy(device->device_name, device_name, sizeof(device->device_name) - 1);
        device->device_name[sizeof(device->device_name) - 1] = '\0';
    }

    // Mark as active if not already
    if (!device->is_active) {
        device->is_active = true;
        ble_state.active_device_count++;
    }

    // Start animation if state changed and animations enabled
    if (old_state != state && ble_state.show_animations && ble_state.display_active) {
        if (circular_ble_is_animated_state(state)) {
            return circular_ble_start_animation(device_id, 3); // Medium animation speed
        }
    }

    // Render immediately if display is active
    if (ble_state.display_active) {
        lv_obj_t *canvas = get_display_canvas();
        if (canvas != NULL) {
            return circular_ble_render_device(canvas, device);
        }
    }

    return 0;
}

int circular_ble_render_all(lv_obj_t *canvas) {
    if (canvas == NULL || !ble_state.display_active) {
        LOG_ERR("Invalid canvas or display not active");
        return -1;
    }

    int ret = 0;

    // Render each active device
    for (uint8_t i = 0; i < BLE_MAX_DEVICES; i++) {
        struct ble_device *device = &ble_state.devices[i];
        if (device->is_active) {
            int device_ret = circular_ble_render_device(canvas, device);
            if (device_ret != 0) {
                LOG_WRN("Failed to render BLE device %d", i);
                ret = device_ret; // Continue rendering other devices
            }
        }
    }

    return ret;
}

int circular_ble_render_device(lv_obj_t *canvas, const struct ble_device *device) {
    if (canvas == NULL || device == NULL) {
        LOG_ERR("NULL parameters");
        return -1;
    }

    if (!device->is_active) {
        return 0; // Don't render inactive devices
    }

    int ret = 0;

    // Render connection arc
    ret = circular_ble_render_arc(canvas, device);
    if (ret != 0) {
        LOG_ERR("Failed to render BLE arc for device %d", device->device_id);
        return ret;
    }

    // Render signal strength if enabled
    if (ble_state.show_signal_strength &&
        (device->state == BLE_STATE_CONNECTED || device->signal_strength > 0)) {
        ret = circular_ble_render_signal_strength(canvas, device);
        if (ret != 0) {
            LOG_WRN("Failed to render signal strength for device %d", device->device_id);
            // Don't return error for signal strength failure
        }
    }

    // Render status icon
    ret = circular_ble_render_status_icon(canvas, device);
    if (ret != 0) {
        LOG_WRN("Failed to render status icon for device %d", device->device_id);
        // Don't return error for icon failure
    }

    return 0;
}

int circular_ble_render_arc(lv_obj_t *canvas, const struct ble_device *device) {
    if (canvas == NULL || device == NULL) {
        LOG_ERR("NULL parameters");
        return -1;
    }

    // Get arc for this device
    struct circular_arc device_arc = circular_ble_get_device_arc(device->device_id);

    // Create ring context
    struct circular_position center_pos = {
        .angle_deg = 0,
        .radius = 0  // Use display center
    };

    struct ring_context *ring_ctx = ring_init(canvas, &center_pos);
    if (ring_ctx == NULL) {
        LOG_ERR("Failed to initialize ring context for device %d", device->device_id);
        return -1;
    }

    // Get animation phase if this device is animating
    uint8_t animation_phase = 0;
    struct ble_animation *anim = find_animation_for_device(device->device_id);
    if (anim != NULL) {
        animation_phase = anim->current_phase;
    }

    // Get color for current state
    lv_color_t arc_color = circular_ble_get_state_color(device->state, animation_phase);

    // Create arc style
    struct ring_style arc_style = {
        .color = arc_color,
        .thickness = 5,
        .tick_count = 0,
        .filled = false
    };

    return ring_draw_arc(ring_ctx, &device_arc, &arc_style);
}

int circular_ble_render_signal_strength(lv_obj_t *canvas, const struct ble_device *device) {
    if (canvas == NULL || device == NULL) {
        LOG_ERR("NULL parameters");
        return -1;
    }

    // Calculate signal strength position
    struct circular_arc device_arc = circular_ble_get_device_arc(device->device_id);
    uint16_t signal_angle = (device_arc.start_angle_deg + device_arc.end_angle_deg) / 2;

    struct circular_position signal_pos = {
        .angle_deg = signal_angle,
        .radius = 115  // Just outside the arc
    };

    struct cartesian_position signal_cart = polar_to_cartesian(&signal_pos);

    // Get signal strength color
    lv_color_t signal_color = circular_ble_get_signal_color(device->signal_strength);

    // Draw signal strength as small bar graph (3 bars)
    uint8_t bar_count = 3;
    uint8_t active_bars = (device->signal_strength * bar_count) / 100;
    if (active_bars > bar_count) active_bars = bar_count;
    if (active_bars == 0 && device->signal_strength > 0) active_bars = 1; // At least one bar if connected

    for (uint8_t i = 0; i < bar_count; i++) {
        bool is_active = (i < active_bars);
        lv_color_t bar_color = is_active ? signal_color : lv_color_hex(0x333333);

        int16_t bar_x = signal_cart.x + (i - 1) * 4;  // -4, 0, +4
        int16_t bar_y = signal_cart.y;
        int16_t bar_height = is_active ? 8 : 4;

        lv_draw_rect(canvas->draw_buf,
                    bar_x - 1, bar_y - bar_height,
                    bar_x + 1, bar_y,
                    bar_color);
    }

    return 0;
}

int circular_ble_render_status_icon(lv_obj_t *canvas, const struct ble_device *device) {
    if (canvas == NULL || device == NULL) {
        LOG_ERR("NULL parameters");
        return -1;
    }

    // Calculate icon position
    struct circular_arc device_arc = circular_ble_get_device_arc(device->device_id);
    uint16_t icon_angle = (device_arc.start_angle_deg + device_arc.end_angle_deg) / 2;

    struct circular_position icon_pos = {
        .angle_deg = icon_angle,
        .radius = 70  // Inside the arc
    };

    struct cartesian_position icon_cart = polar_to_cartesian(&icon_pos);

    // Get animation phase
    uint8_t animation_phase = 0;
    struct ble_animation *anim = find_animation_for_device(device->device_id);
    if (anim != NULL) {
        animation_phase = anim->current_phase;
    }

    // Get icon color
    lv_color_t icon_color = circular_ble_get_state_color(device->state, animation_phase);

    // Draw appropriate icon
    draw_ble_icon(canvas->draw_buf, icon_cart, icon_color, device->state);

    return 0;
}

lv_color_t circular_ble_get_state_color(enum ble_connection_state state, uint8_t animation_phase) {
    switch (state) {
        case BLE_STATE_DISCONNECTED:
            return lv_color_hex(0xFF0000); // Red
        case BLE_STATE_CONNECTING:
            // Orange that pulses
            return lv_color_hex(0xFF0000 + (animation_phase * 0x0100));
        case BLE_STATE_CONNECTED:
            return lv_color_hex(0x00FF00); // Green
        case BLE_STATE_PAIRING:
            // Blue that pulses
            return lv_color_hex(0x0000FF + (animation_phase * 0x000001));
        case BLE_STATE_ERROR:
            return lv_color_hex(0xFF00FF); // Magenta
        default:
            return lv_color_hex(0x666666); // Gray
    }
}

bool circular_ble_is_animated_state(enum ble_connection_state state) {
    return (state == BLE_STATE_CONNECTING || state == BLE_STATE_PAIRING);
}

const char* circular_ble_get_state_description(enum ble_connection_state state) {
    switch (state) {
        case BLE_STATE_DISCONNECTED: return "Disconnected";
        case BLE_STATE_CONNECTING: return "Connecting";
        case BLE_STATE_CONNECTED: return "Connected";
        case BLE_STATE_PAIRING: return "Pairing";
        case BLE_STATE_ERROR: return "Error";
        default: return "Unknown";
    }
}

lv_color_t circular_ble_get_signal_color(uint8_t signal_strength) {
    if (signal_strength < 20) {
        return lv_color_hex(0xFF0000); // Red - poor signal
    } else if (signal_strength < 50) {
        return lv_color_hex(0xFF8000); // Orange - weak signal
    } else if (signal_strength < 80) {
        return lv_color_hex(0xFFFF00); // Yellow - fair signal
    } else {
        return lv_color_hex(0x00FF00); // Green - strong signal
    }
}

struct circular_arc circular_ble_get_device_arc(uint8_t device_id) {
    struct circular_arc arc;

    if (!circular_ble_is_valid_device_id(device_id)) {
        // Return empty arc for invalid device
        arc.start_angle_deg = 0;
        arc.end_angle_deg = 0;
        arc.inner_radius = 0;
        arc.outer_radius = 0;
        return arc;
    }

    // Each device gets equal arc space
    arc.start_angle_deg = device_id * BLE_ARC_SPAN;
    arc.end_angle_deg = arc.start_angle_deg + BLE_ARC_SPAN - 1;
    arc.inner_radius = BLE_ARC_INNER_RADIUS;
    arc.outer_radius = BLE_ARC_OUTER_RADIUS;

    return arc;
}

// Animation functions
int circular_ble_start_animation(uint8_t device_id, uint8_t animation_speed) {
    if (!circular_ble_is_valid_device_id(device_id)) {
        return -1;
    }

    struct ble_device *device = &ble_state.devices[device_id];
    if (!circular_ble_is_animated_state(device->state)) {
        return 0; // No animation needed for this state
    }

    // Find existing animation for this device
    struct ble_animation *anim = find_animation_for_device(device_id);

    if (anim == NULL) {
        // Find free animation slot
        anim = find_free_animation_slot();
        if (anim == NULL) {
            LOG_ERR("No free BLE animation slots available");
            return -1;
        }

        // Initialize new animation
        anim->device = device;
    }

    // Stop existing animation
    if (anim->animation_timer != NULL) {
        lv_timer_del(anim->animation_timer);
        anim->animation_timer = NULL;
    }

    // Setup animation parameters
    anim->current_phase = 0;
    anim->animation_speed = animation_speed;

    LOG_DBG("Starting BLE animation for device %d with speed %d", device_id, animation_speed);

    // Create animation timer
    anim->animation_timer = lv_timer_create(ble_animation_timer_cb, 50, anim);
    if (anim->animation_timer == NULL) {
        LOG_ERR("Failed to create BLE animation timer");
        return -1;
    }

    return 0;
}

void circular_ble_stop_animation(uint8_t device_id) {
    struct ble_animation *anim = find_animation_for_device(device_id);
    if (anim != NULL && anim->animation_timer != NULL) {
        lv_timer_del(anim->animation_timer);
        anim->animation_timer = NULL;
        memset(anim, 0, sizeof(*anim));
    }
}

void circular_ble_stop_all_animations(void) {
    for (uint8_t i = 0; i < BLE_MAX_DEVICES; i++) {
        struct ble_animation *anim = &active_animations[i];
        if (anim->animation_timer != NULL) {
            lv_timer_del(anim->animation_timer);
            anim->animation_timer = NULL;
        }
        memset(anim, 0, sizeof(*anim));
    }
}

void circular_ble_update_animation_frame(struct ble_animation *anim) {
    if (anim == NULL || anim->device == NULL) {
        return;
    }

    // Update phase
    anim->current_phase += anim->animation_speed;

    // Update display
    if (ble_state.display_active) {
        lv_obj_t *canvas = get_display_canvas();
        if (canvas != NULL) {
            circular_ble_render_device(canvas, anim->device);
        }
    }
}

// Utility functions
bool circular_ble_is_valid_device_id(uint8_t device_id) {
    return device_id < BLE_MAX_DEVICES;
}

bool circular_ble_is_valid_signal_strength(uint8_t signal_strength) {
    return signal_strength <= 100;
}

int circular_ble_add_device(uint8_t device_id, const char *device_name,
                            uint8_t profile_type, bool is_host_connection) {
    if (!circular_ble_is_valid_device_id(device_id)) {
        return -1;
    }

    struct ble_device *device = &ble_state.devices[device_id];

    device->device_id = device_id;
    device->is_active = true;
    device->state = BLE_STATE_DISCONNECTED;
    device->signal_strength = 0;
    device->profile_type = profile_type;
    device->is_host_connection = is_host_connection;
    device->last_state_change = k_uptime_get_32();

    if (device_name != NULL) {
        strncpy(device->device_name, device_name, sizeof(device->device_name) - 1);
        device->device_name[sizeof(device->device_name) - 1] = '\0';
    }

    ble_state.active_device_count++;

    LOG_INF("Added BLE device %d: %s (host: %s)", device_id,
             device_name ?: "Unknown", is_host_connection ? "yes" : "no");
    return 0;
}

int circular_ble_remove_device(uint8_t device_id) {
    if (!circular_ble_is_valid_device_id(device_id)) {
        return -1;
    }

    struct ble_device *device = &ble_state.devices[device_id];
    circular_ble_stop_animation(device_id);

    device->is_active = false;
    device->state = BLE_STATE_DISCONNECTED;
    device->signal_strength = 0;

    // Recalculate active device count
    ble_state.active_device_count = 0;
    for (uint8_t i = 0; i < BLE_MAX_DEVICES; i++) {
        if (ble_state.devices[i].is_active) {
            ble_state.active_device_count++;
        }
    }

    LOG_INF("Removed BLE device %d", device_id);
    return 0;
}

const char* circular_ble_get_profile_description(uint8_t profile_type) {
    switch (profile_type) {
        case 0: return "HID";
        case 1: return "Audio";
        case 2: return "Serial";
        case 3: return "Health";
        default: return "Unknown";
    }
}

// Internal helper functions
static void ble_animation_timer_cb(lv_timer_t *timer) {
    struct ble_animation *anim = (struct ble_animation *)timer->user_data;
    if (anim == NULL || anim->device == NULL) {
        return;
    }

    circular_ble_update_animation_frame(anim);
}

static struct ble_animation* find_free_animation_slot(void) {
    for (uint8_t i = 0; i < BLE_MAX_DEVICES; i++) {
        if (active_animations[i].animation_timer == NULL) {
            return &active_animations[i];
        }
    }
    return NULL;
}

static struct ble_animation* find_animation_for_device(uint8_t device_id) {
    for (uint8_t i = 0; i < BLE_MAX_DEVICES; i++) {
        if (active_animations[i].device != NULL &&
            active_animations[i].device->device_id == device_id) {
            return &active_animations[i];
        }
    }
    return NULL;
}

static void draw_ble_icon(lv_draw_buf_t *draw_buf, lv_point_t center,
                            lv_color_t color, enum ble_connection_state state) {
    // Draw simplified Bluetooth logo (circle with diagonal line)
    uint8_t icon_size = 8;

    // Draw outer circle
    lv_draw_circle(draw_buf, center, icon_size / 2, color);

    // Draw diagonal line for Bluetooth symbol
    lv_point_t start = {center.x - icon_size/3, center.y - icon_size/3};
    lv_point_t end = {center.x + icon_size/3, center.y + icon_size/3};
    lv_draw_line(draw_buf, &start, &end, color);

    // Draw second diagonal line
    start.x = center.x - icon_size/3;
    start.y = center.y + icon_size/3;
    end.x = center.x + icon_size/3;
    end.y = center.y - icon_size/3;
    lv_draw_line(draw_buf, &start, &end, color);

    // Draw center dot for connecting/pairing states
    if (state == BLE_STATE_CONNECTING || state == BLE_STATE_PAIRING) {
        lv_point_t dot_pos = center;
        lv_draw_circle(draw_buf, dot_pos, 1, lv_color_hex(0xFFFFFF));
    }
}

void circular_ble_cleanup(void) {
    circular_ble_stop_all_animations();
    memset(&ble_state, 0, sizeof(ble_state));
}

int circular_ble_health_check(void) {
    int issues = 0;

    for (uint8_t i = 0; i < BLE_MAX_DEVICES; i++) {
        struct ble_device *device = &ble_state.devices[i];
        if (device->is_active) {
            if (!circular_ble_is_valid_signal_strength(device->signal_strength)) {
                LOG_ERR("Device %d has invalid signal strength: %d", i, device->signal_strength);
                issues++;
            }
        }
    }

    if (ble_state.ring_ctx == NULL && ble_state.display_active) {
        LOG_ERR("Ring context NULL but display active");
        issues++;
    }

    return (issues == 0) ? 0 : -1;
}

void circular_ble_reset(void) {
    circular_ble_stop_all_animations();
    memset(&ble_state.devices, 0, sizeof(ble_state.devices));
    ble_state.active_device_count = 0;
}