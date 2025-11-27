/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <lvgl.h>
#include <zephyr/kernel.h>

// Display center coordinates
#define CIRCULAR_CENTER_X 120
#define CIRCULAR_CENTER_Y 120

// Display radius constraints
#define MAX_RADIUS 110
#define SAFE_RADIUS 105

// Circular position structure
struct circular_position {
    uint16_t angle_deg;    // Angle in degrees (0-359)
    uint16_t radius;       // Distance from center (0-MAX_RADIUS)
};

// Cartesian coordinates
struct cartesian_position {
    uint16_t x;
    uint16_t y;
};

// Circular arc structure
struct circular_arc {
    uint16_t start_angle_deg;    // Start angle in degrees
    uint16_t end_angle_deg;      // End angle in degrees
    uint16_t inner_radius;       // Inner radius for ring segments
    uint16_t outer_radius;       // Outer radius for ring segments
};

// Circular quadrant definitions
enum circular_quadrant {
    QUADRANT_TOP_RIGHT = 0,      // 0° to 89°
    QUADRANT_BOTTOM_RIGHT = 1,   // 90° to 179°
    QUADRANT_BOTTOM_LEFT = 2,    // 180° to 269°
    QUADRANT_TOP_LEFT = 3        // 270° to 359°
};

// === Coordinate Transformation Functions ===

/**
 * Convert polar coordinates to Cartesian coordinates
 * @param pos Circular position with angle and radius
 * @return Cartesian position with x,y coordinates
 */
struct cartesian_position polar_to_cartesian(const struct circular_position *pos);

/**
 * Convert Cartesian coordinates to polar coordinates
 * @param pos Cartesian position with x,y coordinates
 * @return Circular position with angle and radius
 */
struct circular_position cartesian_to_polar(const struct cartesian_position *pos);

/**
 * Get quadrant for a given angle
 * @param angle_deg Angle in degrees (0-359)
 * @return Quadrant enum value
 */
enum circular_quadrant get_quadrant(uint16_t angle_deg);

/**
 * Normalize angle to 0-359 degree range
 * @param angle_deg Input angle (can be any integer)
 * @return Normalized angle (0-359)
 */
uint16_t normalize_angle(int16_t angle_deg);

/**
 * Calculate arc angle span
 * @param arc Circular arc structure
 * @return Arc angle span in degrees
 */
uint16_t get_arc_span(const struct circular_arc *arc);

/**
 * Check if angle is within arc bounds
 * @param arc Circular arc structure
 * @param angle_deg Angle to check
 * @return True if angle is within arc, false otherwise
 */
bool is_angle_in_arc(const struct circular_arc *arc, uint16_t angle_deg);

// === Position Calculation Functions ===

/**
 * Get position for battery indicators (quadrant-based)
 * @param quadrant Which quadrant (0-3)
 * @param radius Distance from center
 * @param offset_angle Angular offset within quadrant
 * @return Cartesian position
 */
struct cartesian_position get_battery_position(enum circular_quadrant quadrant,
                                               uint16_t radius,
                                               uint16_t offset_angle);

/**
 * Get position for BLE status arcs (outer ring)
 * @param device_index Device index (0-3 for 4 BLE slots)
 * @return Arc structure for BLE status
 */
struct circular_arc get_ble_arc(uint8_t device_index);

/**
 * Get position for central elements (speed, logos, etc.)
 * @param radius Distance from center (usually 0)
 * @return Center position
 */
struct cartesian_position get_center_position(uint16_t radius);

// === Utility Functions ===

/**
 * Calculate distance between two points
 * @param pos1 First position
 * @param pos2 Second position
 * @return Distance in pixels
 */
uint16_t calculate_distance(const struct cartesian_position *pos1,
                           const struct cartesian_position *pos2);

/**
 * Calculate angle between two points relative to center
 * @param pos1 First position
 * @param pos2 Second position
 * @return Angle in degrees
 */
uint16_t calculate_angle(const struct cartesian_position *pos1,
                        const struct cartesian_position *pos2);

/**
 * Check if position is within display bounds
 * @param pos Position to check
 * @return True if position is valid, false otherwise
 */
bool is_valid_position(const struct cartesian_position *pos);

/**
 * Clamp position to display bounds
 * @param pos Position to clamp
 * @return Clamped position within bounds
 */
struct cartesian_position clamp_position(const struct cartesian_position *pos);

// === Initialization Functions ===

/**
 * Initialize circular layout system
 */
void circular_layout_init(void);

/**
 * Get layout constants for validation
 * @param center_x Pointer to store center X
 * @param center_y Pointer to store center Y
 * @param max_radius Pointer to store maximum radius
 */
void get_layout_constants(uint16_t *center_x, uint16_t *center_y, uint16_t *max_radius);