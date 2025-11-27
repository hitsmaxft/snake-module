```
#include "lvgl.h"

/* 用于 canvas 绘制圆环和刻度 */
static void draw_ring_and_ticks(lv_obj_t * canvas, int center_x, int center_y);

void ui_create_240(void)
{
    /* 主屏幕（240x240） */
    lv_obj_t * scr = lv_obj_create(NULL);
    lv_scr_load(scr);

    lv_obj_set_size(scr, 240, 240);
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
    lv_obj_set_style_pad_all(scr, 0, 0);
    lv_obj_center(scr);

    /* ================================
     * 1. 外环 + 刻度（使用 canvas 绘制）
     * ================================ */
    static lv_color_t cbuf[240 * 240];
    lv_obj_t * canvas = lv_canvas_create(scr);
    lv_canvas_set_buffer(canvas, cbuf, 240, 240, LV_IMG_CF_TRUE_COLOR);
    lv_obj_center(canvas);

    lv_canvas_fill_bg(canvas, lv_color_black(), LV_OPA_COVER);
    draw_ring_and_ticks(canvas, 120, 120);

    /* ================================
     * 2. 中心容器 (用于相对布局所有文字)
     * ================================ */
    lv_obj_t * center = lv_obj_create(scr);
    lv_obj_set_size(center, 240, 240);
    lv_obj_clear_flag(center, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_center(center);

    lv_obj_set_layout(center, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(center, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(center, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* ===================================================
     * 3. 主数字（数字）
     * =================================================== */
    lv_obj_t * main_num = lv_label_create(center);
    lv_obj_set_style_text_font(main_num, &lv_font_montserrat_48, 0);
    lv_label_set_text(main_num, "39");

    /* ===================================================
     * 4. 单位文字（文字）
     * =================================================== */
    lv_obj_t * unit = lv_label_create(center);
    lv_obj_set_style_text_font(unit, &lv_font_montserrat_18, 0);
    lv_label_set_text(unit, "km/h");

    /* ===================================================
     * 5. 中号标签（文字）
     * =================================================== */
    lv_obj_t * mid_label = lv_label_create(center);
    lv_obj_set_style_text_font(mid_label, &lv_font_montserrat_28, 0);
    lv_label_set_text(mid_label, "LABEL");

    /* ===================================================
     * 6. 小标签（带背景）
     * =================================================== */
    lv_obj_t * small_label_box = lv_obj_create(center);
    lv_obj_set_size(small_label_box, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_hor(small_label_box, 6, 0);
    lv_obj_set_style_pad_ver(small_label_box, 2, 0);
    lv_obj_set_style_bg_color(small_label_box, lv_color_hex(0xDDDDDD), 0);

    lv_obj_t * small_label = lv_label_create(small_label_box);
    lv_label_set_text(small_label, "TAG");
    lv_obj_set_style_text_font(small_label, &lv_font_montserrat_12, 0);

    /* ===================================================
     * 7. 辅助信息（文字）
     * =================================================== */
    lv_obj_t * info = lv_label_create(center);
    lv_label_set_text(info, "INFO MESSAGE");
    lv_obj_set_style_text_font(info, &lv_font_montserrat_12, 0);

    /* ===================================================
     * 8. 左右图标（图案）
     * =================================================== */
    lv_obj_t * left_icon = lv_label_create(scr);
    lv_label_set_text(left_icon, LV_SYMBOL_WARNING);  /* 替代图标 */
    lv_obj_set_style_text_color(left_icon, lv_color_white(), 0);
    lv_obj_align(left_icon, LV_ALIGN_CENTER, -38, -38);

    lv_obj_t * right_icon = lv_label_create(scr);
    lv_label_set_text(right_icon, LV_SYMBOL_OK);  /* 替代图标 */
    lv_obj_set_style_text_color(right_icon, lv_color_white(), 0);
    lv_obj_align(right_icon, LV_ALIGN_CENTER, 38, -38);
}


/* ===================================================
 * 绘制外环与刻度 (Canvas)
 * =================================================== */
static void draw_ring_and_ticks(lv_obj_t * canvas, int cx, int cy)
{
    lv_draw_line_dsc_t line_dsc;
    lv_draw_line_dsc_init(&line_dsc);

    /* 外环颜色 */
    line_dsc.color = lv_color_hex(0xBDBDBD);
    line_dsc.width = 12;

    /* 外环绘制为粗线圆 */
    lv_canvas_draw_arc(canvas,
                       cx, cy,
                       106, 0, 360,
                       &line_dsc);

    /* 刻度线参数 */
    lv_draw_line_dsc_t tick_dsc;
    lv_draw_line_dsc_init(&tick_dsc);
    tick_dsc.color = lv_color_hex(0xCCCCCC);

    for (int deg = 0; deg < 360; deg += 10) {
        int is_major = (deg % 30) == 0;

        int r1 = is_major ? 86 : 90;
        int r2 = is_major ? 94 : 94;

        float rad = deg * 3.14159f / 180.0f;

        lv_point_t p1 = { cx + r1 * cosf(rad), cy + r1 * sinf(rad) };
        lv_point_t p2 = { cx + r2 * cosf(rad), cy + r2 * sinf(rad) };

        lv_canvas_draw_line(canvas, &p1, &p2, &tick_dsc);
    }

    /* 可选：某段刻度染色（如 300°–330°） */
    lv_draw_line_dsc_t red_dsc;
    lv_draw_line_dsc_init(&red_dsc);
    red_dsc.color = lv_color_hex(0xE53935);

    for (int deg = 300; deg <= 330; deg += 10) {
        float rad = deg * 3.14159f / 180.0f;

        lv_point_t p1 = { cx + 86 * cosf(rad), cy + 86 * sinf(rad) };
        lv_point_t p2 = { cx + 94 * cosf(rad), cy + 94 * sinf(rad) };

        lv_canvas_draw_line(canvas, &p1, &p2, &red_dsc);
    }
}

```
