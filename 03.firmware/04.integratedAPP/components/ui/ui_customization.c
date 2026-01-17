/**
 * @file ui_customization.c
 * @brief UI customization and styling for Handy Keyboard
 */

#include "ui.h"
#include "ui_customization.h"
#include "esp_log.h"
#include <math.h>

static const char *TAG = "UI_CUSTOM";

// ============================================================================
// External UI Elements
// ============================================================================

// Menu header elements (for cloning to other screens)
extern lv_obj_t *ui_HeaderPanel4;     // Menu header panel
extern lv_obj_t *ui_DateHolderLabel;  // Menu date label
extern lv_obj_t *ui_TimeHolder;       // Menu time label
extern lv_obj_t *ui_BatteryBar;       // Menu battery bar

// JPKeyboardScreen elements
extern lv_obj_t *ui_JPKeyboardScreen;
extern lv_obj_t *ui_HeaderPanel;
extern lv_obj_t *ui_Panel3;   // あ (reference for style)
extern lv_obj_t *ui_Panel4;   // か
extern lv_obj_t *ui_Panel5;   // さ
extern lv_obj_t *ui_Panel8;   // た
extern lv_obj_t *ui_Panel9;   // な
extern lv_obj_t *ui_Panel10;  // は
extern lv_obj_t *ui_Panel12;  // ?123 -> <+>
extern lv_obj_t *ui_Panel13;  // ま
extern lv_obj_t *ui_Panel14;  // や
extern lv_obj_t *ui_Panel15;  // ら
extern lv_obj_t *ui_Panel19;  // わ
extern lv_obj_t *ui_Label7;   // ?123 label

// AtoZKeyboardScreen elements
extern lv_obj_t *ui_AtoZKeyboardScreen;
extern lv_obj_t *ui_HeaderPanel1;
extern lv_obj_t *ui_OtherKeyboard;  // lv_keyboard widget

// CursorScreen elements
extern lv_obj_t *ui_CursorScreen;
extern lv_obj_t *ui_HeaderPanel2;
extern lv_obj_t *ui_Panel22;  // ESC (reference for style)
extern lv_obj_t *ui_Panel24;  // Enter
extern lv_obj_t *ui_Panel31;  // ?123 -> [a]
extern lv_obj_t *ui_Panel36;  // あ/a -> あ
extern lv_obj_t *ui_Label22;  // ?123 label
extern lv_obj_t *ui_Label27;  // あ/a label

// SettingScreen elements
extern lv_obj_t *ui_SettingScreen;
extern lv_obj_t *ui_HeaderPanel3;

// AnalogClockWithBackgroud elements
extern lv_obj_t *ui_AnalogClockWithBackgroud;
extern lv_obj_t *ui_AnalogClockContainer;

// Track which screens have been customized
static bool custom_initialized_jp = false;
static bool custom_initialized_atoz = false;
static bool custom_initialized_cursor = false;
static bool custom_initialized_settings = false;
static bool custom_initialized_clock = false;

// Clock hands canvas
static lv_obj_t *clock_canvas = NULL;

// Clock center and dimensions
#define CLOCK_CENTER_X  282
#define CLOCK_CENTER_Y  614
#define SECOND_HAND_LEN 110
#define MINUTE_HAND_LEN 90
#define HOUR_HAND_LEN   60
#define HAND_BASE_WIDTH 10

// ============================================================================
// Style Constants
// ============================================================================

#define PANEL_BG_COLOR      0x696969
#define PANEL_TEXT_COLOR    0xFFFFFF

// ============================================================================
// Helper Functions
// ============================================================================

/**
 * @brief Apply highlighted panel style (gray bg, white text)
 */
static void apply_highlight_style(lv_obj_t *panel)
{
    if (!panel) return;

    lv_obj_set_style_bg_color(panel, lv_color_hex(PANEL_BG_COLOR), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(panel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Find and style child label
    uint32_t child_count = lv_obj_get_child_count(panel);
    for (uint32_t i = 0; i < child_count; i++) {
        lv_obj_t *child = lv_obj_get_child(panel, i);
        if (lv_obj_check_type(child, &lv_label_class)) {
            lv_obj_set_style_text_color(child, lv_color_hex(PANEL_TEXT_COLOR), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }
}

/**
 * @brief Create header elements matching Menu style
 */
static void create_header_content(lv_obj_t *header_panel)
{
    if (!header_panel) return;

    // Clear existing children
    lv_obj_clean(header_panel);

    // Date label
    lv_obj_t *date_label = lv_label_create(header_panel);
    lv_obj_set_width(date_label, LV_SIZE_CONTENT);
    lv_obj_set_height(date_label, LV_SIZE_CONTENT);
    lv_obj_set_x(date_label, -118);
    lv_obj_set_y(date_label, 0);
    lv_obj_set_align(date_label, LV_ALIGN_CENTER);
    lv_label_set_text(date_label, "YYYY/MM/dd (Sat)");
    lv_obj_set_style_text_font(date_label, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Time label
    lv_obj_t *time_label = lv_label_create(header_panel);
    lv_obj_set_width(time_label, 64);
    lv_obj_set_height(time_label, LV_SIZE_CONTENT);
    lv_obj_set_x(time_label, 69);
    lv_obj_set_y(time_label, 2);
    lv_obj_set_align(time_label, LV_ALIGN_CENTER);
    lv_label_set_text(time_label, "24:00");
    lv_obj_set_style_text_font(time_label, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Battery bar
    lv_obj_t *battery_bar = lv_bar_create(header_panel);
    lv_bar_set_value(battery_bar, 25, LV_ANIM_OFF);
    lv_bar_set_start_value(battery_bar, 0, LV_ANIM_OFF);
    lv_obj_set_width(battery_bar, 80);
    lv_obj_set_height(battery_bar, 41);
    lv_obj_set_x(battery_bar, 183);
    lv_obj_set_y(battery_bar, -1);
    lv_obj_set_align(battery_bar, LV_ALIGN_CENTER);

    // Workaround for LVGL 9.1 draw crash
    if (lv_obj_get_style_pad_top(battery_bar, LV_PART_MAIN) > 0) {
        lv_obj_set_style_pad_right(battery_bar,
            lv_obj_get_style_pad_right(battery_bar, LV_PART_MAIN) + 1, LV_PART_MAIN);
    }
}

// ============================================================================
// Screen-Specific Customization
// ============================================================================

/**
 * @brief Customize JPKeyboardScreen
 */
static void customize_jp_keyboard(void)
{
    if (custom_initialized_jp) return;
    if (!ui_JPKeyboardScreen) return;

    // Update ?123 label to <+>
    if (ui_Label7) {
        lv_label_set_text(ui_Label7, "<+>");
    }

    // Apply highlight style to panels (same as Panel3/あ)
    apply_highlight_style(ui_Panel4);
    apply_highlight_style(ui_Panel5);
    apply_highlight_style(ui_Panel8);
    apply_highlight_style(ui_Panel9);
    apply_highlight_style(ui_Panel10);
    apply_highlight_style(ui_Panel13);
    apply_highlight_style(ui_Panel14);
    apply_highlight_style(ui_Panel15);
    apply_highlight_style(ui_Panel19);

    create_header_content(ui_HeaderPanel);
    custom_initialized_jp = true;
    ESP_LOGD(TAG, "JPKeyboard customized");
}

/**
 * @brief Customize AtoZKeyboardScreen
 */
static void customize_atoz_keyboard(void)
{
    if (custom_initialized_atoz) return;
    if (!ui_AtoZKeyboardScreen) return;

    create_header_content(ui_HeaderPanel1);
    custom_initialized_atoz = true;
    ESP_LOGD(TAG, "AtoZ customized");
}

/**
 * @brief Customize CursorScreen
 */
static void customize_cursor(void)
{
    if (custom_initialized_cursor) return;
    if (!ui_CursorScreen) return;

    if (ui_Label22) {
        lv_label_set_text(ui_Label22, "[a]");
    }
    if (ui_Label27) {
        lv_label_set_text(ui_Label27, "あ");
    }

    apply_highlight_style(ui_Panel24);
    create_header_content(ui_HeaderPanel2);
    custom_initialized_cursor = true;
    ESP_LOGD(TAG, "Cursor customized");
}

/**
 * @brief Customize SettingScreen
 */
static void customize_settings(void)
{
    if (custom_initialized_settings) return;
    if (!ui_SettingScreen) return;

    custom_initialized_settings = true;
    ESP_LOGD(TAG, "Settings customized");
}

// ============================================================================
// Clock Hands Drawing
// ============================================================================

/**
 * @brief Draw a triangular clock hand
 *
 * Fixed: Ensure minimum base offset to prevent degenerate triangles
 * at horizontal angles (e.g., 59s, 1s, 29s, 31s where perpendicular
 * offset rounds to 0 in one dimension).
 */
static void draw_clock_hand(lv_layer_t *layer, int32_t cx, int32_t cy,
                            float angle_deg, int32_t length, int32_t base_width,
                            lv_color_t color)
{
    float angle_rad = angle_deg * M_PI / 180.0f;

    // Calculate tip point
    int32_t tip_x = cx + (int32_t)roundf(length * cosf(angle_rad));
    int32_t tip_y = cy + (int32_t)roundf(length * sinf(angle_rad));

    // Calculate perpendicular offset for base points
    // Perpendicular to hand direction: (-sin, cos) for CCW rotation
    float half_base = base_width / 2.0f;
    float perp_x = -sinf(angle_rad) * half_base;
    float perp_y = cosf(angle_rad) * half_base;

    // Round offsets, but preserve direction when rounding to 0
    int32_t offset_x = (int32_t)roundf(perp_x);
    int32_t offset_y = (int32_t)roundf(perp_y);

    // Ensure minimum 1 pixel offset when float value is non-trivial
    // This prevents visual jumps at angles where offset rounds to 0
    if (offset_x == 0 && fabsf(perp_x) > 0.1f) {
        offset_x = (perp_x > 0) ? 1 : -1;
    }
    if (offset_y == 0 && fabsf(perp_y) > 0.1f) {
        offset_y = (perp_y > 0) ? 1 : -1;
    }

    // Fallback: ensure at least one offset is non-zero
    if (offset_x == 0 && offset_y == 0) {
        offset_y = 1;  // Default to vertical offset
    }

    int32_t base1_x = cx + offset_x;
    int32_t base1_y = cy + offset_y;
    int32_t base2_x = cx - offset_x;
    int32_t base2_y = cy - offset_y;

    // Draw triangle (winding order is always CCW with this calculation)
    lv_draw_triangle_dsc_t dsc;
    lv_draw_triangle_dsc_init(&dsc);
    dsc.bg_color = color;
    dsc.bg_opa = LV_OPA_COVER;
    dsc.p[0].x = tip_x;
    dsc.p[0].y = tip_y;
    dsc.p[1].x = base1_x;
    dsc.p[1].y = base1_y;
    dsc.p[2].x = base2_x;
    dsc.p[2].y = base2_y;

    lv_draw_triangle(layer, &dsc);
}

/**
 * @brief Clock canvas draw event callback
 */
static void clock_draw_event_cb(lv_event_t *e)
{
    lv_layer_t *layer = lv_event_get_layer(e);

    // Get current time from user data (hour, minute, second packed into pointer)
    uintptr_t time_data = (uintptr_t)lv_event_get_user_data(e);
    int hour = (time_data >> 16) & 0xFF;
    int minute = (time_data >> 8) & 0xFF;
    int second = time_data & 0xFF;

    // Calculate angles (0 = right = 12 o'clock, clockwise)
    float second_angle = (second * 6.0f);
    float minute_angle = (minute * 6.0f) + (second * 0.1f);
    float hour_angle = ((hour % 12) * 30.0f) + (minute * 0.5f);

    lv_color_t black = lv_color_hex(0x000000);

    // Draw hands (hour first, then minute, then second on top)
    draw_clock_hand(layer, CLOCK_CENTER_X, CLOCK_CENTER_Y,
                    hour_angle, HOUR_HAND_LEN, HAND_BASE_WIDTH, black);
    draw_clock_hand(layer, CLOCK_CENTER_X, CLOCK_CENTER_Y,
                    minute_angle, MINUTE_HAND_LEN, HAND_BASE_WIDTH - 2, black);
    draw_clock_hand(layer, CLOCK_CENTER_X, CLOCK_CENTER_Y,
                    second_angle, SECOND_HAND_LEN, HAND_BASE_WIDTH - 4, black);
}

/**
 * @brief Create clock hands overlay
 */
static void create_clock_hands(void)
{
    if (!ui_AnalogClockWithBackgroud) return;

    clock_canvas = lv_obj_create(ui_AnalogClockWithBackgroud);
    lv_obj_remove_style_all(clock_canvas);
    lv_obj_set_size(clock_canvas, 480, 800);
    lv_obj_set_pos(clock_canvas, 0, 0);
    lv_obj_remove_flag(clock_canvas, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(clock_canvas, clock_draw_event_cb, LV_EVENT_DRAW_MAIN, NULL);

    custom_initialized_clock = true;
    ESP_LOGD(TAG, "Clock hands created at (%d,%d)", CLOCK_CENTER_X, CLOCK_CENTER_Y);
}

// ============================================================================
// Public Functions
// ============================================================================

/**
 * @brief Update check - called periodically
 */
static void customization_update(void)
{
    if (ui_JPKeyboardScreen && !custom_initialized_jp) {
        customize_jp_keyboard();
    }

    if (ui_AtoZKeyboardScreen && !custom_initialized_atoz) {
        customize_atoz_keyboard();
    }

    if (ui_CursorScreen && !custom_initialized_cursor) {
        customize_cursor();
    }

    if (ui_SettingScreen && !custom_initialized_settings) {
        customize_settings();
    }

    if (ui_AnalogClockWithBackgroud && !custom_initialized_clock) {
        create_clock_hands();
    }
}

/**
 * @brief Timer callback
 */
static void customization_timer_cb(lv_timer_t *timer)
{
    customization_update();
}

/**
 * @brief Update clock hands
 */
void ui_clock_update_hands(int hour, int minute, int second)
{
    if (!clock_canvas) return;

    // Pack time data into user_data pointer
    uintptr_t time_data = ((hour & 0xFF) << 16) | ((minute & 0xFF) << 8) | (second & 0xFF);

    // Update event user data and invalidate to trigger redraw
    lv_obj_remove_event_cb(clock_canvas, clock_draw_event_cb);
    lv_obj_add_event_cb(clock_canvas, clock_draw_event_cb, LV_EVENT_DRAW_MAIN, (void*)time_data);
    lv_obj_invalidate(clock_canvas);
}

/**
 * @brief Update header with current date/time
 */
void ui_header_update(const char *date_str, const char *time_str, int battery_pct)
{
    // Update Menu header
    if (ui_DateHolderLabel) {
        lv_label_set_text(ui_DateHolderLabel, date_str);
    }
    if (ui_TimeHolder) {
        lv_label_set_text(ui_TimeHolder, time_str);
    }
    if (ui_BatteryBar) {
        lv_bar_set_value(ui_BatteryBar, battery_pct, LV_ANIM_OFF);
    }

    // TODO: Update other headers when they have similar structure
}

/**
 * @brief Initialize UI customization
 */
void ui_customization_init(void)
{
    lv_timer_create(customization_timer_cb, 100, NULL);
    ESP_LOGI(TAG, "Customization initialized");
}
