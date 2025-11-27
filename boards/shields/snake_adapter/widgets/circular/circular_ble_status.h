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

// BLE display configuration
#define BLE_MAX_DEVICES 4
#define BLE_ARC_INNER_RADIUS 85   // Inner radius of BLE status arcs
#define BLE_ARC_OUTER_RADIUS 105  // Outer radius of BLE status arcs
#define BLE_ARC_SPAN 90          // Degrees per BLE device (360 / 4)

// BLE connection states
enum ble_connection_state {
    BLE_STATE_DISCONNECTED = 0,   // Red arc
    BLE_STATE_CONNECTING = 1,     // Orange blinking arc
    BLE_STATE_CONNECTED = 2,     // Green arc
    BLE_STATE_PAIRING = 3,       // Blue animated arc
    BLE_STATE_ERROR = 4          // Red blinking arc
};

// BLE device information
struct ble_device {
    uint8_t device_id;              // Device identifier (0-3)
    bool is_active;                // True if device is configured/active
    enum ble_connection_state state; // Current connection state
    uint8_t signal_strength;        // Signal strength (0-100)
    char device_name[16];          // Device name or profile
    uint32_t last_state_change;    // Timestamp of last state change
    bool is_host_connection;       // True if this is host connection
    uint8_t profile_type;          // HID, Audio, etc.
};

// BLE display state
struct ble_display_state {
    struct ble_device devices[BLE_MAX_DEVICES];
    uint8_t active_device_count;
    bool display_active;
    bool show_signal_strength;
    bool show_animations;
    struct ring_context *ring_ctx;
};

// BLE animation structure
struct ble_animation {
    struct ble_device *device;
    uint8_t current_phase;         // Animation phase (0-255)
    uint8_t animation_speed;        // Animation speed
    lv_timer_t *animation_timer;
};

// === BLE Display Lifecycle ===

/**
 * Initialize circular BLE status display
 * @return 0 on success, error code on failure
 */
int circular_ble_init(void);

/**
 * Start BLE status display
 * @return 0 on success, error code on failure
 */
int circular_ble_start(void);

/**
 * Stop BLE status display
 * @return 0 on success, error code on failure
 */
int circular_ble_stop(void);

/**
 * Update BLE device state
 * @param device_id Device identifier (0-3)
 * @param state New connection state
 * @param signal_strength Signal strength (0-100)
 * @param device_name Optional device name
 * @return 0 on success, error code on failure
 */
int circular_ble_update_device(uint8_t device_id,
                              enum ble_connection_state state,
                              uint8_t signal_strength,
                              const char *device_name);

// === Display Rendering Functions ===

/**
 * Render all BLE status arcs
 * @param canvas LVGL canvas for drawing
 * @return 0 on success, error code on failure
 */
int circular_ble_render_all(lv_obj_t *canvas);

/**
 * Render BLE status for specific device
 * @param canvas LVGL canvas for drawing
 * @param device Device information
 * @return 0 on success, error code on failure
 */
int circular_ble_render_device(lv_obj_t *canvas, const struct ble_device *device);

/**
 * Render BLE connection arc
 * @param canvas LVGL canvas for drawing
 * @param device Device information
 * @return 0 on success, error code on failure
 */
int circular_ble_render_arc(lv_obj_t *canvas, const struct ble_device *device);

/**
 * Render signal strength indicator
 * @param canvas LVGL canvas for drawing
 * @param device Device information
 * @return 0 on success, error code on failure
 */
int circular_ble_render_signal_strength(lv_obj_t *canvas, const struct ble_device *device);

/**
 * Render connection status icon
 * @param canvas LVGL canvas for drawing
 * @param device Device information
 * @return 0 on success, error code on failure
 */
int circular_ble_render_status_icon(lv_obj_t *canvas, const struct ble_device *device);

// === BLE State Management ===

/**
 * Get color for BLE connection state
 * @param state Connection state
 * @param animation_phase Current animation phase (for animated states)
 * @return LVGL color
 */
lv_color_t circular_ble_get_state_color(enum ble_connection_state state,
                                        uint8_t animation_phase);

/**
 * Check if BLE state should be animated
 * @param state Connection state
 * @return True if animated, false otherwise
 */
bool circular_ble_is_animated_state(enum ble_connection_state state);

/**
 * Get BLE state description
 * @param state Connection state
 * @return String description
 */
const char* circular_ble_get_state_description(enum ble_connection_state state);

/**
 * Get signal strength color
 * @param signal_strength Signal strength (0-100)
 * @return LVGL color
 */
lv_color_t circular_ble_get_signal_color(uint8_t signal_strength);

/**
 * Get BLE device arc parameters
 * @param device_id Device identifier
 * @return Circular arc structure
 */
struct circular_arc circular_ble_get_device_arc(uint8_t device_id);

// === Animation Functions ===

/**
 * Start BLE state animation
 * @param device_id Device identifier
 * @param animation_speed Animation speed (1-255)
 * @return 0 on success, error code on failure
 */
int circular_ble_start_animation(uint8_t device_id, uint8_t animation_speed);

/**
 * Stop BLE animation
 * @param device_id Device identifier
 */
void circular_ble_stop_animation(uint8_t device_id);

/**
 * Stop all BLE animations
 */
void circular_ble_stop_all_animations(void);

/**
 * Update animation frame
 * @param anim Animation structure to update
 */
void circular_ble_update_animation_frame(struct ble_animation *anim);

// === Device Management ===

/**
 * Add BLE device
 * @param device_id Device identifier (0-3)
 * @param device_name Device name
 * @param profile_type Profile type (HID, Audio, etc.)
 * @param is_host_connection True if host connection
 * @return 0 on success, error code on failure
 */
int circular_ble_add_device(uint8_t device_id,
                            const char *device_name,
                            uint8_t profile_type,
                            bool is_host_connection);

/**
 * Remove BLE device
 * @param device_id Device identifier
 * @return 0 on success, error code on failure
 */
int circular_ble_remove_device(uint8_t device_id);

/**
 * Get BLE device information
 * @param device_id Device identifier
 * @param device Pointer to store device information
 * @return 0 on success, error code on failure
 */
int circular_ble_get_device(uint8_t device_id, struct ble_device *device);

/**
 * Get number of active BLE devices
 * @return Number of devices
 */
uint8_t circular_ble_get_active_device_count(void);

/**
 * Get current host connection device
 * @return Device identifier, or 0xFF if no host connection
 */
uint8_t circular_ble_get_host_device(void);

/**
 * Check if any BLE devices are connected
 * @return True if connected, false otherwise
 */
bool circular_ble_has_connected_devices(void);

// === Display Configuration ===

/**
 * Set whether to show signal strength
 * @param show_signal_strength True to show, false to hide
 * @return 0 on success, error code on failure
 */
int circular_ble_set_show_signal_strength(bool show_signal_strength);

/**
 * Set whether to show animations
 * @param show_animations True to show, false to hide
 * @return 0 on success, error code on failure
 */
int circular_ble_set_show_animations(bool show_animations);

/**
 * Get current display settings
 * @param show_signal_strength Pointer to store signal strength setting
 * @param show_animations Pointer to store animation setting
 * @return 0 on success, error code on failure
 */
int circular_ble_get_display_settings(bool *show_signal_strength,
                                     bool *show_animations);

/**
 * Configure BLE arc parameters
 * @param inner_radius Inner radius of BLE arcs
 * @param outer_radius Outer radius of BLE arcs
 * @param arc_span Degrees per device arc
 * @return 0 on success, error code on failure
 */
int circular_ble_configure_arcs(uint16_t inner_radius,
                                uint16_t outer_radius,
                                uint16_t arc_span);

// === Event Integration ===

/**
 * Handle ZMK BLE connection state changed event
 * @param profile_id Profile identifier
 * @param connected True if connected, false if disconnected
 * @return 0 on success, error code on failure
 */
int circular_ble_on_connection_changed(uint8_t profile_id, bool connected);

/**
 * Handle BLE pairing request
 * @param profile_id Profile identifier
 * @return 0 on success, error code on failure
 */
int circular_ble_on_pairing_request(uint8_t profile_id);

/**
 * Handle BLE signal strength update
 * @param profile_id Profile identifier
 * @param signal_strength Signal strength (0-100)
 * @return 0 on success, error code on failure
 */
int circular_ble_on_signal_strength_changed(uint8_t profile_id,
                                           uint8_t signal_strength);

// === Utility Functions ===

/**
 * Validate BLE device ID
 * @param device_id Device identifier to validate
 * @return True if valid, false otherwise
 */
bool circular_ble_is_valid_device_id(uint8_t device_id);

/**
 * Validate signal strength
 * @param signal_strength Signal strength to validate
 * @return True if valid, false otherwise
 */
bool circular_ble_is_valid_signal_strength(uint8_t signal_strength);

/**
 * Format device name for display
 * @param device_name Device name to format
 * @param buffer Buffer to store formatted name
 * @param buffer_size Buffer size
 * @return 0 on success, error code on failure
 */
int circular_ble_format_device_name(const char *device_name,
                                    char *buffer,
                                    size_t buffer_size);

/**
 * Get BLE profile type description
 * @param profile_type Profile type
 * @return String description
 */
const char* circular_ble_get_profile_description(uint8_t profile_type);

// === Cleanup and Maintenance ===

/**
 * Cleanup BLE display resources
 */
void circular_ble_cleanup(void);

/**
 * Check BLE display health
 * @return 0 if healthy, error code if issues detected
 */
int circular_ble_health_check(void);

/**
 * Reset BLE display state
 */
void circular_ble_reset(void);