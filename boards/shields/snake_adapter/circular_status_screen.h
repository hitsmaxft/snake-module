/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/kernel.h>
#include <zmk/display.h>

// Main entry point for circular UI
lv_obj_t* zmk_display_status_screen(void);

// Compatibility functions for helper integration
lv_obj_t* get_display_canvas(void);

// Theme integration functions
void set_next_theme(void);
void print_themes(void);

// Configuration functions
void configure(void);
void theme_init(void);