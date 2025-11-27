/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include "circular_layout.h"
#include <zephyr/logging/log.h>
#include <math.h>

LOG_MODULE_DECLARE(circular_layout, CONFIG_ZMK_LOG_LEVEL);

// Fixed-point math constants for embedded efficiency
#define FIXED_POINT_SCALE 1000
#define PI_FIXED 3142  // π * 1000
#define DEG_TO_RAD_FIXED 17  // π/180 * 1000 ≈ 17

// Fixed-point sine and cosine lookup tables for common angles
static const int16_t sin_table[360] = {
    0, 17, 35, 52, 70, 87, 105, 122, 139, 156, 174, 191, 208, 225, 242, 259,
    276, 292, 309, 326, 342, 358, 375, 391, 407, 423, 438, 454, 469, 485,
    500, 515, 530, 545, 559, 574, 588, 602, 616, 629, 643, 656, 669, 682,
    695, 707, 719, 731, 743, 755, 766, 777, 788, 799, 809, 819, 829, 839,
    848, 857, 866, 875, 883, 891, 899, 906, 914, 921, 927, 934, 940, 946,
    951, 957, 962, 967, 971, 975, 979, 983, 987, 990, 993, 995, 998, 1000,
    1000, 1000, 998, 995, 993, 990, 987, 983, 979, 975, 971, 967, 962, 957,
    951, 946, 940, 934, 927, 921, 914, 906, 899, 891, 883, 875, 866, 857,
    848, 839, 829, 819, 809, 799, 788, 777, 766, 755, 743, 731, 719, 707,
    695, 682, 669, 656, 643, 629, 616, 602, 588, 574, 559, 545, 530, 515,
    500, 485, 469, 454, 438, 423, 407, 391, 375, 358, 342, 326, 309, 292,
    276, 259, 242, 225, 208, 191, 174, 156, 139, 122, 105, 87, 70, 52, 35,
    17, 0, -17, -35, -52, -70, -87, -105, -122, -139, -156, -174, -191,
    -208, -225, -242, -259, -276, -292, -309, -326, -342, -358, -375, -391,
    -407, -423, -438, -454, -469, -485, -500, -515, -530, -545, -559, -574,
    -588, -602, -616, -629, -643, -656, -669, -682, -695, -707, -719, -731,
    -743, -755, -766, -777, -788, -799, -809, -819, -829, -839, -848, -857,
    -866, -875, -883, -891, -899, -906, -914, -921, -927, -934, -940, -946,
    -951, -957, -962, -967, -971, -975, -979, -983, -987, -990, -993, -995,
    -998, -1000, -1000, -1000, -998, -995, -993, -990, -987, -983, -979, -975,
    -971, -967, -962, -957, -951, -946, -940, -934, -927, -921, -914, -906,
    -899, -891, -883, -875, -866, -857, -848, -839, -829, -819, -809, -799,
    -788, -777, -766, -755, -743, -731, -719, -707, -695, -682, -669, -656,
    -643, -629, -616, -602, -588, -574, -559, -545, -530, -515, -500, -485,
    -469, -454, -438, -423, -407, -391, -375, -358, -342, -326, -309, -292,
    -276, -259, -242, -225, -208, -191, -174, -156, -139, -122, -105, -87,
    -70, -52, -35, -17
};

static const int16_t cos_table[360] = {
    1000, 1000, 998, 995, 993, 990, 987, 983, 979, 975, 971, 967, 962, 957,
    951, 946, 940, 934, 927, 921, 914, 906, 899, 891, 883, 875, 866, 857,
    848, 839, 829, 819, 809, 799, 788, 777, 766, 755, 743, 731, 719, 707,
    695, 682, 669, 656, 643, 629, 616, 602, 588, 574, 559, 545, 530, 515,
    500, 485, 469, 454, 438, 423, 407, 391, 375, 358, 342, 326, 309, 292,
    276, 259, 242, 225, 208, 191, 174, 156, 139, 122, 105, 87, 70, 52, 35,
    17, 0, -17, -35, -52, -70, -87, -105, -122, -139, -156, -174, -191,
    -208, -225, -242, -259, -276, -292, -309, -326, -342, -358, -375, -391,
    -407, -423, -438, -454, -469, -485, -500, -515, -530, -545, -559, -574,
    -588, -602, -616, -629, -643, -656, -669, -682, -695, -707, -719, -731,
    -743, -755, -766, -777, -788, -799, -809, -819, -829, -839, -848, -857,
    -866, -875, -883, -891, -899, -906, -914, -921, -927, -934, -940, -946,
    -951, -957, -962, -967, -971, -975, -979, -983, -987, -990, -993, -995,
    -998, -1000, -1000, -1000, -998, -995, -993, -990, -987, -983, -979, -975,
    -971, -967, -962, -957, -951, -946, -940, -934, -927, -921, -914, -906,
    -899, -891, -883, -875, -866, -857, -848, -839, -829, -819, -809, -799,
    -788, -777, -766, -755, -743, -731, -719, -707, -695, -682, -669, -656,
    -643, -629, -616, -602, -588, -574, -559, -545, -530, -515, -500, -485,
    -469, -454, -438, -423, -407, -391, -375, -358, -342, -326, -309, -292,
    -276, -259, -242, -225, -208, -191, -174, -156, -139, -122, -105, -87,
    -70, -52, -35, -17, 0, 17, 35, 52, 70, 87, 105, 122, 139, 156, 174, 191,
    208, 225, 242, 259, 276, 292, 309, 326, 342, 358, 375, 391, 407, 423,
    438, 454, 469, 485, 500, 515, 530, 545, 559, 574, 588, 602, 616, 629,
    643, 656, 669, 682, 695, 707, 719, 731, 743, 755, 766, 777, 788, 799,
    809, 819, 829, 839, 848, 857, 866, 875, 883, 891, 899, 906, 914, 921,
    927, 934, 940, 946, 951, 957, 962, 967, 971, 975, 979, 983, 987, 990,
    993, 995, 998, 1000
};

// Fixed-point multiplication
static int32_t fixed_mul(int16_t a, int16_t b) {
    return ((int32_t)a * b) / FIXED_POINT_SCALE;
}

struct cartesian_position polar_to_cartesian(const struct circular_position *pos) {
    struct cartesian_position result;

    if (pos == NULL) {
        LOG_ERR("NULL position pointer");
        result.x = CIRCULAR_CENTER_X;
        result.y = CIRCULAR_CENTER_Y;
        return result;
    }

    uint16_t angle = normalize_angle(pos->angle_deg);

    // Fixed-point trigonometric calculations
    int32_t sin_val = sin_table[angle];
    int32_t cos_val = cos_table[angle];

    // Calculate x and y using fixed-point math
    int32_t x_offset = fixed_mul(pos->radius, cos_val);
    int32_t y_offset = fixed_mul(pos->radius, sin_val);

    result.x = CIRCULAR_CENTER_X + (x_offset / 1000);
    result.y = CIRCULAR_CENTER_Y + (y_offset / 1000);

    // Clamp to display bounds
    result = clamp_position(&result);

    return result;
}

struct circular_position cartesian_to_polar(const struct cartesian_position *pos) {
    struct circular_position result;

    if (pos == NULL) {
        LOG_ERR("NULL position pointer");
        result.angle_deg = 0;
        result.radius = 0;
        return result;
    }

    // Calculate offsets from center
    int32_t x_offset = pos->x - CIRCULAR_CENTER_X;
    int32_t y_offset = pos->y - CIRCULAR_CENTER_Y;

    // Calculate radius using integer math
    result.radius = (uint16_t)sqrt(x_offset * x_offset + y_offset * y_offset);

    // Calculate angle using arctangent
    if (x_offset == 0 && y_offset == 0) {
        result.angle_deg = 0;
    } else {
        int32_t angle_rad = atan2(y_offset, x_offset) * 180 / 3.14159;
        result.angle_deg = normalize_angle((int16_t)angle_rad);
    }

    return result;
}

enum circular_quadrant get_quadrant(uint16_t angle_deg) {
    uint16_t normalized = normalize_angle(angle_deg);

    if (normalized < 90) {
        return QUADRANT_TOP_RIGHT;
    } else if (normalized < 180) {
        return QUADRANT_BOTTOM_RIGHT;
    } else if (normalized < 270) {
        return QUADRANT_BOTTOM_LEFT;
    } else {
        return QUADRANT_TOP_LEFT;
    }
}

uint16_t normalize_angle(int16_t angle_deg) {
    // Handle negative angles
    while (angle_deg < 0) {
        angle_deg += 360;
    }

    // Handle angles > 360
    while (angle_deg >= 360) {
        angle_deg -= 360;
    }

    return (uint16_t)angle_deg;
}

uint16_t get_arc_span(const struct circular_arc *arc) {
    if (arc == NULL) {
        return 0;
    }

    uint16_t start = normalize_angle(arc->start_angle_deg);
    uint16_t end = normalize_angle(arc->end_angle_deg);

    if (end >= start) {
        return end - start;
    } else {
        // Arc crosses 0 degrees
        return (360 - start) + end;
    }
}

bool is_angle_in_arc(const struct circular_arc *arc, uint16_t angle_deg) {
    if (arc == NULL) {
        return false;
    }

    uint16_t normalized = normalize_angle(angle_deg);
    uint16_t start = normalize_angle(arc->start_angle_deg);
    uint16_t end = normalize_angle(arc->end_angle_deg);

    if (end >= start) {
        return (normalized >= start && normalized <= end);
    } else {
        // Arc crosses 0 degrees
        return (normalized >= start || normalized <= end);
    }
}

struct cartesian_position get_battery_position(enum circular_quadrant quadrant,
                                               uint16_t radius,
                                               uint16_t offset_angle) {
    struct circular_position polar_pos;
    struct cartesian_position result;

    // Base angle for each quadrant (middle of quadrant)
    switch (quadrant) {
        case QUADRANT_TOP_RIGHT:
            polar_pos.angle_deg = 45 + offset_angle;
            break;
        case QUADRANT_BOTTOM_RIGHT:
            polar_pos.angle_deg = 135 + offset_angle;
            break;
        case QUADRANT_BOTTOM_LEFT:
            polar_pos.angle_deg = 225 + offset_angle;
            break;
        case QUADRANT_TOP_LEFT:
            polar_pos.angle_deg = 315 + offset_angle;
            break;
        default:
            LOG_ERR("Invalid quadrant: %d", quadrant);
            polar_pos.angle_deg = 0;
            break;
    }

    polar_pos.radius = radius;
    result = polar_to_cartesian(&polar_pos);

    return result;
}

struct circular_arc get_ble_arc(uint8_t device_index) {
    struct circular_arc arc;

    // Each BLE device gets a 90-degree arc in the outer ring
    uint16_t arc_size = 90;
    uint16_t start_angle = device_index * arc_size;

    arc.start_angle_deg = start_angle;
    arc.end_angle_deg = start_angle + arc_size - 1;
    arc.inner_radius = 85;  // Inner radius of BLE ring
    arc.outer_radius = 105; // Outer radius of BLE ring

    return arc;
}

struct cartesian_position get_center_position(uint16_t radius) {
    struct cartesian_position result;

    result.x = CIRCULAR_CENTER_X;
    result.y = CIRCULAR_CENTER_Y;

    return result;
}

uint16_t calculate_distance(const struct cartesian_position *pos1,
                           const struct cartesian_position *pos2) {
    if (pos1 == NULL || pos2 == NULL) {
        return 0;
    }

    int32_t dx = pos1->x - pos2->x;
    int32_t dy = pos1->y - pos2->y;

    return (uint16_t)sqrt(dx * dx + dy * dy);
}

uint16_t calculate_angle(const struct cartesian_position *pos1,
                        const cartesian_position *pos2) {
    if (pos1 == NULL || pos2 == NULL) {
        return 0;
    }

    int32_t x_diff = pos2->x - pos1->x;
    int32_t y_diff = pos2->y - pos1->y;

    if (x_diff == 0 && y_diff == 0) {
        return 0;
    }

    int32_t angle_rad = atan2(y_diff, x_diff) * 180 / 3.14159;
    return normalize_angle((int16_t)angle_rad);
}

bool is_valid_position(const struct cartesian_position *pos) {
    if (pos == NULL) {
        return false;
    }

    return (pos->x <= 239 && pos->y <= 239);
}

struct cartesian_position clamp_position(const struct cartesian_position *pos) {
    struct cartesian_position result = *pos;

    if (result.x > 239) result.x = 239;
    if (result.y > 239) result.y = 239;

    return result;
}

void circular_layout_init(void) {
    LOG_INF("Circular layout system initialized");
    LOG_INF("Display center: (%d, %d), Max radius: %d",
            CIRCULAR_CENTER_X, CIRCULAR_CENTER_Y, MAX_RADIUS);
}

void get_layout_constants(uint16_t *center_x, uint16_t *center_y, uint16_t *max_radius) {
    if (center_x != NULL) *center_x = CIRCULAR_CENTER_X;
    if (center_y != NULL) *center_y = CIRCULAR_CENTER_Y;
    if (max_radius != NULL) *max_radius = MAX_RADIUS;
}