/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include "circular_status_screen.h"
#include "widgets/circular/circular_ui.h"
#include "widgets/circular/circular_layout.h"
#include "widgets/circular/circular_rings.h"
#include "widgets/helpers/display.h"
#include "widgets/helpers/theme.h"
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zmk/display.h>
#include <zmk/event_manager.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/events/wpm_state_changed.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/usb_conn_state_changed.h>

LOG_MODULE_DECLARE(circular_custom_status_screen, CONFIG_ZMK_LOG_LEVEL);

// Splash screen configuration
#define CIRCULAR_SPLASH_DURATION 50
#define CIRCULAR_SPLASH_FINAL_COUNT 50
#define CIRCULAR_SPLASH_PROGRESS_STEPS 10

// Splash screen state
static uint8_t splash_count = 0;
static bool splash_finished = false;
static lv_obj_t *splash_screen = NULL;
static lv_obj_t *splash_canvas = NULL;

// Forward declarations
static void circular_splash_timer_cb(lv_timer_t *timer);
static int create_splash_screen(void);
static void cleanup_splash_screen(void);

// Event listeners
static ZMK_EVENT_LISTENER(circular_custom_status_screen) {};

// Event handlers
static int circular_event_layer_state_changed(const zmk_event_t *eh) {
    const struct zmk_layer_state_changed *ev = as_zmk_layer_state_changed(eh);
    return circular_status_screen_on_layer_changed((const zmk_event_t *)ev);
}

static int circular_event_wpm_state_changed(const zmk_event_t *eh) {
    const struct zmk_wpm_state_changed *ev = as_zmk_wpm_state_changed(eh);
    return circular_status_screen_on_wpm_changed((const zmk_event_t *)ev);
}

static int circular_event_battery_state_changed(const zmk_event_t *eh) {
    const struct zmk_battery_state_changed *ev = as_zmk_battery_state_changed(eh);
    return circular_status_screen_on_battery_changed((const zmk_event_t *)ev);
}

static int circular_event_ble_active_profile_changed(const zmk_event_t *eh) {
    const struct zmk_ble_active_profile_changed *ev = as_zmk_ble_active_profile_changed(eh);
    return circular_status_screen_on_ble_changed((const zmk_event_t *)ev);
}

static int circular_event_usb_conn_state_changed(const zmk_event_t *eh) {
    const struct zmk_usb_conn_state_changed *ev = as_zmk_usb_conn_state_changed(eh);
    return circular_status_screen_on_usb_changed((const zmk_event_t *)ev);
}

// Event listener registration
ZMK_EVENT_LISTENER(circular_custom_status_screen) = {
    .listeners = (struct zmk_event_subscription[]){
        ZMK_EVENT_SUBSCRIPTION(circular_event_layer_state_changed),
        ZMK_EVENT_SUBSCRIPTION(circular_event_wpm_state_changed),
        ZMK_EVENT_SUBSCRIPTION(circular_event_battery_state_changed),
        ZMK_EVENT_SUBSCRIPTION(circular_event_ble_active_profile_changed),
        ZMK_EVENT_SUBSCRIPTION(circular_event_usb_conn_state_changed),
    },
};

// Main entry point for ZMK
lv_obj_t* zmk_display_status_screen(void) {
    LOG_INF("Starting circular custom status screen");

    // Initialize display and theme system
    int ret = configure();
    if (ret != 0) {
        LOG_ERR("Failed to configure display");
        return NULL;
    }

    ret = init_display();
    if (ret != 0) {
        LOG_ERR("Failed to initialize display");
        return NULL;
    }

    ret = theme_init();
    if (ret != 0) {
        LOG_ERR("Failed to initialize theme system");
        return NULL;
    }

    // Create and show splash screen
    ret = create_splash_screen();
    if (ret != 0) {
        LOG_ERR("Failed to create splash screen");
        return NULL;
    }

    // Load splash screen
    lv_scr_load(splash_screen);

    // Start splash timer
    lv_timer_create(circular_splash_timer_cb, CIRCULAR_SPLASH_DURATION, NULL);

    return splash_screen;
}

// Splash screen implementation
static int create_splash_screen(void) {
    LOG_DBG("Creating circular splash screen");

    // Create splash screen
    splash_screen = lv_obj_create(NULL);
    if (splash_screen == NULL) {
        LOG_ERR("Failed to create splash screen");
        return -1;
    }

    // Create splash canvas
    splash_canvas = lv_canvas_create(splash_screen);
    if (splash_canvas == NULL) {
        LOG_ERR("Failed to create splash canvas");
        lv_obj_del(splash_screen);
        splash_screen = NULL;
        return -1;
    }

    // Set canvas size
    lv_obj_set_size(splash_canvas, 240, 240);

    // Clear canvas with black background
    lv_canvas_fill_bg(splash_canvas, lv_color_hex(0x000000), LV_OPA_COVER);

    // Initialize layout for splash screen
    struct circular_position splash_center = {0, 0};
    struct ring_context *splash_ring_ctx = ring_init(splash_canvas, &splash_center);

    if (splash_ring_ctx != NULL) {
        // Draw circular loading indicator
        struct ring_style splash_style = RING_STYLE_DECORATIVE;
        splash_style.color = lv_color_hex(0x444444);
        splash_style.thickness = 2;

        // Draw outer ring
        ring_draw_complete(splash_ring_ctx, 110, 120, &splash_style);

        // Draw progress arc that will animate
        struct circular_arc splash_arc = {
            .start_angle_deg = 0,
            .end_angle_deg = 0, // Will be animated
            .inner_radius = 105,
            .outer_radius = 115
        };

        // Draw initial progress
        ring_draw_arc(splash_ring_ctx, &splash_arc, &splash_style);
    }

    // Add "CIRCULAR UI" text
    lv_obj_t *label = lv_label_create(splash_screen);
    lv_label_set_text(label, "CIRCULAR UI");
    lv_obj_set_style(label, &lv_style_font_16, 0);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 40);

    // Add subtitle
    lv_obj_t *subtitle = lv_label_create(splash_screen);
    lv_label_set_text(subtitle, "Speed & Performance");
    lv_obj_set_style(subtitle, &lv_style_font_10, 0);
    lv_obj_align(subtitle, LV_ALIGN_CENTER, 0, 60);

    return 0;
}

static void cleanup_splash_screen(void) {
    LOG_DBG("Cleaning up splash screen");

    if (splash_canvas != NULL) {
        lv_obj_del(splash_canvas);
        splash_canvas = NULL;
    }

    if (splash_screen != NULL) {
        lv_obj_del(splash_screen);
        splash_screen = NULL;
    }
}

static void circular_splash_timer_cb(lv_timer_t *timer) {
    if (splash_finished) {
        return;
    }

    if (splash_count >= CIRCULAR_SPLASH_FINAL_COUNT) {
        LOG_DBG("Splash screen finished, starting main UI");

        // Cleanup splash
        cleanup_splash_screen();

        // Initialize and start circular status screen
        int ret = circular_status_screen_init();
        if (ret != 0) {
            LOG_ERR("Failed to initialize circular status screen");
            return;
        }

        ret = circular_status_screen_start();
        if (ret != 0) {
            LOG_ERR("Failed to start circular status screen");
            return;
        }

        // Load main screen
        lv_obj_t *main_screen = circular_status_screen_get_lvgl_screen();
        if (main_screen != NULL) {
            lv_scr_load(main_screen);
        }

        // Apply initial theme
        circular_ui_apply_theme(CIRCULAR_THEME_BLUE, CIRCULAR_THEME_GREEN, true);

        splash_finished = true;

        // Delete timer
        lv_timer_del(timer);
        return;
    }

    // Update splash progress
    if (splash_canvas != NULL && splash_count % (CIRCULAR_SPLASH_FINAL_COUNT / CIRCULAR_SPLASH_PROGRESS_STEPS) == 0) {
        struct circular_position splash_center = {0, 0};
        struct ring_context *splash_ring_ctx = ring_init(splash_canvas, &splash_center);

        if (splash_ring_ctx != NULL) {
            // Clear previous progress
            struct ring_style clear_style = RING_STYLE_DECORATIVE;
            clear_style.color = lv_color_hex(0x000000);
            clear_style.thickness = 3;

            struct circular_arc clear_arc = {
                .start_angle_deg = 0,
                .end_angle_deg = 360,
                .inner_radius = 105,
                .outer_radius = 115
            };

            ring_draw_arc(splash_ring_ctx, &clear_arc, &clear_style);

            // Draw new progress
            clear_style.color = lv_color_hex(0x00FF00);
            uint16_t progress_angle = (splash_count * 360) / CIRCULAR_SPLASH_FINAL_COUNT;

            struct circular_arc progress_arc = {
                .start_angle_deg = 270, // Start at top
                .end_angle_deg = 270 + progress_angle,
                .inner_radius = 105,
                .outer_radius = 115
            };

            ring_draw_arc(splash_ring_ctx, &progress_arc, &clear_style);

            // Invalidate canvas
            lv_obj_invalidate(splash_canvas);
        }
    }

    splash_count++;
}

// Compatibility functions for helper integration
lv_obj_t* get_display_canvas(void) {
    lv_obj_t *circular_canvas = circular_status_screen_get_canvas();
    if (circular_canvas != NULL) {
        return circular_canvas;
    }

    // Fallback to splash canvas during initialization
    return splash_canvas;
}

// Theme integration for circular UI
void set_next_theme(void) {
    // Delegate to circular UI theme cycling
    circular_ui_cycle_theme();
}

void print_themes(void) {
    // Could show theme information on circular UI if needed
    LOG_INF("Theme cycling requested");
}

// Initialize compatibility helpers that circular UI might need
void configure(void) {
    // Setup hardware-specific configurations
    // This would be called before display initialization
    LOG_DBG("Configuring circular display system");
}

void theme_init(void) {
    // Initialize theme system
    LOG_DBG("Initializing circular theme system");
}