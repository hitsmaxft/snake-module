/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize circular status screen system
 *
 * This function initializes the complete circular UI system including
 * layout management, widget rendering, and event handling.
 *
 * @return 0 on success, error code on failure
 */
int zmk_circular_status_screen_init(void);

/**
 * @brief Start circular status screen display
 *
 * This function starts the circular UI display system and begins
 * rendering widgets on the ST7789V display.
 *
 * @return 0 on success, error code on failure
 */
int zmk_circular_status_screen_start(void);

/**
 * @brief Stop circular status screen display
 *
 * This function stops the circular UI display and cleans up
 * display resources.
 *
 * @return 0 on success, error code on failure
 */
int zmk_circular_status_screen_stop(void);

/**
 * @brief Get LVGL screen object for circular UI
 *
 * This function returns the main LVGL screen object used
 * by the circular UI system for ZMK integration.
 *
 * @return LVGL screen object or NULL if not available
 */
lv_obj_t* zmk_circular_status_screen_get_screen(void);

/**
 * @brief Check if circular UI is enabled and active
 *
 * This function checks if the circular UI system has been
 * compiled in and is currently active.
 *
 * @return true if circular UI is active, false otherwise
 */
bool zmk_circular_status_screen_is_active(void);

#ifdef __cplusplus
}
#endif

// Compile-time configuration
#ifdef CONFIG_CIRCULAR_UI_ENABLED
#define ZMK_CIRCULAR_UI_ENABLED 1
#else
#define ZMK_CIRCULAR_UI_ENABLED 0
#endif